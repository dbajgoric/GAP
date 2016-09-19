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

#ifndef GAP_FRONTEND_ASSIGN_STMT_H
#define GAP_FRONTEND_ASSIGN_STMT_H

#include "array_subscript.h"
#include <clang/AST/Expr.h>
#include <memory>

namespace gap
{

	/// An assignment statement that registers only the array subscript expressions
	/// required for data dependence analysis. Note that this class will not enforce
	/// constaints on the assign stmts required to be able to parallelize the loop
	/// nest (such as that scalar variable defined outside the nest cannot appear
	/// as LHS of an assignment, or that array subscript expression must derefence
	/// the array all the way till its element type - for int a[5][5] the subscript
	/// expression a[i][j] is allowed but a[i] is not)
	class AssignStmt
	{
		typedef std::unique_ptr<ArraySubscript> ArrSubscriptPtr;

	public:

		AssignStmt(
			clang::ASTContext& ast_ctx
			, clang::BinaryOperator& bin_assign);

		/// Modifiers
		void SetLhs(ArrSubscriptPtr lhs);
		void PushToRhs(const ArraySubscript& arr_subscript);
		void PushToRhs(ArraySubscript&& arr_subscript);

		/// Accessors
		clang::BinaryOperator* GetStmt();
		const clang::BinaryOperator* GetStmt() const;
		const ArraySubscript* GetLhs() const;
		const std::vector<ArraySubscript>& GetRhs() const;
		ArraySubscript* GetLhs();
		std::vector<ArraySubscript>& GetRhs();

		/// Dump
		void Dump() const;

	private:

		clang::BinaryOperator* m_stmt;
		ArrSubscriptPtr m_lhs;
		std::vector<ArraySubscript> m_rhs;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_ASSIGN_STMT_H
