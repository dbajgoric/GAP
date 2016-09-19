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

#include "symbol_table.h"
#include <algorithm>
#include <exception>

namespace gap
{

	SymbolTable::SymbolTable(clang::Stmt* stmt)
		: m_stmt(stmt)
	{
	}

	SymbolTable::SymbolTable(SymbolTable&& other)
		: m_symbols(std::move(other.m_symbols))
		, m_stmt(other.m_stmt)
	{
		other.m_stmt = nullptr;
	}

	SymbolTable& SymbolTable::operator=(SymbolTable&& other)
	{
		if (this != &other)
		{
			m_symbols = std::move(other.m_symbols);
			m_stmt = other.m_stmt;
			other.m_stmt = nullptr;
		}
		return *this;
	}

	bool SymbolTable::Contains(const std::string& name) const
	{
		return m_symbols.find(name) != m_symbols.end();
	}

	bool SymbolTable::Empty() const
	{
		return m_symbols.empty();
	}

	bool SymbolTable::AddSymbol(const std::string& name, IdentifierPtr identifier_ptr)
	{
		return m_symbols.insert(std::make_pair(name, std::move(identifier_ptr))).second;
	}

	Identifier* SymbolTable::GetIdentifier(const std::string& name) const
	{
		auto pair = m_symbols.find(name);
		if (pair == m_symbols.end())
			return nullptr;

		return pair->second.get();
	}

	clang::Stmt* SymbolTable::GetScopeStmt() const
	{
		return m_stmt;
	}

	SymbolTable::MapIterator SymbolTable::ChildBegin()
	{
		return m_symbols.begin();
	}

	SymbolTable::ConstMapIterator SymbolTable::ChildBegin() const
	{
		return m_symbols.begin();
	}

	SymbolTable::MapIterator SymbolTable::ChildEnd()
	{
		return m_symbols.end();
	}

	SymbolTable::ConstMapIterator SymbolTable::ChildEnd() const
	{
		return m_symbols.end();
	}

} /// namespace gap