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

#include "kernel_def_gen.h"
#include "nest_analysis_driver.h"
#include "invocation_stmts_gen.h"
#include "perfect_loop_nest.h"
#include "ast_helpers.h"
#include "linear_expr_serializer.h"
#include <algorithm>


namespace gap
{
	namespace
	{

		void EliminateDupVarDecls(
			const std::vector<clang::VarDecl*>& input_vars
			, const std::vector<clang::VarDecl*>& output_vars
			, std::vector<clang::VarDecl*>& unique_vars)
		{
			unique_vars.reserve(input_vars.size() + output_vars.size());
			unique_vars = input_vars;
			for (auto var_decl : output_vars)
				if (std::find(unique_vars.begin(), unique_vars.end(), var_decl)
					== unique_vars.end())
					unique_vars.push_back(var_decl);
		}

		std::vector<clang::QualType> GetKernelParamTypes(
			clang::ASTContext& ast_ctx
			, VarDeclArrInfoMap& host_var_arr_info_map
			, Transformation::TransformType transform_type
			, const std::vector<clang::VarDecl*>& loop_idx_vec)
		{
			std::vector<clang::QualType> kernel_param_types;
			if (transform_type == Transformation::TRANSFORM_INNER_PAR)
				kernel_param_types.push_back(loop_idx_vec[0]->getType());

			for (auto arr_info_pair : host_var_arr_info_map)
			{
				kernel_param_types.push_back(
					ast_ctx.getPointerType(ast_ctx.getBaseElementType(arr_info_pair.first->getType())));

				auto & array_id = arr_info_pair.second.ArrayId();
				for (std::size_t i = 0; i < array_id.GetDimensionality(); ++i)
					kernel_param_types.push_back(ast_ctx.getSizeType());
			}

			return kernel_param_types;
		}

		clang::Expr* GetLowerBndExpr(
			PerfectLoopNest& nest
			, std::size_t loop_idx)
		{
			assert(loop_idx < nest.GetNestDepth());
			auto init_binop = clang::cast<clang::BinaryOperator>(
				nest.GetLoopHdr(loop_idx)->GetStmt()->getInit());

			return init_binop->getRHS();
		}

		clang::Expr* GetUpperBndExpr(
			PerfectLoopNest& nest
			, std::size_t loop_idx)
		{
			assert(loop_idx < nest.GetNestDepth());
			auto cond_binop = clang::cast<clang::BinaryOperator>(
				nest.GetLoopHdr(loop_idx)->GetStmt()->getCond());

			return cond_binop->getRHS();
		}

		clang::VarDecl* ExtractIdxExprsFromArrSub(
			clang::ASTContext& ast_ctx
			, clang::ArraySubscriptExpr* arr_sub_expr
			, std::vector<clang::Expr*>& sub_idx_exprs)
		{
			assert(arr_sub_expr != nullptr);
			clang::VarDecl* array_var_decl = nullptr;
			auto arr_sub_base = arr_sub_expr->getBase()->IgnoreCasts();

			if (auto array_dre_expr = clang::dyn_cast<clang::DeclRefExpr>(arr_sub_base))
				array_var_decl = GetVarDecl(array_dre_expr);
			else
				array_var_decl = ExtractIdxExprsFromArrSub(
					ast_ctx
					, clang::dyn_cast<clang::ArraySubscriptExpr>(arr_sub_base)
					, sub_idx_exprs);

			sub_idx_exprs.push_back(arr_sub_expr->getIdx());
			return array_var_decl;
		}

		clang::VarDecl* GetArrSubExprVarDecl(clang::ArraySubscriptExpr* arr_sub_expr)
		{
			assert(arr_sub_expr != nullptr);
			auto arr_sub_base = arr_sub_expr->getBase()->IgnoreCasts();

			if (auto array_dre_expr = clang::dyn_cast<clang::DeclRefExpr>(arr_sub_base))
				return GetVarDecl(array_dre_expr);
			else
				return GetArrSubExprVarDecl(clang::dyn_cast<clang::ArraySubscriptExpr>(arr_sub_base));
		}

		/// Returns true if the given flat array subscript index expr references the provided
		/// loop index variable decl
		bool FlatArrIdxExprRefsIdxVar(
			const clang::Stmt* expr
			, const clang::VarDecl* idx_var_decl)
		{
			if (expr == nullptr)
				return false;
			if (auto decl_ref_expr = clang::dyn_cast<clang::DeclRefExpr>(expr))
				return GetVarDecl(decl_ref_expr) == idx_var_decl;

			for (auto child_iter = expr->child_begin(); child_iter != expr->child_end(); ++child_iter)
				if (FlatArrIdxExprRefsIdxVar(*child_iter, idx_var_decl))
					return true;

			return false;
		}

		clang::ForStmt* CreateForStmtHelper(
			clang::ASTContext& ast_ctx
			, clang::VarDecl* idx_var
			, clang::VarDecl* low_bnd_var
			, clang::VarDecl* upp_bnd_var)
		{
			return CreateForStmt(
				ast_ctx
				, CreateBinOp(
					ast_ctx
					, CreateDeclRefExpr(ast_ctx, idx_var, idx_var->getType())
					, CreateDeclRefExpr(ast_ctx, low_bnd_var, low_bnd_var->getType())
					, clang::BO_Assign
					, idx_var->getType())

				, CreateBinOp(
					ast_ctx
					, CreateDeclRefExpr(ast_ctx, idx_var, idx_var->getType())
					, CreateDeclRefExpr(ast_ctx, upp_bnd_var, upp_bnd_var->getType())
					, clang::BO_LT
					, ast_ctx.BoolTy)

				, CreateUnaryOp(
					ast_ctx
					, CreateDeclRefExpr(ast_ctx, idx_var, idx_var->getType())
					, clang::UO_PreInc
					, idx_var->getType())

				, nullptr);
		}

		void TransformArrSubExpr(
			clang::ASTContext& ast_ctx
			, clang::ArraySubscriptExpr* arr_sub_expr
			, clang::VarDecl* dev_arr_var
			, clang::VarDecl* flat_idx_expr_var)
		{
			arr_sub_expr->setLHS(
				CreateDeclRefExpr(
					ast_ctx
					, dev_arr_var
					, dev_arr_var->getType()));

			arr_sub_expr->setRHS(
				CreateDeclRefExpr(
					ast_ctx
					, flat_idx_expr_var
					, flat_idx_expr_var->getType()));
		}

		/// Creates a subtract expression based on two variable decls
		clang::Expr* CreateSubtractExpr(
			clang::ASTContext& ast_ctx
			, clang::VarDecl* lhs
			, clang::VarDecl* rhs)
		{
			return
				CreateBinOp(
					ast_ctx
					, CreateDeclRefExpr(ast_ctx, lhs, lhs->getType())
					, CreateDeclRefExpr(ast_ctx, rhs, rhs->getType())
					, clang::BO_Sub
					, lhs->getType());
		}

	} /// Anonymous namespace


	KernelDefGen::KernelDefGen(
		clang::ASTContext& ast_ctx
		, NestAnalysisDriver& analysis_driver
		, InvocationStmtsGen& invoc_stmts_gen
		, const std::string& kernel_name)

		: m_ast_ctx(ast_ctx)
		, m_analysis_driver(analysis_driver)
		, m_invoc_stmts_gen(invoc_stmts_gen)
		, m_kernel_name(kernel_name)
		, m_transform(m_analysis_driver.GetTransformation())
		, m_seq_nest_innermost_loop(nullptr)
	{
		m_kernel_decl_def[KERNEL_DECL] = m_kernel_decl_def[KERNEL_DEF] = nullptr;
		m_kernel_decl_stmts[KERNEL_DECL] = m_kernel_decl_stmts[KERNEL_DEF] = nullptr;

		/// Run the generator
		RunGenerator();
	}

	clang::Expr* KernelDefGen::GetThreadOffsetExpr(clang::FieldDecl* dim3_field) const
	{
		return
			CreateBinOp(
				m_ast_ctx
				, CreateBinOp(
					m_ast_ctx
					, CreateMemberExpr(
						m_ast_ctx
						, CreateDeclRefExpr(m_ast_ctx, m_cuda_vars[BLOCK_IDX], m_cuda_vars[BLOCK_IDX]->getType())
						, false
						, dim3_field)

					, CreateMemberExpr(
						m_ast_ctx
						, CreateDeclRefExpr(m_ast_ctx, m_cuda_vars[BLOCK_DIM], m_cuda_vars[BLOCK_DIM]->getType())
						, false
						, dim3_field)

					, clang::BO_Mul
					, dim3_field->getType())

				, CreateMemberExpr(
					m_ast_ctx
					, CreateDeclRefExpr(m_ast_ctx, m_cuda_vars[THREAD_IDX], m_cuda_vars[THREAD_IDX]->getType())
					, false
					, dim3_field)

				, clang::BO_Add
				, dim3_field->getType());
	}

	KernelDefGen::IdxVarInfo KernelDefGen::CreateIdxVarInfo(
		clang::VarDecl* host_idx_var
		, clang::VarDecl* dev_idx_var
		, std::size_t loop_idx) const
	{
		auto & nest = m_analysis_driver.GetTransformedNest();
		auto low_bnd_var =
			CreateVarDecl(
				m_ast_ctx
				, host_idx_var->getName().str() + "_low_bnd"
				, host_idx_var->getType()
				, GetLowerBndExpr(nest, loop_idx));

		auto upp_bnd_var =
			CreateVarDecl(
				m_ast_ctx
				, host_idx_var->getName().str() + "_upp_bnd"
				, host_idx_var->getType()
				, GetUpperBndExpr(nest, loop_idx));

		return
			IdxVarInfo(
				host_idx_var
				, dev_idx_var
				, low_bnd_var
				, upp_bnd_var);
	}

	KernelDefGen::FlatArrSubIdx KernelDefGen::GetFlatArrSubIndex(
		clang::ArraySubscriptExpr* arr_sub_expr)
	{
		std::vector<clang::Expr*> sub_idx_exprs;
		auto hst_arr_var = ExtractIdxExprsFromArrSub(m_ast_ctx, arr_sub_expr, sub_idx_exprs);
		auto & nest = m_analysis_driver.GetTransformedNest();
		std::size_t seq_nest_depth = GetSeqInnerNestDepth();
		clang::Expr* flat_expr = nullptr;
		auto dev_arr_pair = m_host_dev_arr_map.find(hst_arr_var);
		assert(dev_arr_pair != m_host_dev_arr_map.end() && "this is a bug");

		for (std::size_t i = 0; i < sub_idx_exprs.size() - 1; ++i)
		{
			auto mul_op = CreateBinOp(
				m_ast_ctx
				, sub_idx_exprs[i]
				, CreateDeclRefExpr(
					m_ast_ctx
					, dev_arr_pair->second.GetSizeVar(i + 1)
					, dev_arr_pair->second.GetSizeVar(i + 1)->getType())

				, clang::BO_Mul
				, sub_idx_exprs[i]->getType());

			for (std::size_t j = i + 2; j < sub_idx_exprs.size(); ++j)
				mul_op = CreateBinOp(
					m_ast_ctx
					, mul_op
					, CreateDeclRefExpr(
						m_ast_ctx
						, dev_arr_pair->second.GetSizeVar(j)
						, dev_arr_pair->second.GetSizeVar(j)->getType())

					, clang::BO_Mul
					, mul_op->getType());

			if (flat_expr == nullptr)
				flat_expr = mul_op;
			else
				flat_expr = CreateBinOp(
					m_ast_ctx
					, flat_expr
					, mul_op
					, clang::BO_Add
					, mul_op->getType());
		}

		flat_expr =
			flat_expr != nullptr
			? CreateBinOp(m_ast_ctx, flat_expr, sub_idx_exprs.back(), clang::BO_Add, flat_expr->getType())
			: sub_idx_exprs.back();

		return
		{
			hst_arr_var
			, dev_arr_pair->second.DevArrVar()
			, CreateVarDecl(
				m_ast_ctx
				, GetNextArraySubIdxVarName(hst_arr_var)
				, m_ast_ctx.LongLongTy
				, flat_expr)

			, seq_nest_depth == 0
			  || seq_nest_depth == 1 && !FlatArrIdxExprRefsIdxVar(flat_expr, nest.GetNestIdxVec().back())
		};
	}

	std::size_t KernelDefGen::GetSeqInnerNestDepth() const
	{
		auto & nest = m_analysis_driver.GetTransformedNest();
		auto & transform = m_analysis_driver.GetTransformation();
		return
			transform.GetTransformType() == Transformation::TRANSFORM_INNER_PAR
			? 0
			: nest.GetNestDepth() - transform.GetDepFreeLoopsCnt();
	}

	std::string KernelDefGen::GetNextArraySubIdxVarName(clang::VarDecl* hst_arr_var)
	{
		assert(hst_arr_var != nullptr);
		auto arr_sub_cnt_pair = m_arr_sub_count_map.find(hst_arr_var);
		std::size_t curr_count;
		if (arr_sub_cnt_pair == m_arr_sub_count_map.end())
		{
			curr_count = 0;
			m_arr_sub_count_map.insert(std::make_pair(hst_arr_var, curr_count + 1));
		}
		else
		{
			curr_count = arr_sub_cnt_pair->second;
			++arr_sub_cnt_pair->second;
		}
		auto dev_arr_pair = m_host_dev_arr_map.find(hst_arr_var);
		assert(dev_arr_pair != m_host_dev_arr_map.end());

		return
			dev_arr_pair->second.DevArrVar()->getName().str()
			+ "_subidx_"
			+ std::to_string(curr_count);
	}

	KernelDefGen::IdxVarInfo* KernelDefGen::FindIdxVarInfo(clang::VarDecl* host_idx_var)
	{
		auto result = std::find_if(
			m_idx_vars_info.begin()
			, m_idx_vars_info.end()
			,
			[&host_idx_var](IdxVarInfo& idx_var_info)
		{
			return idx_var_info.HstIdxVar() == host_idx_var;
		});

		return
			result != m_idx_vars_info.end()
			? &*result
			: nullptr;
	}

	void KernelDefGen::TransformArraySubExprs()
	{
		auto & assign_stmts = m_analysis_driver.GetTransformedNest().GetAssignStmts();
		for (auto & assgn_stmt : assign_stmts)
		{
			if (auto lhs_arr_sub = assgn_stmt.GetLhs())
			{
				auto arr_sub_expr = lhs_arr_sub->GetArrSubExpr();
				auto flat_idx_pair = m_flat_arr_sub_idx_map.find(arr_sub_expr);
				assert(flat_idx_pair != m_flat_arr_sub_idx_map.end());
				TransformArrSubExpr(
					m_ast_ctx, arr_sub_expr
					, flat_idx_pair->second.DevArrVar()
					, flat_idx_pair->second.FlatArrIdxVar());
			}

			for (auto & rhs_arr_sub : assgn_stmt.GetRhs())
			{
				auto arr_sub_expr = rhs_arr_sub.GetArrSubExpr();
				auto flat_idx_pair = m_flat_arr_sub_idx_map.find(arr_sub_expr);
				assert(flat_idx_pair != m_flat_arr_sub_idx_map.end());
				TransformArrSubExpr(
					m_ast_ctx, arr_sub_expr
					, flat_idx_pair->second.DevArrVar()
					, flat_idx_pair->second.FlatArrIdxVar());
			}
		}
	}

	void KernelDefGen::GenKernelHdr()
	{
		auto & nest = m_analysis_driver.GetTransformedNest();
		auto transform_type = m_transform.GetTransformType();
		auto & nest_idx_vec = m_analysis_driver.GetNewIdxVec();
		auto & host_var_arr_info_map = m_invoc_stmts_gen.GetHostVarArrDeclInfoMap();
		std::vector<clang::ParmVarDecl*> kernel_params;

		for (std::size_t i = KERNEL_DECL; i < KERNEL_ENUM_SIZE; ++i)
			m_kernel_decl_def[i] =
			CreateFunDeclNoParams(
				m_ast_ctx
				, m_kernel_name
				, m_ast_ctx.VoidTy
				, GetKernelParamTypes(m_ast_ctx, host_var_arr_info_map, transform_type, nest_idx_vec));

		if (transform_type == Transformation::TRANSFORM_INNER_PAR)
		{
			auto host_idx_var = nest_idx_vec[0];
			const std::string& var_name = host_idx_var->getName().str();
			auto outermost_dev_idx_var =
				CreateParmVarDecl(
					m_ast_ctx
					, m_kernel_decl_def[KERNEL_DEF]
					, var_name
					, host_idx_var->getType());

			m_idx_vars_info.push_back(
				IdxVarInfo(
					host_idx_var
					, outermost_dev_idx_var
					, nullptr
					, nullptr));

			kernel_params.push_back(outermost_dev_idx_var);
		}
		for (auto arr_info_pair : host_var_arr_info_map)
		{
			std::string array_name = arr_info_pair.first->getName().str();
			auto param_decl = CreateParmVarDecl(
				m_ast_ctx
				, m_kernel_decl_def[KERNEL_DEF]
				, array_name
				, m_ast_ctx.getPointerType(m_ast_ctx.getBaseElementType(arr_info_pair.first->getType())));

			auto host_dev_arr_pair = std::make_pair(arr_info_pair.first, DevArray(param_decl));
			kernel_params.push_back(param_decl);
			auto & array_id = arr_info_pair.second.ArrayId();
			for (std::size_t i = 0; i < array_id.GetDimensionality(); ++i)
			{
				auto size_parm = CreateParmVarDecl(
					m_ast_ctx
					, m_kernel_decl_def[KERNEL_DEF]
					, array_name + "_size_" + std::to_string(i)
					, m_ast_ctx.getSizeType());

				kernel_params.push_back(size_parm);
				host_dev_arr_pair.second.PushSizeVar(size_parm);
			}
			m_host_dev_arr_map.insert(host_dev_arr_pair);
		}

		m_kernel_decl_def[KERNEL_DEF]->setParams(kernel_params);
		m_kernel_decl_def[KERNEL_DECL]->setParams(kernel_params);
	}

	void KernelDefGen::GenCudaVarDecls()
	{
		auto dim3_struct_decl = NoLoadLookup<clang::RecordDecl>(
			m_ast_ctx
			, *m_ast_ctx.getTranslationUnitDecl()
			, "dim3");

		assert(dim3_struct_decl != nullptr);
		std::vector<std::string> cuda_var_names = { "blockIdx", "blockDim", "threadIdx" };
		assert(cuda_var_names.size() == CUDA_VARS_COUNT);
		for (std::size_t i = 0; i < CUDA_VARS_COUNT; ++i)
			m_cuda_vars[i] = CreateVarDecl(
				m_ast_ctx
				, cuda_var_names[i]
				, m_ast_ctx.getRecordType(dim3_struct_decl));

		std::vector<std::string> dim3_field_names = { "x", "y", "z" };
		assert(dim3_field_names.size() == DIM3_FIELDS_COUNT);
		for (std::size_t i = 0; i < DIM3_FIELDS_COUNT; ++i)
		{
			auto dim3_field = NoLoadLookup<clang::FieldDecl>(
				m_ast_ctx
				, *dim3_struct_decl
				, dim3_field_names[i]);

			assert(dim3_field != nullptr);
			m_dim3_fields[i] = dim3_field;
		}
	}

	void KernelDefGen::GenDevIdxAndBoundVars()
	{
		std::size_t loop_idx =
			m_transform.GetTransformType() == Transformation::TRANSFORM_INNER_PAR
			? 1
			: 0;

		auto & nest_idx_vec = m_analysis_driver.GetNewIdxVec();
		std::size_t doall_nest_depth = m_transform.GetDepFreeLoopsCnt();
		assert(doall_nest_depth > 0);
		std::size_t nest_depth = m_analysis_driver.GetTransformedNest().GetNestDepth();
		std::size_t dim3_field_idx = doall_nest_depth - 1;

		for (std::size_t i = 0; loop_idx < nest_depth; ++i)
		{
			clang::VarDecl* dev_idx_var = CreateVarDecl(
				m_ast_ctx
				, nest_idx_vec[loop_idx]->getName().str()
				, nest_idx_vec[loop_idx]->getType());

			IdxVarInfo idx_var_info = CreateIdxVarInfo(nest_idx_vec[loop_idx], dev_idx_var, loop_idx);
			m_idx_vars_info.push_back(idx_var_info);

			if (i < doall_nest_depth)
				/// Note how the thread based index is translated by the value of the corresponding's
				/// loop lower boundary
				dev_idx_var->setInit(
					CreateBinOp(
						m_ast_ctx
						, GetThreadOffsetExpr(m_dim3_fields[dim3_field_idx])
						, CreateDeclRefExpr(m_ast_ctx, idx_var_info.LowBndVar(), idx_var_info.LowBndVar()->getType())
						, clang::BO_Add
						, m_dim3_fields[dim3_field_idx]->getType()));

			--dim3_field_idx;
			++loop_idx;
		}
	}

	void KernelDefGen::GenParSubnestIdxVarAndBndDeclStmts()
	{
		std::size_t loop_idx =
			m_transform.GetTransformType() == Transformation::TRANSFORM_INNER_PAR
			? 1
			: 0;

		std::size_t doall_nest_depth = m_analysis_driver.GetTransformation().GetDepFreeLoopsCnt();
		for (std::size_t i = 0; i < doall_nest_depth; ++i)
		{
			IdxVarInfo idx_var_info = m_idx_vars_info[loop_idx];

			/// First add lower and upper bound decl stmts
			m_body_stmts.push_back(
				CreateDeclStmt(
					m_ast_ctx
					, std::vector<clang::VarDecl*>({
						idx_var_info.LowBndVar()
						, idx_var_info.UppBndVar()
			})));

			/// Then, add the device index variable decl stmt
			m_body_stmts.push_back(CreateDeclStmt(m_ast_ctx, std::vector<clang::VarDecl*>({ idx_var_info.DevIdxVar() })));
			++loop_idx;
		}
	}

	void KernelDefGen::GenUpperBndsCheckStmt()
	{
		std::size_t loop_idx =
			m_transform.GetTransformType() == Transformation::TRANSFORM_INNER_PAR
			? 1
			: 0;

		clang::Expr* cond_expr = CreateBinOp(
			m_ast_ctx
			, CreateDeclRefExpr(m_ast_ctx, m_idx_vars_info[0].DevIdxVar(), m_idx_vars_info[0].DevIdxVar()->getType())
			, CreateDeclRefExpr(m_ast_ctx, m_idx_vars_info[0].UppBndVar(), m_idx_vars_info[0].UppBndVar()->getType())
			, clang::BO_GE
			, m_ast_ctx.BoolTy);

		++loop_idx;
		std::size_t doall_nest_depth = m_analysis_driver.GetTransformation().GetDepFreeLoopsCnt();
		for (std::size_t i = 1; i < doall_nest_depth; ++i)
		{
			IdxVarInfo idx_var_info = m_idx_vars_info[loop_idx];
			cond_expr = CreateBinOp(
				m_ast_ctx
				, cond_expr
				, CreateBinOp(
					m_ast_ctx
					, CreateDeclRefExpr(m_ast_ctx, idx_var_info.DevIdxVar(), idx_var_info.DevIdxVar()->getType())
					, CreateDeclRefExpr(m_ast_ctx, idx_var_info.UppBndVar(), idx_var_info.UppBndVar()->getType())
					, clang::BO_GE
					, m_ast_ctx.BoolTy)

				, clang::BO_LOr
				, m_ast_ctx.BoolTy);

			++loop_idx;
		}

		m_body_stmts.push_back(
			CreateIfStmt(
				m_ast_ctx
				, cond_expr
				, CreateReturnStmt(m_ast_ctx)));
	}

	void KernelDefGen::GenFlatArraySubIndices()
	{
		auto & assign_stmts = m_analysis_driver.GetTransformedNest().GetAssignStmts();
		for (auto & assgn_stmt : assign_stmts)
		{
			if (auto lhs_arr_sub = assgn_stmt.GetLhs())
				m_flat_arr_sub_idx_map.insert(
					std::make_pair(lhs_arr_sub->GetArrSubExpr(), GetFlatArrSubIndex(lhs_arr_sub->GetArrSubExpr())));

			for (auto & rhs_arr_sub : assgn_stmt.GetRhs())
				m_flat_arr_sub_idx_map.insert(
					std::make_pair(rhs_arr_sub.GetArrSubExpr(), GetFlatArrSubIndex(rhs_arr_sub.GetArrSubExpr())));
		}
	}

	void KernelDefGen::GenDeclStmtsForIndependentFlatArrIdxExprs()
	{
		std::vector<clang::VarDecl*> indep_flat_idx_vars;
		indep_flat_idx_vars.reserve(m_flat_arr_sub_idx_map.size());
		for (auto flat_idx_expr_pair : m_flat_arr_sub_idx_map)
			if (flat_idx_expr_pair.second.CanDefineOutOfSeqNest())
				indep_flat_idx_vars.push_back(flat_idx_expr_pair.second.FlatArrIdxVar());

		m_body_stmts.push_back(
			CreateDeclStmt(m_ast_ctx, indep_flat_idx_vars));
	}

	void KernelDefGen::GenSeqSubnestIdxVarDeclStmts()
	{
		std::size_t seq_nest_depth = GetSeqInnerNestDepth();
		if (seq_nest_depth == 0)
			return;

		m_seq_subnest_idx_vec.reserve(seq_nest_depth);
		std::size_t nest_depth = m_analysis_driver.GetTransformedNest().GetNestDepth();
		std::size_t i = nest_depth - seq_nest_depth;
		for (; i < nest_depth; ++i)
			m_seq_subnest_idx_vec.push_back(m_idx_vars_info[i].DevIdxVar());

		m_body_stmts.push_back(
			CreateDeclStmt(m_ast_ctx, m_seq_subnest_idx_vec));
	}

	void KernelDefGen::GenSeqSubnest()
	{
		if (m_seq_subnest_idx_vec.empty())
			return;

		std::size_t nest_depth = m_analysis_driver.GetTransformedNest().GetNestDepth();
		std::size_t i = nest_depth - m_seq_subnest_idx_vec.size();
		clang::ForStmt* outermost_loop = nullptr;
		clang::ForStmt* curr_loop = nullptr;

		for (; i < nest_depth; ++i)
		{
			IdxVarInfo idx_var_info = m_idx_vars_info[i];
			auto bnds_decl_stmt = CreateDeclStmt(
				m_ast_ctx
				, std::vector<clang::VarDecl*>(
			{
				idx_var_info.LowBndVar()
				, idx_var_info.UppBndVar()
			}));

			auto tmp_for_stmt =
				CreateForStmtHelper(
					m_ast_ctx
					, idx_var_info.DevIdxVar()
					, idx_var_info.LowBndVar()
					, idx_var_info.UppBndVar());

			if (curr_loop == nullptr)
			{
				m_body_stmts.push_back(bnds_decl_stmt);
				outermost_loop = curr_loop = tmp_for_stmt;
			}
			else
			{
				curr_loop->setBody(
					CreateCmpndStmt(
						m_ast_ctx
						, { bnds_decl_stmt, tmp_for_stmt }));

				curr_loop = tmp_for_stmt;
			}
		}

		m_seq_nest_innermost_loop = curr_loop;
		m_body_stmts.push_back(outermost_loop);
	}

	void KernelDefGen::GenKernelDeclStmts()
	{
		m_kernel_decl_stmts[KERNEL_DECL] =
			CreateDeclStmt(m_ast_ctx, std::vector<clang::FunctionDecl*>({ m_kernel_decl_def[KERNEL_DECL] }));

		m_kernel_decl_stmts[KERNEL_DEF] =
			CreateDeclStmt(m_ast_ctx, std::vector<clang::FunctionDecl*>({ m_kernel_decl_def[KERNEL_DEF] }));
	}

	void KernelDefGen::CompleteKernelDefinition()
	{
		TransformArraySubExprs();
		std::vector<clang::Stmt*> stmts;
		if (m_seq_nest_innermost_loop != nullptr)
		{
			std::vector<clang::VarDecl*> inner_nest_flat_idx_vars;
			for (auto flat_idx_pair : m_flat_arr_sub_idx_map)
				if (!flat_idx_pair.second.CanDefineOutOfSeqNest())
					inner_nest_flat_idx_vars.push_back(flat_idx_pair.second.FlatArrIdxVar());

			if (!inner_nest_flat_idx_vars.empty())
				stmts.push_back(CreateDeclStmt(m_ast_ctx, inner_nest_flat_idx_vars));
		}

		auto & assign_stmts = m_analysis_driver.GetTransformedNest().GetAssignStmts();
		for (auto & assgn_stmt : assign_stmts)
			stmts.push_back(assgn_stmt.GetStmt());

		if (m_seq_nest_innermost_loop != nullptr)
			m_seq_nest_innermost_loop->setBody(
				CreateCmpndStmt(m_ast_ctx, stmts));
		else
			m_body_stmts.insert(m_body_stmts.end(), stmts.begin(), stmts.end());

		/// Finally, set the kernel body
		m_kernel_decl_def[KERNEL_DEF]->setBody(
			CreateCmpndStmt(m_ast_ctx, m_body_stmts));
	}

	void KernelDefGen::RunGenerator()
	{
		GenKernelHdr();
		GenCudaVarDecls();
		GenDevIdxAndBoundVars();
		GenParSubnestIdxVarAndBndDeclStmts();
		GenUpperBndsCheckStmt();
		GenFlatArraySubIndices();
		GenDeclStmtsForIndependentFlatArrIdxExprs();
		GenSeqSubnestIdxVarDeclStmts();
		GenSeqSubnest();
		GenKernelDeclStmts();
		CompleteKernelDefinition();
	}

	clang::DeclStmt* KernelDefGen::GetKernelDecl()
	{
		return m_kernel_decl_stmts[KERNEL_DECL];
	}

	clang::DeclStmt* KernelDefGen::GetKernelDef()
	{
		return m_kernel_decl_stmts[KERNEL_DEF];
	}

} /// namespace gap