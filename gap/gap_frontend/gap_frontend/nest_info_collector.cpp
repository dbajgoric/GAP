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

#include "nest_info_collector.h"
#include "sym_tbl_tree.h"
#include <exception>
#include <cassert>


namespace gap
{

	NestInfoCollector::NestInfoCollector(
		clang::ASTContext& ast_ctx
		, const SymTblTree& sym_tbl_tree
		, clang::ForStmt& outermost_loop)

		: m_ast_ctx(ast_ctx)
		, m_sym_tbl_tree(sym_tbl_tree)
		, m_curr_loop(nullptr)
	{
		TraverseForStmt(&outermost_loop);
	}

	bool NestInfoCollector::TraverseForStmt(clang::ForStmt* for_stmt)
	{
		if (!m_perfect_nest)
		{
			/// Instantiate the perfect loop nest instance
			assert(m_curr_loop == nullptr && "m_curr_loop must be nullptr at this point");
			m_perfect_nest.reset(new PerfectLoopNest(m_ast_ctx, m_sym_tbl_tree, *for_stmt));
		}
		else
		{
			assert(m_curr_loop != nullptr && "m_curr_loop cannot be nullptr at this point");
			m_perfect_nest->PushLoop(*m_curr_loop, *for_stmt);
		}

		m_curr_loop = for_stmt;
		return RecursiveASTVisitorBase::TraverseStmt(for_stmt->getBody());
	}

	bool NestInfoCollector::TraverseBinAssign(clang::BinaryOperator* assign_op)
	{
		assert(!!m_perfect_nest && "loop nest instance should've been already created");
		assert(m_curr_loop != nullptr && "m_curr_loop cannot be nullptr at this point");
		m_perfect_nest->PushAssignStmt(*m_curr_loop, *assign_op);
		return true;
	}

	bool NestInfoCollector::VisitStmt(clang::Stmt* stmt)
	{
		clang::Stmt::StmtClass stmt_class = stmt->getStmtClass();
		if (stmt_class != clang::Stmt::ForStmtClass
			&& stmt_class != clang::Stmt::BinaryOperatorClass
			&& stmt_class != clang::Stmt::CompoundStmtClass)
			throw std::runtime_error(
				"nest cannot be parallelized as it contains statements other "
				"than inner for loops and assignment statements");

		if (auto bin_op = clang::dyn_cast<const clang::BinaryOperator>(stmt))
			if (bin_op->getOpcode() != clang::BO_Assign)
				throw std::runtime_error(
					"nest cannot be parallelized as it contains "
					"a non-assignment binary statement");

		return true;
	}

	PerfectLoopNest& NestInfoCollector::GetConstructedNest()
	{
		assert(!!m_perfect_nest && "m_perfect_nest must not be empty");
		return *m_perfect_nest;
	}

	const PerfectLoopNest& NestInfoCollector::GetConstructedNest() const
	{
		assert(!!m_perfect_nest && "m_perfect_nest must not be empty");
		return *m_perfect_nest;
	}

} /// namespace gap