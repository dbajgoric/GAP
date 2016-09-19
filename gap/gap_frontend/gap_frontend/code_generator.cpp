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

#include "code_generator.h"
#include "file_path_helpers.h"
#include "invocation_stmts_gen.h"
#include "kernel_def_gen.h"
#include "nest_analysis_driver.h"
#include "perfect_loop_nest.h"
#include "ast_helpers.h"
#include <fstream>
#include <cctype>
#include <cassert>


namespace gap
{
	namespace
	{

		std::string ToUpper(const std::string& str)
		{
			std::string upper_case_str;
			std::transform(
				str.begin()
				, str.end()
				, std::back_inserter(upper_case_str)
				, [](unsigned char c) { return std::toupper(c); });

			return upper_case_str;
		}

	} /// Anonymous namespace


	CodeGenerator::CodeGenerator(clang::ASTContext& ast_ctx)
		: m_ast_ctx(ast_ctx)
		, m_rewriter(ast_ctx.getSourceManager(), ast_ctx.getLangOpts())
		, m_src_mngr(ast_ctx.getSourceManager())
		, m_print_policy(ast_ctx.getLangOpts())
	{
		auto main_file_entry = m_src_mngr.getFileEntryForID(m_src_mngr.getMainFileID());
		assert(main_file_entry != nullptr);
		std::string file_root = GetFileRoot(main_file_entry->getName());
		std::string dir = main_file_entry->getDir()->getName();

		m_abs_paths[MAIN_FILE_ABSPATH] = dir + "\\__" + file_root + "_c2cuda.cu";
		m_hdr_base_name = "__" + file_root + "_kernel_decl_c2cuda.cuh";
		m_abs_paths[HDR_FILE_ABSPATH] = dir + "\\" + m_hdr_base_name;
		m_abs_paths[SRC_FILE_ABSPATH] = dir + "\\__" + file_root + "_kernel_def_c2cuda.cu";
		m_hdr_guard = ToUpper(file_root) + "_KERNEL_DECL_C2CUDA_H";

		/// Set indentation to 4 spaces
		m_print_policy.Indentation = 4;
	}

	std::string CodeGenerator::StmtToStr(clang::Stmt* stmt)
	{
		std::string str;
		llvm::raw_string_ostream raw_stream(str);
		stmt->printPretty(raw_stream, nullptr, m_print_policy);
		return raw_stream.str();
	}

	void CodeGenerator::ReplaceStmt(clang::Stmt* stmt_to_remove, clang::Stmt* stmt_to_add)
	{
		m_rewriter.RemoveText(stmt_to_remove->getSourceRange());
		m_rewriter.InsertText(stmt_to_remove->getSourceRange().getBegin(), StmtToStr(stmt_to_add), true, true);
	}

	void CodeGenerator::WriteMainFile()
	{
		if (m_str_buffers[KRNL_DEF_BUF].empty())
			return;

		std::error_code err_code;
		llvm::raw_fd_ostream main_file(
			m_abs_paths[MAIN_FILE_ABSPATH]
			, err_code
			, llvm::sys::fs::F_None);

		main_file
			<< "#include <math.h>\n"
			<< "#include <cuda_runtime.h>\n"
			<< "#include \"" << m_hdr_base_name << "\"\n\n";

		m_rewriter.getEditBuffer(m_src_mngr.getMainFileID()).write(main_file);
	}

	void CodeGenerator::WriteKrnlHdrFile()
	{
		if (m_str_buffers[KRNL_DECL_BUF].empty())
			return;

		std::error_code err_code;
		llvm::raw_fd_ostream hdr_file(
			m_abs_paths[HDR_FILE_ABSPATH]
			, err_code
			, llvm::sys::fs::F_None);

		hdr_file
			<< "#ifndef " << m_hdr_guard
			<< "\n#define " << m_hdr_guard
			<< "\n\n"
			<< m_str_buffers[KRNL_DECL_BUF]
			<< "\n"
			<< "#endif /// " << m_hdr_guard;
	}

	void CodeGenerator::WriteKrnlSrcFile()
	{
		if (m_str_buffers[KRNL_DEF_BUF].empty())
			return;

		std::error_code err_code;
		llvm::raw_fd_ostream src_file(
			m_abs_paths[SRC_FILE_ABSPATH]
			, err_code
			, llvm::sys::fs::F_None);

		src_file
			<< "#include <math.h>\n"
			<< "#include <cuda_runtime.h>\n"
			<< "#include \"" << m_hdr_base_name << "\"\n\n"
			<< m_str_buffers[KRNL_DEF_BUF];
	}

	void CodeGenerator::HandleLoopNest(
		NestAnalysisDriver& analysis_driver
		, SymTblTree& sym_tbl_tree
		, const clang::FunctionDecl& original_fun)
	{
		std::string kernel_name =
			"__" + original_fun.getName().str() + "_c2cuda_kernel";

		InvocationStmtsGen invoc_stmts_gen(
			m_ast_ctx
			, analysis_driver
			, sym_tbl_tree
			, kernel_name);

		KernelDefGen kernel_def_gen(
			m_ast_ctx
			, analysis_driver
			, invoc_stmts_gen
			, kernel_name);

		ReplaceStmt(
			analysis_driver.GetTransformedNest().GetOutermostLoopHdr().GetStmt()
			, invoc_stmts_gen.GetGenBlock());

		m_str_buffers[KRNL_DECL_BUF].append("__global__ ");
		m_str_buffers[KRNL_DECL_BUF].append(StmtToStr(kernel_def_gen.GetKernelDecl()));
		m_str_buffers[KRNL_DEF_BUF].append("__global__ ");
		m_str_buffers[KRNL_DEF_BUF].append(StmtToStr(kernel_def_gen.GetKernelDef()));
	}

	void CodeGenerator::CompleteGeneration()
	{
		WriteMainFile();
		WriteKrnlHdrFile();
		WriteKrnlSrcFile();
	}

} /// namespace gap