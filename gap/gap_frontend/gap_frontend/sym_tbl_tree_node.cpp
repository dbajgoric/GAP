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

#include "sym_tbl_tree_node.h"

namespace gap
{

	TreeNode::TreeNode(
		TreeNode* par_node
		, clang::Stmt* scope_stmt
		, SymbolTable&& sym_tbl)

		: m_par_node(par_node)
		, m_scope_stmt(scope_stmt)
		, m_sym_tbl(std::move(sym_tbl))
	{
	}

	void TreeNode::PushChild(UniqueNodePtr&& node_ptr)
	{
		m_children.push_back(std::move(node_ptr));
	}

	void TreeNode::PopChild()
	{
		m_children.pop_back();
	}

	SymbolTable& TreeNode::GetSymTbl()
	{
		return m_sym_tbl;
	}

	const SymbolTable& TreeNode::GetSymTbl() const
	{
		return m_sym_tbl;
	}

	TreeNode* TreeNode::GetParentNode()
	{
		return m_par_node;
	}

	const TreeNode* TreeNode::GetParentNode() const
	{
		return m_par_node;
	}

	clang::Stmt* TreeNode::GetScopeStmt()
	{
		return m_scope_stmt;
	}

	const clang::Stmt* TreeNode::GetScopeStmt() const
	{
		return m_scope_stmt;
	}

	TreeNode::iterator TreeNode::ChildBegin()
	{
		return m_children.begin();
	}

	TreeNode::const_iterator TreeNode::ChildBegin() const
	{
		return m_children.begin();
	}

	TreeNode::iterator TreeNode::ChildEnd()
	{
		return m_children.end();
	}

	TreeNode::const_iterator TreeNode::ChildEnd() const
	{
		return m_children.end();
	}

	TreeNode& TreeNode::FrontChild()
	{
		return *m_children.front().get();
	}

	const TreeNode& TreeNode::FrontChild() const
	{
		return *m_children.front().get();
	}

	TreeNode& TreeNode::BackChild()
	{
		return *m_children.back().get();
	}

	TreeNode& TreeNode::BackChild() const
	{
		return *m_children.back().get();
	}

	bool TreeNode::HasChildren() const
	{
		return m_children.empty();
	}

	TreeNode::size_type TreeNode::ChildrenCount() const
	{
		return m_children.size();
	}

} /// namespace gap