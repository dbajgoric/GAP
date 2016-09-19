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

#ifndef GAP_FRONTEND_LINEAR_EXPR_SERIALIZER_H
#define GAP_FRONTEND_LINEAR_EXPR_SERIALIZER_H

#include "linear_expr.h"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <vector>

namespace gap
{

	/// (De)Serializes the LinearExpr object (from)to the clang::Expr instance.
	/// The builder enforces the following rules when deserializing:
	///
	/// 1) Only variables whose value is not known at compile time are the loop
	/// index variables. All other variables have to be constant integers whose
	/// init value is a compile time integral constant. Note, however, that this
	/// class cannot identify the loop index variables. It only requires that the
	/// expression be a linear combination
	/// 2) Expression has to be linear - meaning that expressions with i*j where
	/// i and j are loop index variaibles are not allowed
	/// 3) All coefficients and contants have to be integral values known at the
	/// compile time (as point 1) explains this includes constant integeral vars
	/// whose init value is a compile time constant)
	/// 4) Only expressions whose each MUL operator has simple expressions for
	/// both left and right hand side - expression is simple if it doesn't contain
	/// any operators. Furthermore, at least left or at least right hand side of
	/// the MUL operator must be a compile-time constant. For instance, expr
	/// i*2 + 2*j + 78 can be deserialized while expression 2*i*54 + 2*8*j - 3 can
	/// not. Also, parentheses and division operator may not appear as part of the
	/// expression 
	///
	/// FIXMEs AND IMPROVEMENTS
	/// 1) Deserializer currently completely ignores the unary operators such as
	/// unary minus that can affect the sign of the coefficient
	/// 2) Support other binary operators except +, - and *
	/// 3) Implement complex expression expansion that is capable of expanding a
	/// complex arithmetic expression into a canonical form where each term is a
	/// series of multiplications and terms are connected with + or - operator.
	/// For example: (((a + b)*c)+d)*e => a*c*e + b*c*e + d*e. Each of the terms
	/// can contain a single unknown and additional folding mechanism must be
	/// designed to fold multiple constants within the term into a single one
	class LinearExprSerializer
	{
	public:

		static clang::Expr* Serialize(const LinearExpr& lin_expr);
		static LinearExpr Deserialize(clang::ASTContext& ast_ctx, const clang::Expr* expr);

	private:

		LinearExprSerializer();
	};

} /// namespace gap

#endif /// GAP_FRONTEND_LINEAR_EXPR_SERIALIZER_H
