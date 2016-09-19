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

#include "fake_stmt_mngr.h"
#include <cassert>


namespace gap
{

	void FakeStmtMngr::PushParStmt(const clang::Stmt* par_stmt, bool is_fake_stmt)
	{
		m_parstmt_stack.push(par_stmt);
		if (!is_fake_stmt)
			m_fakestmt_cnt_stack.push(0);
		else
			IncTopParCnt();
	}

	void FakeStmtMngr::ClearFakesTopPar()
	{
		assert(
			!m_fakestmt_cnt_stack.empty() &&
			"m_fakestmt_cnt_stack is empty");

		auto fake_cnt = m_fakestmt_cnt_stack.top();
		for (auto i = 0U; i < fake_cnt; ++i)
			m_parstmt_stack.pop();
	}

	void FakeStmtMngr::PopParStmt()
	{
		assert(
			!m_parstmt_stack.empty() &&
			!m_fakestmt_cnt_stack.empty() &&
			"m_parstmt_stack and/or m_fakestmt_cnt_stack are empty");

		m_parstmt_stack.pop();
		m_fakestmt_cnt_stack.pop();
	}

	const clang::Stmt* FakeStmtMngr::GetTopParStmt() const
	{
		if (m_parstmt_stack.empty())
			return nullptr;

		return m_parstmt_stack.top();
	}

	void FakeStmtMngr::IncTopParCnt()
	{
		assert(
			!m_fakestmt_cnt_stack.empty() &&
			"m_fakestmt_cnt_stack is empty");

		++m_fakestmt_cnt_stack.top();
	}

	void FakeStmtMngr::ResetTopParCnt()
	{
		assert(
			!m_fakestmt_cnt_stack.empty() &&
			"m_fakestmt_cnt_stack is empty");

		m_fakestmt_cnt_stack.top() = 0;
	}

} /// namespace gap