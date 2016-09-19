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

#ifndef GAP_FRONTEND_SYM_TBL_TREE_NODE_H
#define GAP_FRONTEND_SYM_TBL_TREE_NODE_H

#include <memory>
#include <clang/AST/Stmt.h>
#include "symbol_table.h"

namespace gap
{

	/// Single tree node that contains the symbol table, pointer to the
	/// parent node (if not root) and list of children nodes. The node
	/// also stores the stmt that owns the scope
	class TreeNode
	{
	public:

		typedef std::unique_ptr<TreeNode> UniqueNodePtr;
		typedef std::vector<UniqueNodePtr> UniqueNodePtrVector;
		typedef UniqueNodePtrVector::size_type size_type;
		typedef UniqueNodePtrVector::iterator iterator;
		typedef UniqueNodePtrVector::const_iterator const_iterator;

		TreeNode(TreeNode* par_node, clang::Stmt* scope_stmt, SymbolTable&& sym_tbl);

		/// Modifiers
		void PushChild(UniqueNodePtr&& node_ptr);
		void PopChild();

		/// Getters
		SymbolTable& GetSymTbl();
		const SymbolTable& GetSymTbl() const;
		TreeNode* GetParentNode();
		const TreeNode* GetParentNode() const;
		clang::Stmt* GetScopeStmt();
		const clang::Stmt* GetScopeStmt() const;

		/// Iterators
		iterator ChildBegin();
		const_iterator ChildBegin() const;
		iterator ChildEnd();
		const_iterator ChildEnd() const;

		/// Element access
		TreeNode& FrontChild();
		const TreeNode& FrontChild() const;
		TreeNode& BackChild();
		TreeNode& BackChild() const;

		/// Capacity
		bool HasChildren() const;
		size_type ChildrenCount() const;

	private:

		TreeNode* m_par_node;
		clang::Stmt* m_scope_stmt;
		SymbolTable m_sym_tbl;
		UniqueNodePtrVector m_children;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_SYM_TBL_TREE_NODE_H
