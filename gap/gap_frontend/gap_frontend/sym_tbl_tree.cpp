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

#include "sym_tbl_tree.h"

namespace gap
{

	SymTblTree::NodePtrBoolPair SymTblTree::Insert(
		const clang::Stmt* par_node_stmt
		, clang::Stmt* scope_stmt
		, SymbolTable&& sym_tbl)
	{
		if (par_node_stmt == nullptr)
		{
			assert(
				!m_root_node &&
				__FUNCTION__ "(): par_node_stmt may be nullptr only on root node insertion");

			if (m_root_node)
				return NodePtrBoolPair(nullptr, false);

			m_root_node.reset(new TreeNode(nullptr, scope_stmt, std::move(sym_tbl)));
			return NodePtrBoolPair(m_root_node.get(), true);
		}

		NodePtrBoolPair node_ptr_pair = SearchHelper(par_node_stmt, m_root_node.get());
		if (!node_ptr_pair.second)
			return node_ptr_pair;

		/// Insert the new node to the parent's list
		node_ptr_pair.first->PushChild(std::make_unique<TreeNode>(
			node_ptr_pair.first
			, scope_stmt
			, std::move(sym_tbl)));

		return NodePtrBoolPair(&node_ptr_pair.first->BackChild(), true);
	}

	bool SymTblTree::Remove(clang::Stmt* stmt)
	{
		/// FIXME: implement or remove the method
		return false;
	}

	SymTblTree::NodePtrBoolPair SymTblTree::SearchHelper(
		const clang::Stmt* scope_stmt
		, TreeNode* node) const
	{
		if (node == nullptr)
			return NodePtrBoolPair(nullptr, false);
		if (node->GetScopeStmt() == scope_stmt)
			return NodePtrBoolPair(node, true);

		/// Traverse children left to right
		for (auto iter = node->ChildBegin(); iter != node->ChildEnd(); ++iter)
		{
			NodePtrBoolPair ret_pair = SearchHelper(scope_stmt, (*iter).get());
			if (ret_pair.second)
				return ret_pair;
		}
		return NodePtrBoolPair(nullptr, false);
	}

	SymTblTree::NodePtrBoolPair SymTblTree::Search(
		const clang::Stmt* scope_stmt)
	{
		return SearchHelper(scope_stmt, m_root_node.get());
	}

	SymTblTree::ConstNodePtrBoolPair SymTblTree::Search(
		const clang::Stmt* scope_stmt) const
	{
		return SearchHelper(scope_stmt, m_root_node.get());
	}

	SymbolTable* SymTblTree::FindSymTable(const clang::Stmt* scope_stmt)
	{
		const SymTblTree* this_ptr = static_cast<const SymTblTree*>(this);
		return const_cast<SymbolTable*>(this_ptr->FindSymTable(scope_stmt));
	}

	const SymbolTable* SymTblTree::FindSymTable(const clang::Stmt* scope_stmt) const
	{
		auto symtbl_bool_pair = Search(scope_stmt);
		return
			symtbl_bool_pair.second
			? &symtbl_bool_pair.first->GetSymTbl()
			: nullptr;
	}

	const Identifier* SymTblTree::FindIdHelper(
		const TreeNode* node
		, const std::string& name) const
	{
		if (node == nullptr)
			return nullptr;
		if (node->GetSymTbl().Contains(name))
			return node->GetSymTbl().GetIdentifier(name);

		return FindIdHelper(node->GetParentNode(), name);
	}

	Identifier* SymTblTree::FindIdentifier(
		const clang::Stmt* start_scope
		, const std::string& id_name)
	{
		const SymTblTree* this_ptr = static_cast<const SymTblTree*>(this);
		return const_cast<Identifier*>(this_ptr->FindIdentifier(start_scope, id_name));
	}

	const Identifier* SymTblTree::FindIdentifier(
		const clang::Stmt* start_scope
		, const std::string& id_name) const
	{
		auto node_bool_pair = Search(start_scope);
		return
			node_bool_pair.second
			? FindIdHelper(node_bool_pair.first, id_name)
			: nullptr;
	}

	bool SymTblTree::Empty() const
	{
		return !m_root_node;
	}

	void SymTblTree::Dump() const
	{
		if (Empty())
			return;

		TreeNodeDumper dumper(m_ast_context, *m_root_node.get());
		dumper.Dump();
	}

	void  SymTblTree::DumpWithSymbols() const
	{
		if (Empty())
			return;

		TreeNodeDumper dumper(m_ast_context, *m_root_node.get());
		dumper.DumpWithSymbols();
	}

} /// namespace gap