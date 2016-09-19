///==============================================================================
/// GAP (General Autonomous Parallelizer) License
///==============================================================================
///
/// GAP is distributed under the following BSD-style license:
///
/// Copyright (c) 2016 Dzanan Bajgoric
/// All rights reserved.
/// 
/// Redistribution and use in source and binary forms, with or without modification,
/// are permitted provided that the following conditions are met:
/// 
/// 1. Redistributions of source code must retain the above copyright notice, this
///    list of conditions and the following disclaimer.
/// 
/// 2. Redistributions in binary form must reproduce the above copyright notice, this
///    list of conditions and the following disclaimer in the documentation and/or other
///    materials provided with the distribution.
/// 
/// 3. The name of the author may not be used to endorse or promote products derived from
///    this software without specific prior written permission from the author.
/// 
/// 4. Products derived from this software may not be called "GAP" nor may "GAP" appear
///    in their names without specific prior written permission from the author.
/// 
/// THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
/// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
/// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
/// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO
/// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
/// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
/// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "nest_rewriter.h"
#include "nest_transformer.h"
#include "perfect_loop_nest.h"
#include "clang_allocator.h"
#include "ast_helpers.h"
#include <algorithm>
#include <string>
#include <cassert>


namespace gap
{
	namespace
	{

		/// Finds each DER expression in the nest and, if it references an index
		/// variable in the original nest, it sets its declaration to corresponding
		/// index variable in the transformed nest
		void UpdateIdxDerDelcsToNewIdxVarDecls(
			clang::Stmt* stmt
			, const std::vector<clang::VarDecl*>& old_idx_vec
			, std::vector<clang::VarDecl*>& new_idx_vec)
		{
			if (stmt == nullptr)
				return;

			if (auto decl_ref_expr = clang::dyn_cast<clang::DeclRefExpr>(stmt))
			{
				auto idx_var_decl = std::find(
					old_idx_vec.begin()
					, old_idx_vec.end()
					, GetVarDecl(decl_ref_expr));

				if (idx_var_decl != old_idx_vec.end())
					decl_ref_expr->setDecl(new_idx_vec[std::distance(old_idx_vec.begin(), idx_var_decl)]);

				return;
			}
			for (auto child_iter = stmt->child_begin(); child_iter != stmt->child_end(); ++child_iter)
				UpdateIdxDerDelcsToNewIdxVarDecls(*child_iter, old_idx_vec, new_idx_vec);
		}

		clang::Expr* GetSingleBndExpr(
			clang::ASTContext& ast_ctx
			, const arma::subview_col<util::Rational<long long>>& single_bnd_coeffs
			, const util::Rational<long long>& single_bnd_constant
			, std::vector<clang::VarDecl*>& outer_loops_idx_vars)
		{
			assert(single_bnd_coeffs.n_elem > 0 && "bound must be a non-empty column vector");
			assert(
				outer_loops_idx_vars.size() == single_bnd_coeffs.n_elem - 1
				&& "vector of outer loop indices and column vector representing a bound must be compatible");

			clang::BinaryOperator* single_bnd_expr = nullptr;
			ClangAllocator allocator(ast_ctx);
			for (std::size_t i = 0; i < outer_loops_idx_vars.size(); ++i)
			{
				auto mult_op = CreateBinOp(
					ast_ctx
					, CreateFloatLiteral(ast_ctx, llvm::APFloat(single_bnd_coeffs[i]), ast_ctx.FloatTy)
					, CreateDeclRefExpr(ast_ctx, outer_loops_idx_vars[i], outer_loops_idx_vars[i]->getType())
					, clang::BO_Mul
					, ast_ctx.FloatTy);

				if (single_bnd_expr != nullptr)
					single_bnd_expr->setRHS(mult_op);

				single_bnd_expr = CreateBinOp(
					ast_ctx
					, single_bnd_expr == nullptr ? mult_op : single_bnd_expr
					, allocator.Alloc<clang::GNUNullExpr>(clang::Stmt::EmptyShell())
					, clang::BO_Add
					, ast_ctx.FloatTy);
			}

			/// The case when constructing the outermost loop's bound
			if (single_bnd_expr == nullptr)
				return CreateFloatLiteral(ast_ctx, llvm::APFloat(single_bnd_constant), ast_ctx.FloatTy);

			single_bnd_expr->setRHS(CreateFloatLiteral(ast_ctx, llvm::APFloat(single_bnd_constant), ast_ctx.FloatTy));
			return single_bnd_expr;
		}

		/// Outer loops index variables must be ordered starting from the outermost loop
		/// in the outer_loops_idx_vars argument
		clang::Expr* GetTransformedBndExpr(
			clang::ASTContext& ast_ctx
			, const util::Bound& new_bnd
			, std::vector<clang::VarDecl*>& outer_loops_idx_vars
			, const clang::VarDecl& curr_loop_idx_var
			, bool is_lower_bnd)
		{
			assert(new_bnd.first.n_cols > 0 && "there must be at least single bound column vector");
			clang::FunctionDecl* ceil_or_floor_fn_decl = NoLoadLookup<clang::FunctionDecl>(
				ast_ctx
				, *ast_ctx.getTranslationUnitDecl()
				, is_lower_bnd ? "ceilf" : "floorf");

			assert(ceil_or_floor_fn_decl != nullptr && "ceilf and floorf functions must be in scope");
			clang::FunctionDecl* min_or_max_fn_decl = NoLoadLookup<clang::FunctionDecl>(
				ast_ctx
				, *ast_ctx.getTranslationUnitDecl()
				, is_lower_bnd ? "__max_arg" : "__min_arg");

			assert(min_or_max_fn_decl != nullptr && "__min_arg and __max_arg functions must be in scope");
			if (new_bnd.first.n_cols == 1)
			{
				/// Note that in case of upper bound +1 is added to the upper bound expresion as '<' is
				/// the loop's condition operator
				util::Rational<long long> sngl_bnd_constant =
					is_lower_bnd ? new_bnd.second[0] : new_bnd.second[0] + static_cast<long long>(1);

				/// Bound consists of a single expression
				return CreateCStyleCastExpr(
					ast_ctx
					, curr_loop_idx_var.getType()
					, clang::CK_FloatingToIntegral
					, CreateCallExpr(
						ast_ctx
						, CreateDeclRefExpr(ast_ctx, ceil_or_floor_fn_decl, ceil_or_floor_fn_decl->getReturnType())
						, { GetSingleBndExpr(ast_ctx, new_bnd.first.col(0), sngl_bnd_constant, outer_loops_idx_vars) }
				, ceil_or_floor_fn_decl->getReturnType()));
			}

			/// There are multiple possible lower or upper bounds, thus special min/max functions
			/// will be used to choose the proper bound at runtime
			std::vector<clang::Expr*> bnd_exprs;
			bnd_exprs.reserve(new_bnd.first.n_cols);
			for (std::size_t col_idx = 0U; col_idx < new_bnd.first.n_cols; ++col_idx)
			{
				/// Note that in case of upper bound +1 is added to the upper bound expresion as '<' is
				/// the loop's condition operator
				util::Rational<long long> sngl_bnd_constant =
					is_lower_bnd ? new_bnd.second[col_idx] : new_bnd.second[col_idx] + static_cast<long long>(1);

				bnd_exprs.push_back(
					GetSingleBndExpr(
						ast_ctx
						, new_bnd.first.col(col_idx)
						, sngl_bnd_constant
						, outer_loops_idx_vars));
			}

			return CreateCStyleCastExpr(
				ast_ctx
				, curr_loop_idx_var.getType()
				, clang::CK_FloatingToIntegral
				, CreateCallExpr(
					ast_ctx
					, CreateDeclRefExpr(ast_ctx, ceil_or_floor_fn_decl, ceil_or_floor_fn_decl->getReturnType()),
					{
						CreateCallExpr(
							ast_ctx
							, CreateDeclRefExpr(ast_ctx, min_or_max_fn_decl, min_or_max_fn_decl->getReturnType()),
							{
								CreateIntLiteral(ast_ctx, GetUnsigned(64, new_bnd.first.n_cols), ast_ctx.getSizeType())
								, CreateCmpndLiteralExpr(
									ast_ctx
									, CreateIncompleteArrType(ast_ctx, bnd_exprs[0]->getType())
									, CreateInitListExpr(ast_ctx, bnd_exprs)
									, clang::VK_LValue)
							}
							, min_or_max_fn_decl->getReturnType())
					}
			, ceil_or_floor_fn_decl->getReturnType()));
		}

		/// Updates lower and upper nest bound exprs. according to the transformed nest
		/// bounds
		void UpdateLoopNestBnds(
			clang::ASTContext& ast_ctx
			, ForLoopHeader* outermost_loop_hdr
			, const std::vector<clang::VarDecl*>& nest_idx_vec
			, const NestTransformer& transformer)
		{
			std::vector<clang::VarDecl*> outer_loops_idx_vars;
			outer_loops_idx_vars.reserve(nest_idx_vec.size());
			ForLoopHeader* loop_hdr = outermost_loop_hdr;
			auto & lower_nest_bnd = transformer.GetLowerBnd();
			auto & upper_nest_bnd = transformer.GetUpperBnd();
			std::size_t i(0);

			while (loop_hdr != nullptr)
			{
				auto for_stmt = loop_hdr->GetStmt();
				clang::cast<clang::BinaryOperator>(for_stmt->getInit())->setRHS(
					GetTransformedBndExpr(
						ast_ctx
						, lower_nest_bnd[i]
						, outer_loops_idx_vars
						, *nest_idx_vec[i]
						, true));

				clang::cast<clang::BinaryOperator>(for_stmt->getCond())->setRHS(
					GetTransformedBndExpr(
						ast_ctx
						, upper_nest_bnd[i]
						, outer_loops_idx_vars
						, *nest_idx_vec[i]
						, false));

				outer_loops_idx_vars.push_back(nest_idx_vec[i]);
				loop_hdr = loop_hdr->GetChild();
				++i;
			}
		}

		/// Produces the mapping of a single loop index variable in the transformed nest
		/// to the expression according to the single transformation matrix column. Note
		/// that index variables must be ordered starting from the outermost loop
		clang::Expr* GetSingleIdxVarReferenceExpr(
			clang::ASTContext& ast_ctx
			, const std::vector<clang::VarDecl*>& idx_vars
			, const arma::subview_col<util::IntMatElemType>& transform_mat_col)
		{
			clang::Expr* idx_var_ref_expr = nullptr;
			ClangAllocator allocator(ast_ctx);

			for (std::size_t row = 0; row < transform_mat_col.n_elem; ++row)
			{
				if (transform_mat_col[row] == 0)
					continue;

				auto mul_op = CreateBinOp(
					ast_ctx
					, CreateIntLiteral(ast_ctx, GetSigned(64, transform_mat_col[row]), idx_vars[row]->getType())
					, CreateDeclRefExpr(ast_ctx, idx_vars[row], idx_vars[row]->getType())
					, clang::BO_Mul
					, idx_vars[row]->getType());

				if (idx_var_ref_expr == nullptr)
					idx_var_ref_expr = mul_op;
				else
					idx_var_ref_expr = CreateBinOp(
						ast_ctx
						, idx_var_ref_expr
						, mul_op
						, clang::BO_Add
						, idx_vars[row]->getType());
			}

			assert(idx_var_ref_expr != nullptr && "this is a bug");
			return idx_var_ref_expr;
		}

		/// Determines how the index variables of the transformed nest map to the
		/// index variables of the original nest according to the transformation
		/// matrix. These exprs. will replace all the occurences of the old index
		/// variables in the transformed nest
		void CalcNewReferenceExprsForLoopIdxVars(
			clang::ASTContext& ast_ctx
			, const std::vector<clang::VarDecl*>& idx_vars
			, const util::IntMatrixType& transform_mat
			, std::map<clang::VarDecl*, clang::Expr*>& idx_var_ref_exprs_map)
		{
			assert(
				idx_vars.size() == transform_mat.n_rows
				&& idx_vars.size() == transform_mat.n_cols
				&& "the number of loop index variables must match the transform matrix dimension");

			for (std::size_t i = 0; i < transform_mat.n_cols; ++i)
				assert(
					idx_var_ref_exprs_map.insert(
						std::make_pair(
							idx_vars[i]
							, CreateParenExpr(
								ast_ctx
								, GetSingleIdxVarReferenceExpr(ast_ctx, idx_vars, transform_mat.col(i))))).second);
		}

		/// Finds and replaces all DERs that reference the original loop nest index variables
		/// with new expressions built previously using the nest transformation matrix
		void ReplaceLoopIdxVarsDeclRefExprs(
			const std::map<clang::VarDecl*, clang::Expr*>& new_idx_var_ref_exprs
			, clang::Stmt* immediate_parent
			, clang::Stmt* stmt)
		{
			assert(stmt != nullptr && "this must never happen");
			if (auto der = clang::dyn_cast<clang::DeclRefExpr>(stmt))
			{
				auto var_decl_expr_pair = new_idx_var_ref_exprs.find(
					GetVarDecl(der));

				if (var_decl_expr_pair != new_idx_var_ref_exprs.end())
				{
					assert(immediate_parent != nullptr && "parent must exist at this point");
					std::replace(
						immediate_parent->child_begin()
						, immediate_parent->child_end()
						, static_cast<clang::Expr*>(der)
						, var_decl_expr_pair->second);

					return;
				}
			}
			for (auto iter = stmt->child_begin(); iter != stmt->child_end(); ++iter)
				ReplaceLoopIdxVarsDeclRefExprs(new_idx_var_ref_exprs, stmt, *iter);
		}

	} /// Anonymous namespace


	NestRewriter::NestRewriter(
		clang::ASTContext& ast_ctx
		, PerfectLoopNest& original_nest
		, const NestTransformer& transformer)

		: m_transformed_nest(original_nest)
	{
		GenerateNewNestIdxVector(ast_ctx, original_nest.GetNestDepth());
		UpdateIdxDerDelcsToNewIdxVarDecls(
			m_transformed_nest.GetOutermostLoopHdr().GetStmt()
			, original_nest.GetNestIdxVec()
			, m_new_nest_idx_vec);

		UpdateLoopNestBnds(
			ast_ctx
			, &m_transformed_nest.GetOutermostLoopHdr()
			, m_new_nest_idx_vec
			, transformer);

		std::map<clang::VarDecl*, clang::Expr*> new_idx_var_ref_exprs;
		CalcNewReferenceExprsForLoopIdxVars(
			ast_ctx
			, m_new_nest_idx_vec
			, transformer.GetTransformMat()
			, new_idx_var_ref_exprs);

		/// Perform innermost loop body traversal that will update any references to
		/// the loop index variables with exprs in function of new loop variables
		ReplaceLoopIdxVarsDeclRefExprs(
			new_idx_var_ref_exprs
			, nullptr
			, m_transformed_nest.GetLoopHdr(original_nest.GetNestDepth() - 1)->GetStmt()->getBody());
	}

	void NestRewriter::GenerateNewNestIdxVector(
		clang::ASTContext& ast_ctx
		, std::size_t nest_depth
		, const std::string& idx_var_base_name)
	{
		m_new_nest_idx_vec.reserve(nest_depth);
		for (std::size_t i = 0; i < nest_depth; ++i)
			m_new_nest_idx_vec.push_back(
				CreateVarDecl(
					ast_ctx
					, "__" + idx_var_base_name + std::to_string(i)
					, ast_ctx.LongLongTy));
	}

	PerfectLoopNest& NestRewriter::GetTransformedNest()
	{
		return m_transformed_nest;
	}

	std::vector<clang::VarDecl*>& NestRewriter::GetNewIdxVec()
	{
		return m_new_nest_idx_vec;
	}

	const PerfectLoopNest& NestRewriter::GetTransformedNest() const
	{
		return m_transformed_nest;
	}

	const std::vector<clang::VarDecl*>& NestRewriter::GetNewIdxVec() const
	{
		return m_new_nest_idx_vec;
	}

} /// namespace gap