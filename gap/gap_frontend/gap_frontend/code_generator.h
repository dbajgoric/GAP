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

#ifndef GAP_FRONTEND_CODE_GENERATOR_H
#define GAP_FRONTEND_CODE_GENERATOR_H

#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Decl.h>

namespace gap
{

	/// Forward declarations
	class NestAnalysisDriver;
	class SymTblTree;

	/// Performs parallel code generation on the level of single translation unit. Assuming
	/// that given source contains any parallelizable loop nests, this generator will produce
	/// three files:
	/// 1) modified source file corresponding to the file being compiled - this code contains
	/// kernel invocation statement instead of sequential nests and other supporting code
	/// 2) Kernel declaration header - each kernel generated throughout compilation of given
	/// translation unit will be declared here
	/// 3) Kernel definition source - each kernel produced is defined in this source
	///
	/// Note that generator does not modify any of the existing source files, it simply uses
	/// their content to generate a new set of sources
	class CodeGenerator
	{
	public:

		explicit CodeGenerator(clang::ASTContext& ast_ctx);

		void HandleLoopNest(
			NestAnalysisDriver& analysis_driver
			, SymTblTree& sym_tbl_tree
			, const clang::FunctionDecl& original_fun);

		void CompleteGeneration();

	private:

		std::string StmtToStr(clang::Stmt* stmt);
		void ReplaceStmt(clang::Stmt* stmt_to_remove, clang::Stmt* stmt_to_add);
		void WriteMainFile();
		void WriteKrnlHdrFile();
		void WriteKrnlSrcFile();

		clang::ASTContext& m_ast_ctx;
		clang::Rewriter m_rewriter;
		clang::SourceManager& m_src_mngr;
		clang::PrintingPolicy m_print_policy;

		enum { MAIN_FILE_ABSPATH, HDR_FILE_ABSPATH, SRC_FILE_ABSPATH, FILE_COUNT = 3 };
		std::string m_abs_paths[FILE_COUNT];
		enum { KRNL_DECL_BUF, KRNL_DEF_BUF, BUF_COUNT = 2 };
		std::string m_str_buffers[BUF_COUNT];
		std::string m_hdr_guard;
		std::string m_hdr_base_name;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_CODE_GENERATOR_H
