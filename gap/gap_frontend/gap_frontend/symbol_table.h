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

#ifndef GAP_FRONTEND_SYMBOL_TABLE_H
#define GAP_FRONTEND_SYMBOL_TABLE_H

#include <map>
#include <memory>
#include <clang/AST/Stmt.h>
#include "identifier.h"

namespace gap
{

	/// Provides the most common set of services offered by a symbol table. It
	/// also registers stmt class whose scope it represents
	class SymbolTable
	{
		typedef std::unique_ptr<Identifier>				IdentifierPtr;
		typedef std::map<std::string, IdentifierPtr>	MapType;
		typedef MapType::value_type						MapValType;
		typedef MapType::iterator						MapIterator;
		typedef MapType::const_iterator					ConstMapIterator;

	public:

		explicit SymbolTable(clang::Stmt* stmt);
		SymbolTable(SymbolTable&& other);
		SymbolTable& operator=(SymbolTable&& other);

		bool Contains(const std::string& name) const;
		bool Empty() const;
		bool AddSymbol(const std::string& name, IdentifierPtr identifier_ptr);
		Identifier* GetIdentifier(const std::string& name) const;
		clang::Stmt* GetScopeStmt() const;

		MapIterator ChildBegin();
		MapIterator ChildEnd();
		ConstMapIterator ChildBegin() const;
		ConstMapIterator ChildEnd() const;

	private:

		SymbolTable(const SymbolTable& other) = delete;
		SymbolTable& operator=(SymbolTable& other) = delete;

		MapType m_symbols;
		clang::Stmt* m_stmt;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_SYMBOL_TABLE_H
