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

#include "perfect_loop_nest.h"
#include "sym_tbl_tree.h"
#include <algorithm>
#include <exception>
#include <cassert>


namespace gap
{
	namespace
	{

		unsigned int NUM_OF_SPACES_BTWN_LVLS = 2;

		ForLoopHeader* FindForLoopHdr(ForLoopHeader* loop_hdr, const clang::ForStmt& for_stmt)
		{
			if (loop_hdr == nullptr)
				return nullptr;
			else if (loop_hdr->GetStmt() == &for_stmt)
				return loop_hdr;

			return FindForLoopHdr(loop_hdr->GetChild(), for_stmt);
		}

		bool UnknownsInArrSubAreIdxVarsOfEnclosingLoops(
			const ArraySubscript& arr_sub
			, const std::vector<const ForLoopHeader*>& enclosing_loops)
		{
			for (std::size_t i = 0; i < arr_sub.GetDimensionality(); ++i)
				if (!UnknownsAreIdxVarsOfEnclosingLoops(arr_sub.GetLinearExpr(i), enclosing_loops))
					return false;

			return true;
		}

		/// Verifies that array subscript evaluated type is equal to array element type.
		/// Note that this is checked by comparing the number of dereferencing operations
		/// (i.e. a[i][j]) against the array dimension
		bool ArrSubEvaluatesToArrElemType(
			const ArraySubscript& arr_sub
			, const SymTblTree& sym_tbl_tree
			, const clang::ForStmt& enclosing_loop)
		{
			const Identifier* id = sym_tbl_tree.FindIdentifier(
				&enclosing_loop
				, arr_sub.GetArrDecl()->getName().str());

			assert(id != nullptr && "unexpected symbol table search failure");
			assert(id->IsArrayLikeIdentifier() && "id is expected to be an array like identifier");
			const ArrayLikeIdentifier* array_id = CastAsArrayId(id);

			return array_id->GetDimensionality() == arr_sub.GetDimensionality();
		}

		/// Loops must appear in innermost to outermost order in the vector argument
		void ValidateArrSubscript(
			const ArraySubscript& arr_sub
			, const SymTblTree& sym_tbl_tree
			, const std::vector<const ForLoopHeader*>& enclosing_loop_hdrs)
		{
			if (!UnknownsInArrSubAreIdxVarsOfEnclosingLoops(arr_sub, enclosing_loop_hdrs))
				throw std::runtime_error(
					"only index variables of enclosing loops can appear as unknowns in "
					"array subscripts");

			if (!ArrSubEvaluatesToArrElemType(arr_sub, sym_tbl_tree, *enclosing_loop_hdrs[0]->GetStmt()))
				throw std::runtime_error("array subscript evaluated type must be equal to array element type");
		}

		void ValidateAssignStmt(
			const AssignStmt& assign_stmt
			, const SymTblTree& sym_tbl_tree
			, const ForLoopHeader& enclosing_loop_hdr)
		{
			if (assign_stmt.GetLhs() == nullptr)
				throw std::runtime_error("only array subscripts can appear as LHS of assignment stmts");

			std::vector<const ForLoopHeader*> enclosing_loop_hdrs = { &enclosing_loop_hdr };
			enclosing_loop_hdr.GetOuterLoopHeaders(enclosing_loop_hdrs);

			ValidateArrSubscript(*assign_stmt.GetLhs(), sym_tbl_tree, enclosing_loop_hdrs);
			for (auto & arr_sub : assign_stmt.GetRhs())
				ValidateArrSubscript(arr_sub, sym_tbl_tree, enclosing_loop_hdrs);
		}

		/// Based on the known input and output nest variables, classify the variables
		/// in the current assignment statement as input or output variables. Note that
		/// single variable can be both input and output variable
		void ClassifyAssignStmtArrVars(
			AssignStmt& assgn_stmt
			, std::vector<clang::VarDecl*>& nest_input_vars
			, std::vector<clang::VarDecl*>& nest_output_vars)
		{
			if (auto lhs_var = assgn_stmt.GetLhs())
			{
				auto lhs_arr_decl = lhs_var->GetArrDecl();
				if (std::find(nest_output_vars.begin(), nest_output_vars.end(), lhs_arr_decl)
					== nest_output_vars.end())
					nest_output_vars.push_back(lhs_arr_decl);
			}
			for (auto & rhs_var : assgn_stmt.GetRhs())
			{
				auto rhs_arr_decl = rhs_var.GetArrDecl();
				if (std::find(nest_input_vars.begin(), nest_input_vars.end(), rhs_arr_decl)
					== nest_input_vars.end())
					nest_input_vars.push_back(rhs_arr_decl);
			}
		}

	} /// Anonymous namespace


	PerfectLoopNest::PerfectLoopNest(
		clang::ASTContext& ast_ctx
		, const SymTblTree& sym_tbl_tree
		, clang::ForStmt& outermost_loop)

		: m_ast_ctx(ast_ctx)
		, m_sym_tbl_tree(sym_tbl_tree)
		, m_outermost_loop_hdr(ast_ctx, outermost_loop, nullptr)
		, m_nest_idx_vec({ &m_outermost_loop_hdr.GetIdxVar() })
		, m_nest_depth(1)
	{
	}

	void PerfectLoopNest::PushLoop(
		const clang::ForStmt& parent_loop
		, clang::ForStmt& new_loop)
	{
		if (!m_assign_stmts.empty())
			throw std::runtime_error("perfect loop nest may not contain stmts in between the loops");

		if (auto parent_loop_hdr = FindForLoopHdr(&m_outermost_loop_hdr, parent_loop))
		{
			if (parent_loop_hdr->GetChild())
				throw std::runtime_error("perfect loop nest may not contain multiple loops at any level");

			parent_loop_hdr->SetChild(std::make_unique<ForLoopHeader>(m_ast_ctx, new_loop, parent_loop_hdr));
			m_nest_idx_vec.push_back(&parent_loop_hdr->GetChild()->GetIdxVar());
			++m_nest_depth;
			return;
		}
		assert("failed to find parent loop");
	}

	void PerfectLoopNest::PushAssignStmt(
		const clang::ForStmt& enclosing_loop
		, clang::BinaryOperator& assign_op)
	{
		auto enclosing_loop_hdr = FindForLoopHdr(&m_outermost_loop_hdr, enclosing_loop);
		assert(enclosing_loop_hdr != nullptr && "failed to find enclosing loop");
		if (enclosing_loop_hdr->GetChild())
			throw std::runtime_error("perfect loop may not contain stmts in between the loops");

		AssignStmt assign_stmt(m_ast_ctx, assign_op);
		ValidateAssignStmt(assign_stmt, m_sym_tbl_tree, *enclosing_loop_hdr);
		ClassifyAssignStmtArrVars(assign_stmt, m_nest_input_vars, m_nest_output_vars);
		m_assign_stmts.push_back(std::move(assign_stmt));
	}

	std::size_t PerfectLoopNest::GetNestDepth() const
	{
		return m_nest_depth;
	}

	ForLoopHeader& PerfectLoopNest::GetOutermostLoopHdr()
	{
		return m_outermost_loop_hdr;
	}

	ForLoopHeader* PerfectLoopNest::GetLoopHdr(std::size_t level)
	{
		return const_cast<ForLoopHeader*>(
			static_cast<const PerfectLoopNest*>(this)->GetLoopHdr(level));
	}

	const ForLoopHeader& PerfectLoopNest::GetOutermostLoopHdr() const
	{
		return m_outermost_loop_hdr;
	}

	const ForLoopHeader* PerfectLoopNest::GetLoopHdr(std::size_t level) const
	{
		assert(level < m_nest_depth && "level out of bounds");
		if (level >= m_nest_depth)
			return nullptr;

		const ForLoopHeader* loop_hdr = &m_outermost_loop_hdr;
		std::size_t cnt = 0;
		while (cnt++ < level)
			loop_hdr = loop_hdr->GetChild();

		return loop_hdr;
	}

	std::vector<AssignStmt>& PerfectLoopNest::GetAssignStmts()
	{
		return m_assign_stmts;
	}

	const std::vector<AssignStmt>& PerfectLoopNest::GetAssignStmts() const
	{
		return m_assign_stmts;
	}

	std::vector<clang::VarDecl*>& PerfectLoopNest::GetNestIdxVec()
	{
		return m_nest_idx_vec;
	}

	const std::vector<clang::VarDecl*>& PerfectLoopNest::GetNestIdxVec() const
	{
		return m_nest_idx_vec;
	}

	void PerfectLoopNest::Dump() const
	{
		const ForLoopHeader* loop_hdr = &m_outermost_loop_hdr;
		unsigned int lvl = 0;
		while (loop_hdr != nullptr)
		{
			std::cout << std::string(NUM_OF_SPACES_BTWN_LVLS * lvl++, ' ');
			loop_hdr->Dump();
			std::cout << "\n";
			loop_hdr = loop_hdr->GetChild();
		}

		for (auto & assign_stmt : m_assign_stmts)
		{
			std::cout << std::string(NUM_OF_SPACES_BTWN_LVLS * lvl, ' ');
			assign_stmt.Dump();
			std::cout << "\n";
		}
	}

	std::vector<clang::VarDecl*>& PerfectLoopNest::GetNestInputVars()
	{
		return m_nest_input_vars;
	}

	std::vector<clang::VarDecl*>& PerfectLoopNest::GetNestOutputVars()
	{
		return m_nest_output_vars;
	}

} /// namespace gap