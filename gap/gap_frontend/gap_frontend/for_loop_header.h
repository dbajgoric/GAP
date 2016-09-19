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

#ifndef GAP_FRONTEND_FOR_LOOP_HEADER_H
#define GAP_FRONTEND_FOR_LOOP_HEADER_H

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include "linear_expr.h"
#include <memory>

namespace gap
{

	/// Represents for loop header including the loop index identifier, lower
	/// and upper boundary and increment expression. The class validates the
	/// data according to internal compiler rules:
	/// 1) the lower and upper bounds of the outermost loop has to be a compile
	/// time integer constant - an integer literal, a macro or a constant int
	/// variable initialized with an int literal within current function
	/// 2) the lower and upper bounds of the any inner loop must be a linear
	/// function of index variables of the enclosing loops. The coefficients and
	/// a constant can be an integer literal or a constant int variable with
	/// a known integer value
	/// 3) LHS of init, cond an inc loop header in any of the loops in the nest
	/// must be a declaration reference expression (DRE) representing an integer
	/// variable that is NOT already used as an index variable of one of outer
	/// loops
	/// 4) only loops with unit positive loop strides are supported. Following
	/// expressions are allowed for inc part: ++i, i++, i += 1, i = i + 1 and
	/// i = 1 + i, where 'i' is the index variable of the given loop
	/// 5) Currently, the only supported operator in the cond expression is the
	/// '<' (lower then) operator
	class ForLoopHeader
	{
		typedef std::unique_ptr<ForLoopHeader> ForLoopHdrPtr;

	public:

		ForLoopHeader(
			clang::ASTContext& ast_ctx
			, clang::ForStmt& for_stmt
			, ForLoopHeader* parent_loop);

		/// Modifiers
		void SetChild(ForLoopHdrPtr child_loop_hdr);

		/// Accessors
		clang::VarDecl& GetIdxVar();
		const clang::VarDecl& GetIdxVar() const;
		const LinearExpr& GetLowerBound() const;
		const LinearExpr& GetUpperBound() const;
		ForLoopHeader* GetParent();
		ForLoopHeader* GetChild();
		clang::ForStmt* GetStmt();
		const ForLoopHeader* GetParent() const;
		const ForLoopHeader* GetChild() const;
		const clang::ForStmt* GetStmt() const;
		void GetOuterLoopHeaders(std::vector<const ForLoopHeader*>& outer_loop_headers) const;

		/// Dump
		void Dump() const;

	private:

		clang::ASTContext& m_ast_ctx;
		ForLoopHeader* m_parent;
		ForLoopHdrPtr m_child;
		clang::ForStmt* m_for_stmt;

		clang::VarDecl* m_idx_var;
		LinearExpr m_lower_bound;
		LinearExpr m_upper_bound;
	};


	/// For a given linear expression this function checks if all its variables
	/// are index variables of enclosing loops. Loop bounds as well as linear
	/// exprs in array subscripts must satisfy this constraint
	bool UnknownsAreIdxVarsOfEnclosingLoops(
		const LinearExpr& linear_expr
		, const std::vector<const ForLoopHeader*>& enclosing_loops);

} /// namespace gap

#endif /// GAP_FRONTEND_FOR_LOOP_HEADER_H
