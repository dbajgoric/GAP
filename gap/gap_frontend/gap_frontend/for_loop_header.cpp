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

#include "for_loop_header.h"
#include "linear_expr_serializer.h"
#include "ast_helpers.h"
#include <iostream>
#include <algorithm>
#include <exception>


namespace gap
{
	namespace
	{

		clang::BinaryOperator* ValidateInitStmt(clang::Stmt* init_stmt, clang::VarDecl*& idx_var)
		{
			if (init_stmt == nullptr)
				throw std::runtime_error("loop without the init statement is not supported");
			if (!clang::isa<clang::BinaryOperator>(init_stmt))
				throw std::runtime_error("loop init stmt must be an assignment stmt");

			clang::BinaryOperator* init_bin_op = clang::cast<clang::BinaryOperator>(init_stmt);
			if (init_bin_op->getOpcode() != clang::BinaryOperatorKind::BO_Assign)
				throw std::runtime_error("loop init stmt must be an assignment stmt");

			if (!clang::isa<clang::DeclRefExpr>(init_bin_op->getLHS()))
				throw std::runtime_error("lhs of the init stmt in for loop has to be a variable");

			auto decl_ref_expr = clang::cast<clang::DeclRefExpr>(init_bin_op->getLHS());
			if (!decl_ref_expr->getType()->isIntegerType())
				throw std::runtime_error("loops with non-integral index variables are not supported");

			idx_var = GetVarDecl(decl_ref_expr);
			return init_bin_op;
		}

		clang::BinaryOperator* ValidateCondExpr(clang::Expr* cond_expr, const clang::VarDecl* idx_var)
		{
			if (cond_expr == nullptr)
				throw std::runtime_error("loop without the condition expression is not supported");
			if (!clang::isa<clang::BinaryOperator>(cond_expr))
				throw std::runtime_error("loop condition expression must be a binary operator");

			clang::BinaryOperator* cond_bin_op = clang::cast<clang::BinaryOperator>(cond_expr);
			clang::BinaryOperatorKind op_kind = cond_bin_op->getOpcode();
			clang::Expr* lhs_minus_casts = cond_bin_op->getLHS()->IgnoreCasts();

			if (op_kind != clang::BO_LT/*&& op_kind != clang::BO_LE*/)
				throw std::runtime_error("condition expression must be a '<' binary operator");
			if (!clang::isa<clang::DeclRefExpr>(lhs_minus_casts))
				throw std::runtime_error("loop condition expression lhs must be a declaration reference expression");

			clang::DeclRefExpr* decl_ref_expr = clang::cast<clang::DeclRefExpr>(lhs_minus_casts);
			if (idx_var != GetVarDecl(decl_ref_expr))
				throw std::runtime_error("the loop index variable has to be lhs of the loop's condition expression");

			return cond_bin_op;
		}

		void ValidateBinaryIncOp(const clang::BinaryOperator* inc_bin_op, const clang::VarDecl* idx_var)
		{
			if (!clang::isa<clang::DeclRefExpr>(inc_bin_op->getLHS()))
				throw std::runtime_error("loop increment lhs must be a declaration reference expression");

			auto decl_ref_expr = clang::cast<clang::DeclRefExpr>(inc_bin_op->getLHS());
			if (idx_var != GetVarDecl(decl_ref_expr))
				throw std::runtime_error("the loop index variable has to be lhs of the loop's increment");

			switch (inc_bin_op->getOpcode())
			{
			case clang::BO_Assign:
			{
				if (!clang::isa<clang::BinaryOperator>(inc_bin_op->getRHS()))
					throw std::runtime_error(
						"loop increment may take one of the following formats: "
						"++i, i++, i += 1, i = i + 1, i = 1 + i");

				auto inc_rhs = clang::cast<clang::BinaryOperator>(inc_bin_op->getRHS());
				clang::IntegerLiteral* loop_stride = nullptr;
				clang::DeclRefExpr* tmp_ref_expr = nullptr;

				if (clang::isa<clang::IntegerLiteral>(inc_rhs->getLHS())
					&& clang::isa<clang::DeclRefExpr>(inc_rhs->getRHS()))
				{
					loop_stride = clang::cast<clang::IntegerLiteral>(inc_rhs->getLHS());
					tmp_ref_expr = clang::cast<clang::DeclRefExpr>(inc_rhs->getRHS());
				}
				else if (clang::isa<clang::IntegerLiteral>(inc_rhs->getRHS())
					&& clang::isa<clang::DeclRefExpr>(inc_rhs->getLHS()))
				{
					loop_stride = clang::cast<clang::IntegerLiteral>(inc_rhs->getRHS());
					tmp_ref_expr = clang::cast<clang::DeclRefExpr>(inc_rhs->getLHS());
				}

				if (loop_stride->getValue() != 1)
					throw std::runtime_error("non-unit loop strides are currently not supported");

				if (!clang::isa<clang::DeclRefExpr>(inc_rhs->getRHS()))
					throw std::runtime_error(
						"loop increment may take one of the following formats: "
						"++i, i++, i += 1, i = i + 1, i = 1 + i");

				if (idx_var != GetVarDecl(tmp_ref_expr))
					throw std::runtime_error(
						"loop increment may take one of the following formats: "
						"++i, i++, i += 1, i = i + 1, i = 1 + i");
				break;
			}

			case clang::BO_AddAssign:
			{
				if (!clang::isa<clang::IntegerLiteral>(inc_bin_op->getRHS()))
					throw std::runtime_error(
						"loop increment may take one of the following formats: "
						"++i, i++, i += 1, i = i + 1, i = 1 + i");

				auto loop_stride = clang::cast<clang::IntegerLiteral>(inc_bin_op->getRHS());
				if (loop_stride->getValue() != 1)
					throw std::runtime_error("non-unit loop strides are currently not supported");

				break;
			}

			default:
			{
				throw std::runtime_error(
					"loop increment may take one of the following formats: "
					"++i, i++, i += 1, i = i + 1, i = 1 + i");
			}
			}
		}

		void ValidateUnaryIncOp(const clang::UnaryOperator* inc_unary_op, const clang::VarDecl* idx_var)
		{
			if (inc_unary_op->getOpcode() != clang::UO_PreInc
				&& inc_unary_op->getOpcode() != clang::UO_PostInc)
				throw std::runtime_error(
					"loop increment may take one of the following formats: "
					"++i, i++, i += 1, i = i + 1, i = 1 + i");

			if (auto decl_ref_expr = clang::dyn_cast<clang::DeclRefExpr>(inc_unary_op->getSubExpr()))
			{
				if (idx_var != GetVarDecl(decl_ref_expr))
					throw std::runtime_error(
						"loop increment may take one of the following formats: "
						"++i, i++, i += 1, i = i + 1, i = 1 + i");
				return;
			}

			throw std::runtime_error(
				"loop increment may take one of the following formats: "
				"++i, i++, i += 1, i = i + 1, i = 1 + i");
		}

		void ValidateIncExpr(const clang::Expr* inc_expr, const clang::VarDecl* idx_var)
		{
			if (inc_expr == nullptr)
				throw std::runtime_error("for loop without the increment expression is not supported");
			if (auto inc_bin_op = clang::dyn_cast<clang::BinaryOperator>(inc_expr))
				ValidateBinaryIncOp(inc_bin_op, idx_var);
			else if (auto inc_unary_op = clang::dyn_cast<clang::UnaryOperator>(inc_expr))
				ValidateUnaryIncOp(inc_unary_op, idx_var);
			else
				throw std::runtime_error("for loop increment expression must be either binary or unary operator");
		}

	} /// Anonymous namespace

	bool UnknownsAreIdxVarsOfEnclosingLoops(
		const LinearExpr& linear_expr
		, const std::vector<const ForLoopHeader*>& enclosing_loops)
	{
		auto & vars = linear_expr.GetVars();
		for (auto var : vars)
		{
			auto iter = std::find_if(
				enclosing_loops.begin(), enclosing_loops.end(),
				[&var](const ForLoopHeader* enclosing_loop)
			{
				return var == &enclosing_loop->GetIdxVar();
			});

			if (iter == enclosing_loops.end())
				return false;
		}
		return true;
	}

	ForLoopHeader::ForLoopHeader(
		clang::ASTContext& ast_ctx
		, clang::ForStmt& for_stmt
		, ForLoopHeader* parent)

		: m_ast_ctx(ast_ctx)
		, m_parent(parent)
		, m_child(nullptr)
		, m_for_stmt(&for_stmt)
	{
		auto init_bin_op = ValidateInitStmt(for_stmt.getInit(), m_idx_var);
		m_lower_bound = LinearExprSerializer::Deserialize(ast_ctx, init_bin_op->getRHS());
		auto cond_bin_op = ValidateCondExpr(for_stmt.getCond(), m_idx_var);
		m_upper_bound = LinearExprSerializer::Deserialize(ast_ctx, cond_bin_op->getRHS());

		/// As only '<' operator is supported in the loop's condition, the actual upper bound
		/// is upp_bnd_expr - 1
		m_upper_bound.AddToConstant(GetSigned(m_upper_bound.GetConstant().getBitWidth(), -1));
		ValidateIncExpr(for_stmt.getInc(), m_idx_var);

		if (m_parent == nullptr)
		{
			/// This is outermost loop header
			if (m_lower_bound.GetVarsCount() != 0
				|| m_upper_bound.GetVarsCount() != 0)
				throw std::runtime_error(
					"lower and upper bounds of an outermost for loop must "
					"be a constant expression");
		}
		else
		{
			std::vector<const ForLoopHeader*> outer_loop_headers;
			GetOuterLoopHeaders(outer_loop_headers);
			if (!UnknownsAreIdxVarsOfEnclosingLoops(m_lower_bound, outer_loop_headers)
				|| !UnknownsAreIdxVarsOfEnclosingLoops(m_upper_bound, outer_loop_headers))
				throw std::runtime_error(
					"lower and upper bounds of any inner for loop must be a linear "
					"function of index variables of enclosing loops in the nest");
		}
	}

	void ForLoopHeader::SetChild(ForLoopHdrPtr child_loop_hdr)
	{
		m_child = std::move(child_loop_hdr);
	}

	clang::VarDecl& ForLoopHeader::GetIdxVar()
	{
		return *m_idx_var;
	}

	const clang::VarDecl& ForLoopHeader::GetIdxVar() const
	{
		return *m_idx_var;
	}

	const LinearExpr& ForLoopHeader::GetLowerBound() const
	{
		return m_lower_bound;
	}

	const LinearExpr& ForLoopHeader::GetUpperBound() const
	{
		return m_upper_bound;
	}

	ForLoopHeader* ForLoopHeader::GetParent()
	{
		return m_parent;
	}

	const ForLoopHeader* ForLoopHeader::GetParent() const
	{
		return m_parent;
	}

	ForLoopHeader* ForLoopHeader::GetChild()
	{
		return m_child.get();
	}

	const ForLoopHeader* ForLoopHeader::GetChild() const
	{
		return m_child.get();
	}

	clang::ForStmt* ForLoopHeader::GetStmt()
	{
		return m_for_stmt;
	}

	const clang::ForStmt* ForLoopHeader::GetStmt() const
	{
		return m_for_stmt;
	}

	void ForLoopHeader::GetOuterLoopHeaders(std::vector<const ForLoopHeader*>& outer_loop_headers) const
	{
		const ForLoopHeader* parent = this;
		while (parent = parent->GetParent())
			outer_loop_headers.push_back(parent);
	}

	void ForLoopHeader::Dump() const
	{
		const std::string& idx_var_name(m_idx_var->getName().str());
		std::cout
			<< "for("
			<< idx_var_name
			<< " = ";

		m_lower_bound.Dump();
		std::cout
			<< "; "
			<< idx_var_name
			<< " < ";

		m_upper_bound.Dump();
		std::cout
			<< "; "
			<< "++"
			<< idx_var_name
			<< ")";
	}

} /// namespace gap