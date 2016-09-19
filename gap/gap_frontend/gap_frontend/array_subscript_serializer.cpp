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

#include "array_subscript_serializer.h"
#include "linear_expr_serializer.h"
#include "ast_helpers.h"
#include <exception>
#include <cassert>


namespace gap
{
	namespace
	{

		clang::DeclRefExpr* Deserialize(
			clang::ASTContext& ast_ctx
			, clang::ArraySubscriptExpr* arr_sub_expr
			, std::vector<LinearExpr>& lin_idx_exprs)
		{
			assert(arr_sub_expr != nullptr);
			clang::DeclRefExpr* array_der = nullptr;
			auto arr_sub_base = arr_sub_expr->getBase()->IgnoreCasts();

			if (!clang::isa<clang::DeclRefExpr>(arr_sub_base))
				array_der = Deserialize(
					ast_ctx
					, clang::dyn_cast<clang::ArraySubscriptExpr>(arr_sub_base)
					, lin_idx_exprs);
			else
				array_der = clang::cast<clang::DeclRefExpr>(arr_sub_base);

			lin_idx_exprs.push_back(LinearExprSerializer::Deserialize(
				ast_ctx
				, arr_sub_expr->getIdx()));

			return array_der;
		}

	} /// Anonymous namespace


	clang::ArraySubscriptExpr* ArrSubscriptSerializer::Serialize(const ArraySubscript& arr_subscript)
	{
		/// FIXME: implement serialization
		return nullptr;
	}

	std::unique_ptr<ArraySubscript> ArrSubscriptSerializer::Deserialize(
		clang::ASTContext& ast_ctx
		, clang::Expr* arr_access_expr)
	{
		clang::Expr* expr_without_casts = arr_access_expr->IgnoreCasts();
		std::unique_ptr<ArraySubscript> retval;
		if (auto arr_sub_expr = clang::dyn_cast<clang::ArraySubscriptExpr>(expr_without_casts))
		{
			std::vector<LinearExpr> lin_idx_exprs;
			clang::DeclRefExpr* array_der = gap::Deserialize(
				ast_ctx
				, arr_sub_expr
				, lin_idx_exprs);

			assert(array_der != nullptr && "array subscript expression should always terminate with a DER");
			retval = std::make_unique<ArraySubscript>(
				GetVarDecl(array_der)
				, std::move(lin_idx_exprs)
				, arr_sub_expr);
		}
		else if (auto decl_ref_expr = clang::dyn_cast<clang::DeclRefExpr>(arr_access_expr))
		{
			clang::QualType decl_type = decl_ref_expr->getDecl()->getType();
			retval =
				!decl_type->isArrayType() && !decl_type->isPointerType()
				? std::unique_ptr<ArraySubscript>()
				: std::make_unique<ArraySubscript>(
					GetVarDecl(decl_ref_expr)
					, nullptr);
		}
		return retval;
	}

} /// namespace gap