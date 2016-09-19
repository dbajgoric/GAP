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

#ifndef GAP_FRONTEND_ARRAY_SUBSCRIPT_H
#define GAP_FRONTEND_ARRAY_SUBSCRIPT_H

#include "linear_expr.h"
#include <clang/AST/Expr.h>
#include <vector>

namespace gap
{

	/// Represents an array subscript expression a[]...[] where each index
	/// is a linear function represented by LinearExpr instance. Note that
	/// this class doesn't enforce any rules on the variables appearing as
	/// part of LinearExpr instances (i.e. ensuring that each array index
	/// expr is a linear combination of index variables enclosing loops)
	class ArraySubscript
	{
	public:

		explicit ArraySubscript(
			clang::VarDecl* array_decl
			, clang::ArraySubscriptExpr* arr_sub_expr);

		ArraySubscript(
			clang::VarDecl* array_decl
			, const std::vector<LinearExpr>& lin_exprs
			, clang::ArraySubscriptExpr* arr_sub_expr);

		ArraySubscript(
			clang::VarDecl* array_decl
			, std::vector<LinearExpr>&& lin_exprs
			, clang::ArraySubscriptExpr* arr_sub_expr);

		/// Modifiers
		void PushLinearExpr(const LinearExpr& lin_expr);
		void PushLinearExpr(LinearExpr&& lin_expr);
		void SetLinearExpr(const LinearExpr& lin_expr, std::size_t dim);
		void SetLinearExpr(LinearExpr&& lin_expr, std::size_t dim);

		/// Accessors
		const clang::VarDecl* GetArrDecl() const;
		const LinearExpr& GetLinearExpr(std::size_t dim) const;
		clang::VarDecl* GetArrDecl();
		LinearExpr& GetLinearExpr(std::size_t dim);
		std::size_t GetDimensionality() const;
		clang::ArraySubscriptExpr* GetArrSubExpr();

		/// Dump
		void Dump() const;

	private:

		clang::VarDecl* m_array_decl;
		std::vector<LinearExpr> m_array_subscript;
		clang::ArraySubscriptExpr* m_arr_sub_expr;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_ARRAY_SUBSCRIPT_H
