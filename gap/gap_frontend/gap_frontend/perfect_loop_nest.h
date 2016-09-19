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

#ifndef GAP_FRONTEND_PERFECT_LOOP_NEST_H
#define GAP_FRONTEND_PERFECT_LOOP_NEST_H

#include "for_loop_header.h"
#include "assign_stmt.h"
#include <vector>
#include <memory>

namespace gap
{

	/// Forward declaration
	class SymTblTree;

	/// Represents a perfect loop nest as required by the data dependnece analyzer.
	/// The loop nest records the outermost for loop header as well as any assign.
	/// stmts having an array subscript expression on LHS or RHS (or both). Note
	/// that this class provides means to build AssignStmt from the binary assign
	/// operator enforcing the following constraints:
	/// 1) scalar variable defined outside the loop nest MUST NOT appear as LHS
	/// of the assignment stmt
	/// 2) each array subscript must be a linear combination of the index variables
	/// of enclosing loops (other variables may not appear as part of subscript)
	/// 3) array subscript expressions whose resulting type is different than the
	/// array elem type are not allowed. I.e. for int a[5][5] only subscript of form
	/// a[i][j] may appear in a parallelizable perfect loop nest. Subscript a[i] is
	/// illegal as this results in an access to a entire array dimension and data
	/// depend. analyzer cannot properly handle this scenario
	///
	/// FIXMEs AND IMPROVEMENTS:
	/// 1) currently only assignment stmts can appear in a parallelizable loop nest
	/// so this class assumes that LHS of each of these must be an array subscript.
	/// Once declarations of vars are allowed within the nest, this will have to
	/// changed to check if LHS scalar variable is defined within (does not prevent
	/// the parallelization) or outside the loop nest (prevents parallelization)
	class PerfectLoopNest
	{
	public:

		PerfectLoopNest(
			clang::ASTContext& ast_ctx
			, const SymTblTree& sym_tbl_tree
			, clang::ForStmt& outermost_loop);

		/// Modifiers
		void PushLoop(const clang::ForStmt& parent_loop, clang::ForStmt& new_loop);
		void PushAssignStmt(const clang::ForStmt& enclosing_loop, clang::BinaryOperator& assign_op);

		/// Accessors
		std::size_t GetNestDepth() const;
		ForLoopHeader& GetOutermostLoopHdr();
		ForLoopHeader* GetLoopHdr(std::size_t level);
		const ForLoopHeader& GetOutermostLoopHdr() const;
		const ForLoopHeader* GetLoopHdr(std::size_t level) const;
		std::vector<AssignStmt>& GetAssignStmts();
		const std::vector<AssignStmt>& GetAssignStmts() const;
		std::vector<clang::VarDecl*>& GetNestIdxVec();
		const std::vector<clang::VarDecl*>& GetNestIdxVec() const;
		std::vector<clang::VarDecl*>& GetNestInputVars();
		std::vector<clang::VarDecl*>& GetNestOutputVars();

		/// Dump
		void Dump() const;

	private:

		clang::ASTContext& m_ast_ctx;
		const SymTblTree& m_sym_tbl_tree;
		ForLoopHeader m_outermost_loop_hdr;
		std::vector<AssignStmt> m_assign_stmts;
		std::vector<clang::VarDecl*> m_nest_input_vars;
		std::vector<clang::VarDecl*> m_nest_output_vars;
		std::vector<clang::VarDecl*> m_nest_idx_vec;
		std::size_t m_nest_depth;
	};

} /// namespace gap

#endif /// PERFECT_LOOP_NEST_H
