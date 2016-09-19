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

#ifndef GAP_FRONTEND_SYM_TBL_TREE_H
#define GAP_FRONTEND_SYM_TBL_TREE_H

#include <memory>
#include <utility>
#include <cassert>
#include "tree_node_dumper.h"

namespace gap
{

	/// M-ary tree of symbol tables. Each symbol table corresponds to
	/// a specif scope that may contain identifiers (contained within
	/// the symbol table of the tree node.
	///
	/// Note that the tree is initially empty and it is up to the user
	/// to create its nodes. When inserting a new node in the tree, stmt
	/// and its parent both have to be provided. Parent is allowed to be
	/// zero if and only if the tree is empty, at which point the provided
	/// stmt identifies the root scope. Pre-order tree traversal is used
	/// for insertion, removal and lookup operations
	class SymTblTree
	{
		typedef std::unique_ptr<TreeNode> UniqueNodePtr;

	public:

		typedef std::pair<TreeNode*, bool> NodePtrBoolPair;
		typedef std::pair<const TreeNode*, bool> ConstNodePtrBoolPair;

		SymTblTree(clang::ASTContext& ast_context)
			: m_ast_context(ast_context)
		{
		}

		/// Modifiers
		NodePtrBoolPair Insert(const clang::Stmt* par_node_stmt, clang::Stmt* scope_stmt, SymbolTable&& sym_tbl);
		bool Remove(clang::Stmt* scope_stmt);

		/// Lookup
		NodePtrBoolPair Search(const clang::Stmt* scope_stmt);
		ConstNodePtrBoolPair Search(const clang::Stmt* scope_stmt) const;
		SymbolTable* FindSymTable(const clang::Stmt* scope_stmt);
		const SymbolTable* FindSymTable(const clang::Stmt* scope_stmt) const;
		Identifier* FindIdentifier(const clang::Stmt* start_scope, const std::string& id_name);
		const Identifier* FindIdentifier(const clang::Stmt* start_scope, const std::string& id_name) const;

		/// Capacity
		bool Empty() const;

		/// Dump
		void Dump() const;
		void DumpWithSymbols() const;

	private:

		NodePtrBoolPair SearchHelper(const clang::Stmt* scope_stmt, TreeNode* node) const;
		const Identifier* FindIdHelper(const TreeNode* node, const std::string& name) const;

		SymTblTree(const SymTblTree& other) = delete;
		SymTblTree& operator=(const SymTblTree& other) = delete;

		UniqueNodePtr m_root_node;
		clang::ASTContext& m_ast_context;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_SYM_TBL_TREE_H
