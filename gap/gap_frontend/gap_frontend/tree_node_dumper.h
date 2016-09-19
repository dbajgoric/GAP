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

#ifndef GAP_FRONTEND_TREE_NODE_DUMPER_H
#define GAP_FRONTEND_TREE_NODE_DUMPER_H

#include <iostream>
#include <clang/AST/ASTContext.h>
#include "sym_tbl_tree_node.h"

namespace gap
{

	/// Kind of TreeNode visitor that traverses the node and all its
	/// children in pre-order fashion and dumps the info about nodes
	/// to standard output
	class TreeNodeDumper
	{
		typedef void(TreeNodeDumper::*VisitFn)(const TreeNode* node) const;

	public:

		explicit TreeNodeDumper(clang::ASTContext& ast_context, const TreeNode& node);
		void Dump() const;
		void DumpWithSymbols() const;

	private:

		void VisitNode(const TreeNode* node) const;
		void VisitNodeWithSymbols(const TreeNode* node) const;
		void DumpNodeHelper(const TreeNode* node, VisitFn visit_fn) const;

		const TreeNode& m_node;
		mutable unsigned int m_level;
		static unsigned int m_spaces_btw_lvls;
		clang::ASTContext& m_ast_context;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_TREE_NODE_DUMPER_H
