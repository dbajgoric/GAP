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

#ifndef GAP_FRONTEND_INVOCATION_STMTS_GEN_H
#define GAP_FRONTEND_INVOCATION_STMTS_GEN_H

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <vector>
#include <unordered_map>
#include "array_decl_info.h"

namespace gap
{

	/// Forward declarations
	class NestAnalysisDriver;
	class SymTblTree;

	typedef std::unordered_map<const clang::VarDecl*, ArrayDeclInfo> VarDeclArrInfoMap;

	/// Produces the following stmts based on the transformed loop nest and info
	/// about the applied transformation (InvocationStmtsGen got its name from the
	/// the fact that all this code is supporting the device kernel that is invoked):
	///
	/// 1) device memory allocation statements for the nest input and output arrays
	/// as well as temporary flattened host arrays allocation for the purpose of host
	/// to device memcpy
	/// 2) host to device memcpy stmts for the nest input arrays (for multidim
	/// arrays the mentioned flattened host array is populated first which is then
	/// copied to device memory in one go)
	/// 3) definitions for the block_dim and grid_dim variables which determine
	/// the kernel's execution configuration
	/// 4) device kernel call expression:
	/// - in case of outer loop parallelization and for nests where there is no
	/// loop carried dependence, only kernel call expression is generated followed
	/// by the cudaSync call
	/// - in case of inner loop parallelization, the outermost loop stmt is built
	/// with kernel call expression and cuda synchronization function within its
	/// body
	/// 5) device to host memcpy stmts for the nest output arrays (similar comment
	/// about temporary flattened host arrays like in host to device memcpy)
	/// 6) memfree stmts for device memory as well as temporary host arrays (if any)
	///
	/// FIXMEs and IMPROVEMENTS:
	/// 1) the original nest index variables have to be set to their final values
	/// after the parallelized nest has completed execution. This is easiest way to
	/// ensure that any reference to these variables after the nest won't be broken
	/// in parallelized code
	/// 2) In case any variables appear as part of assignment statements they have
	/// be passed to the kernel, otherwise kernel compilation will fail as these
	/// variables won't be defined. This can be avoided when such variable is a
	/// constant whose value can be deduced at compile time
	/// 3) CUDA functions return values should be checked and program should act in
	/// case of error (at the very least it should terminate on error)
	class InvocationStmtsGen
	{
	public:

		enum MemcpyDir { HOST_DEVICE, DEVICE_HOST };

		InvocationStmtsGen(
			clang::ASTContext& ast_ctx
			, NestAnalysisDriver& analysis_driver
			, SymTblTree& sym_tbl_tree
			, const std::string& kernel_name);

		clang::CompoundStmt* GetGenBlock();
		VarDeclArrInfoMap& GetHostVarArrDeclInfoMap();

	private:

		ArrayLikeIdentifier* FindArrayIdWithSpecifiedDims(
			const clang::Stmt* nest_enclosing_scope
			, const std::string& array_name);

		/// Creates a vector of expressions representing the list of kernel arguments. The kernel
		/// args always have the following order: (i0, dev_arrs_and_host_arr_size) where i0 is the
		/// outermost loop index variable in the case of inner loop parallelization. Note that size
		/// of the original host array is passed together with each dev array. The size is required
		/// when computing flat array index expressions
		void BuildKernelArgList(std::vector<clang::Expr*>& kernel_arg_list);

		void GenMemAllocStmtsHelper(
			const std::vector<clang::VarDecl*>& array_var_decls
			, VarDeclArrInfoMap& var_decl_arr_info_map);

		void GenMemAllocStmts();
		void GenDataMemcpyHelper(MemcpyDir memcpy_dir);
		void GenHostToDevDataMemcpy();
		void GenDevToHostDataMempcy();
		void GenBlockGridDimVarDecls();
		void GenKernelCallSegment();
		void GenMemFreeStmtsHelper(
			clang::FunctionDecl* free_fn_decl
			, clang::FunctionDecl* cudafree_fn_decl
			, VarDeclArrInfoMap& var_decl_arr_info_map);

		void GenMemFreeStmts();
		void CompleteGeneration();
		void RunGenerator();

		clang::ASTContext& m_ast_ctx;
		NestAnalysisDriver& m_analysis_driver;
		SymTblTree& m_sym_tbl_tree;
		std::vector<clang::Stmt*> m_produced_stmts;
		std::string m_kernel_name;
		clang::CompoundStmt* m_generated_block;

		/// Maps each input/output array to a collection of relevant information
		VarDeclArrInfoMap m_input_host_array_info_map;
		VarDeclArrInfoMap m_output_host_array_info_map;
		/// This map merges the two maps below removing any duplicates
		VarDeclArrInfoMap m_host_array_info_map;

		/// Kernel configuration variables - block and grid dimension
		enum { BLOCK_DIM, GRID_DIM, KERNEL_CONFIG_SIZE = 2 };
		clang::VarDecl* m_kernel_config[KERNEL_CONFIG_SIZE];

		enum { CUDA_DEV_SYNC, CUDA_PEEK_ERR, CUDA_FNS_SIZE = 2 };
		clang::CallExpr* m_cuda_fns[CUDA_FNS_SIZE];
	};

} /// namespace gap

#endif /// GAP_FRONTEND_INVOCATION_STMTS_GEN_H
