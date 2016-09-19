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

#ifndef GAP_FRONTEND_NEST_INFO_COLLECTOR_H
#define GAP_FRONTEND_NEST_INFO_COLLECTOR_H

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ASTContext.h>
#include "perfect_loop_nest.h"

namespace gap
{

	/// Forward declarations
	class SymTblTree;

	/// Collects all the relevant info about the loop nests found in the first
	/// parallelization pass. This class also analyses nests and discards the
	/// nests that don't satisfy any of the constraints (for instance, only
	/// perfect loop nests can be parallelized). For parallelizable nests, the
	/// loop nest representation is built including: loop nest vector (loop
	/// variable for each loop), lower and upper bounds, the assignment stmts
	/// with out-of-the nest arrays on the LHS or RHS and so on. Only assign
	/// stmts may appear in the innermost loop nest! In case any other stmts
	/// are found the nest parallelization cannot be performed.

	/// FIXES AND IMROVEMENTS:
	/// 1) Currently, no stmts are allowed in between the loops in the nest.
	/// That is, stricly perfect loop nests are supported. However, it would
	/// be possible to relax this constraint:
	/// - allow defining loop variables before the loop where it is used. Loop
	/// variables are special as they don't introduce data dependence to the
	/// code. They are completely eliminated from the parallel code by directly
	/// providing loop variable value to each iteration of the loop. This is
	/// possible because it is known, at compile time, how loop variable value
	/// gets updated (it is incremented by one in each iteration).
	/// - it should be possible to apply similar reasoning to other variables.
	/// As long as we can predict how variable changes over the course of nest's
	/// execution, it could be eliminated alltogether and replaced by a constant
	/// value for each iteration in the parallel code.
	///
	/// 2) Allow stmts other then assignments within innermost loop. Variable
	/// declarations, function calls (under certain conditions such as that array
	/// that is read or written anywhere in the nest or an index variable is not
	/// passed to it as an argument), if-elseif-else stmt etc. and other stmts
	/// can all be supported other certain restrictions
	class NestInfoCollector : public clang::RecursiveASTVisitor<NestInfoCollector>
	{
		typedef clang::RecursiveASTVisitor<NestInfoCollector> RecursiveASTVisitorBase;

	public:

		/// Attempts to construct a perfect loop nest and throws on failure
		NestInfoCollector(
			clang::ASTContext& ast_ctx
			, const SymTblTree& sym_tbl_tree
			, clang::ForStmt& outermost_loop);

		bool TraverseForStmt(clang::ForStmt* for_stmt);
		bool TraverseBinAssign(clang::BinaryOperator* assign_op);
		bool VisitStmt(clang::Stmt* stmt);

		PerfectLoopNest& GetConstructedNest();
		const PerfectLoopNest& GetConstructedNest() const;

	private:

		clang::ASTContext& m_ast_ctx;
		const SymTblTree& m_sym_tbl_tree;
		std::unique_ptr<PerfectLoopNest> m_perfect_nest;
		const clang::ForStmt* m_curr_loop;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_NEST_INFO_COLLECTOR_H
