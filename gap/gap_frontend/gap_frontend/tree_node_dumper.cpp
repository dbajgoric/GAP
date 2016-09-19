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

#include "tree_node_dumper.h"

namespace gap
{

	unsigned int TreeNodeDumper::m_spaces_btw_lvls = 2;

	TreeNodeDumper::TreeNodeDumper(clang::ASTContext& ast_context, const TreeNode& node)
		: m_node(node)
		, m_level(0)
		, m_ast_context(ast_context)
	{
	}

	void TreeNodeDumper::VisitNode(const TreeNode* node) const
	{
		std::cout << std::string(m_spaces_btw_lvls * m_level, ' ');
		std::cout << node->GetScopeStmt()->getStmtClassName() << "\n";
	}

	void TreeNodeDumper::VisitNodeWithSymbols(const TreeNode* node) const
	{
		std::cout << std::string(m_spaces_btw_lvls * m_level, ' ');
		std::cout << node->GetScopeStmt()->getStmtClassName();
		auto & symtbl = node->GetSymTbl();
		if (!symtbl.Empty())
		{
			std::cout << "{\n";
			++m_level;
			for (auto iter = symtbl.ChildBegin(); iter != symtbl.ChildEnd(); ++iter)
			{
				std::cout << std::string(m_spaces_btw_lvls * m_level, ' ');
				std::cout << iter->first;
				if (iter->second->IsArrayLikeIdentifier())
				{
					const ArrayLikeIdentifier* arr_id = CastAsArrayId(iter->second.get());
					std::cout << ", Dim = " << arr_id->GetDimensionality() << ", [";
					for (unsigned int i = 0; i < arr_id->GetDimensionality(); ++i)
					{
						arr_id->GetSize(i)->dumpPretty(m_ast_context);
						if (i < arr_id->GetDimensionality() - 1)
							std::cout << ", ";
					}
					std::cout << "]";
				}
				std::cout << "\n";
			}
			--m_level;
			std::cout << std::string(m_spaces_btw_lvls * m_level, ' ');
			std::cout << "}\n";
		}
		else
		{
			std::cout << "\n";
		}
	}

	void TreeNodeDumper::DumpNodeHelper(
		const TreeNode* node
		, VisitFn visit_fn) const
	{
		if (node == nullptr)
			return;

		(this->*visit_fn)(node);
		++m_level;
		for (auto iter = node->ChildBegin(); iter != node->ChildEnd(); ++iter)
			DumpNodeHelper((*iter).get(), visit_fn);
		--m_level;
	}

	void TreeNodeDumper::Dump() const
	{
		DumpNodeHelper(&m_node, &TreeNodeDumper::VisitNode);
		std::cout << std::flush;
	}

	void TreeNodeDumper::DumpWithSymbols() const
	{
		DumpNodeHelper(&m_node, &TreeNodeDumper::VisitNodeWithSymbols);
		std::cout << std::flush;
	}

} /// namespace gap