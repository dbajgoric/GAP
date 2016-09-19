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

#ifndef GAP_FRONTEND_FAKE_STMT_MNGR_H
#define GAP_FRONTEND_FAKE_STMT_MNGR_H

#include <list>
#include <stack>
#include <clang/AST/Stmt.h>

namespace gap
{

	/// Each of the conditional stmts plus the compound stmt act
	/// as a divider, in sense that they divide the enclosing scope
	/// on two parts: the scope corresponding to this particular
	/// conditional stmt and the remainder the enclosing scope.
	///
	/// Symbol table builder manages this nuisance using so-called
	/// fake stmts that 'own' the remainder of the enclosing scope
	/// after it's been divided. In this way, any symbols added to
	/// the symbol table after the divisor stmt will not be visible
	/// to the scope of the divisor, as they'll land to the scope
	/// owned by the fake smtm.
	///
	/// This class stores and manages fake stmts along with other
	/// supporting data
	class FakeStmtMngr
	{
		typedef std::stack<const clang::Stmt*> StmtPtrStackType;
		typedef std::stack<unsigned int> UIntStackType;

	public:

		FakeStmtMngr() = default;

		void PushParStmt(const clang::Stmt* par_stmt, bool is_fake_stmt = false);
		void ClearFakesTopPar();
		void PopParStmt();
		const clang::Stmt* GetTopParStmt() const;
		void IncTopParCnt();
		void ResetTopParCnt();

	private:

		StmtPtrStackType m_parstmt_stack;
		UIntStackType m_fakestmt_cnt_stack;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_FAKE_STMT_MNGR_H
