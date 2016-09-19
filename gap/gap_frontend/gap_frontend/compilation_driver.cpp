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

#include "compilation_driver.h"
#include <iostream>
#include <clang/AST/DeclGroup.h>
#include "sym_tbl_builder.h"
#include "nest_info_collector.h"
#include "nest_analysis_driver.h"
#include <clang/AST/Stmt.h>


namespace gap
{
	namespace
	{

		unsigned int GetStmtLineNumber(
			clang::SourceManager& src_mngr
			, clang::FileID file_id
			, clang::Stmt* stmt)
		{
			return
				src_mngr.getLineNumber(
					file_id
					, src_mngr.getDecomposedLoc(stmt->getLocStart()).second);
		}

	} /// Anonymous namespace


	CompilationDriver::CompilationDriver(clang::ASTContext& ast_ctx)
		: m_ast_ctx(ast_ctx)
		, m_code_gen(m_ast_ctx)
	{
		auto & src_mngr = m_ast_ctx.getSourceManager();
		m_main_file =
		{
			src_mngr.getMainFileID()
			, src_mngr.getFileEntryForID(src_mngr.getMainFileID())
		};
	}

	CompilationDriver::~CompilationDriver()
	{
		m_code_gen.CompleteGeneration();
	}

	bool CompilationDriver::HandleTopLevelDecl(clang::DeclGroupRef decl_group_ref)
	{
		for (auto decl_iter = decl_group_ref.begin(); decl_iter != decl_group_ref.end(); ++decl_iter)
		{
			if (!clang::isa<clang::FunctionDecl>(*decl_iter))
				continue;

			/// Create a symbol table for this top-level declaration
			SymTblBuilder sym_tbl_builder(m_ast_ctx);
			sym_tbl_builder.TraverseDecl(*decl_iter);

			auto const & loop_nests = sym_tbl_builder.GetLoopNests();
			for (auto nest : loop_nests)
			{
				try
				{
					NestInfoCollector collector(m_ast_ctx, sym_tbl_builder.GetSymTblTree(), *nest);
					NestAnalysisDriver analysis_driver(m_ast_ctx, collector.GetConstructedNest());
					m_code_gen.HandleLoopNest(
						analysis_driver
						, sym_tbl_builder.GetSymTblTree()
						, *clang::cast<clang::FunctionDecl>(*decl_iter));
				}
				catch (const std::runtime_error& exc)
				{
					std::cout
						<< m_main_file.second->getName()
						<< "("
						<< GetStmtLineNumber(m_ast_ctx.getSourceManager(), m_main_file.first, nest)
						<< "): info: "
						<< exc.what()
						<< std::endl;
				}
			}
		}

		return true;
	}

} /// namespace gap