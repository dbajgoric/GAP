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

#include "linear_expr_serializer.h"
#include <exception>
#include <cassert>
#include "ast_helpers.h"


namespace gap
{
	namespace
	{

		bool TryEvaluateAsInt(
			clang::ASTContext& ast_ctx
			, const clang::Expr* expr
			, llvm::APSInt& int_val)
		{
			if (expr->isEvaluatable(ast_ctx))
				return expr->EvaluateAsInt(int_val, ast_ctx);

			return false;
		}

		llvm::APSInt ValueAndSignToInt(const llvm::APSInt& int_val, bool is_neg_sign)
		{
			return
				is_neg_sign
				? GetSigned(int_val.getBitWidth(), -1) * int_val
				: int_val;
		}

		bool IsNegSign(clang::BinaryOperatorKind bin_opkind)
		{
			return bin_opkind == clang::BO_Sub;
		}

		void Deserialize(
			clang::ASTContext& ast_ctx
			, LinearExpr& lin_expr
			, const clang::Expr* expr
			, clang::BinaryOperatorKind parent_opkind)
		{
			assert(parent_opkind == clang::BO_Add || parent_opkind == clang::BO_Sub);
			llvm::APSInt int_val;
			bool is_neg_sign(IsNegSign(parent_opkind));
			if (TryEvaluateAsInt(ast_ctx, expr, int_val))
			{
				/// This is a free constant
				lin_expr.AddToConstant(ValueAndSignToInt(int_val, is_neg_sign));
				return;
			}
			else if (auto decl_ref_expr = clang::dyn_cast<clang::DeclRefExpr>(expr))
			{
				/// This is a variable with +1 or -1 multiplier
				lin_expr.InsertOrAddIfKnownVar(
					GetVarDecl(decl_ref_expr)
					, ValueAndSignToInt(llvm::APSInt::get(1), is_neg_sign));

				return;
			}

			if (!clang::isa<clang::BinaryOperator>(expr))
				throw std::runtime_error("unable to deserialize expression as a linear expression");

			auto bin_op = clang::cast<clang::BinaryOperator>(expr);
			switch (bin_op->getOpcode())
			{
			case clang::BO_Mul:
			{
				auto lhs = bin_op->getLHS()->IgnoreCasts();
				auto rhs = bin_op->getRHS()->IgnoreCasts();
				if (TryEvaluateAsInt(ast_ctx, lhs, int_val)
					&& clang::isa<clang::DeclRefExpr>(rhs))
				{
					lin_expr.InsertOrAddIfKnownVar(
						GetVarDecl(clang::cast<clang::DeclRefExpr>(rhs))
						, ValueAndSignToInt(int_val, is_neg_sign));
				}
				else if (TryEvaluateAsInt(ast_ctx, rhs, int_val)
					&& clang::isa<clang::DeclRefExpr>(lhs))
				{
					lin_expr.InsertOrAddIfKnownVar(
						GetVarDecl(clang::cast<clang::DeclRefExpr>(lhs))
						, ValueAndSignToInt(int_val, is_neg_sign));
				}
				else
				{
					throw std::runtime_error(
						"multiplication operator must have a constant integer expression "
						"as one operand and a decl ref expr as the other operand");
				}
				break;
			}

			case clang::BO_Add:
			case clang::BO_Sub:
			{
				Deserialize(ast_ctx, lin_expr, bin_op->getLHS()->IgnoreCasts(), clang::BO_Add);
				Deserialize(ast_ctx, lin_expr, bin_op->getRHS()->IgnoreCasts(), bin_op->getOpcode());
				break;
			}

			default:
			{
				throw std::runtime_error("unable to deserialize expression as a linear expression");
				break;
			}
			}
		}

	} /// Anonymous namespace


	clang::Expr* LinearExprSerializer::Serialize(const LinearExpr& lin_expr)
	{
		/// FIXME: implement the function
		return nullptr;
	}

	LinearExpr LinearExprSerializer::Deserialize(
		clang::ASTContext& ast_ctx
		, const clang::Expr* expr)
	{
		assert(expr != nullptr && "expr must not be a nullpr");
		LinearExpr lin_expr;
		gap::Deserialize(ast_ctx, lin_expr, expr->IgnoreCasts(), clang::BO_Add);
		return lin_expr;
	}

} /// namespace gap