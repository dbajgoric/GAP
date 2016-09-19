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

#ifndef GAP_FRONTEND_KERNEL_DEF_GEN_H
#define GAP_FRONTEND_KERNEL_DEF_GEN_H

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <unordered_map>
#include <vector>
#include <cassert>
#include "transformation.h"
#include "array_decl_info.h"

namespace gap
{

	/// Forward declaration
	class NestAnalysisDriver;
	class InvocationStmtsGen;

	/// Produces a kernel definition based on the transformed loop nest. The following segments
	/// are generated:
	///
	/// 1) Kernel function header with params declarations. In case of inner loop parallelizaton
	/// the first three params are the outermost loop index variable, its lower and upper bounds
	/// followed by the pointers to input and output device arrays follow
	/// 2) Declaration statements for each of the index variables of the parallel sub-nest are
	/// generated based on block idx, thread idx and block dim. The calculated value is translated
	/// so that the very first thread in each dimension will match the lower bound of the corresponding
	/// loop. Additionally, the lower and upper bound variables are defined for each loop in the
	/// parallel subnest and these variables are referenced where required (this minimizes the
	/// computation overhead by computing the bound expression only once per thread)
	/// 3) The index variables produced in step 2 are checked against the upper bounds of the loops
	/// and if any of these is greater-or-equal to the upper bound the given cuda thread terminates
	/// immediatelly
	/// 4)
	///
	/// IMPROVEMENTS:
	/// 1) discover which index exprs are identical and reuse them without recalculation for every array
	/// subscript
	/// 2) flat array subcripts are currently pulled out of the sequential inner nest IFF this nest has
	/// depth of 1 and the original array subscript doesn't reference the index variable of this nest.
	/// This condition can be relaxed though: the flat array subscript may be pulled out if the original
	/// subscript doesn't reference any of the index variables of the inner nest and the loop nest bounds
	/// of this nest excluding its outermost loop don't reference its index variables
	class KernelDefGen
	{
		class IdxVarInfo
		{
		public:

			IdxVarInfo(
				clang::VarDecl* host_idx_var
				, clang::VarDecl* dev_idx_var
				, clang::VarDecl* lower_bnd_var
				, clang::VarDecl* upper_bnd_var)

				: m_host_idx_var(host_idx_var)
				, m_dev_idx_var(dev_idx_var)
				, m_lower_bnd_var(lower_bnd_var)
				, m_upper_bnd_var(upper_bnd_var)
			{
			}

			clang::VarDecl* HstIdxVar() { return m_host_idx_var; }
			clang::VarDecl* DevIdxVar() { return m_dev_idx_var; }
			clang::VarDecl* LowBndVar() { return m_lower_bnd_var; }
			clang::VarDecl* UppBndVar() { return m_upper_bnd_var; }

		private:

			clang::VarDecl* m_host_idx_var;
			clang::VarDecl* m_dev_idx_var;
			clang::VarDecl* m_lower_bnd_var;
			clang::VarDecl* m_upper_bnd_var;
		};

		struct FlatArrSubIdx
		{
			clang::VarDecl* HstArrVar() { return m_host_arr_var; }
			clang::VarDecl* DevArrVar() { return m_dev_arr_var; }
			clang::VarDecl* FlatArrIdxVar() { return m_flat_arr_idx_var; }
			bool CanDefineOutOfSeqNest() { return m_can_def_outof_seq_nest; }

			clang::VarDecl* m_host_arr_var;
			clang::VarDecl* m_dev_arr_var;
			clang::VarDecl* m_flat_arr_idx_var;
			bool m_can_def_outof_seq_nest;
		};

		class DevArray
		{
		public:

			explicit DevArray(clang::VarDecl* dev_arr_var)
				: m_dev_arr_var(dev_arr_var)
			{
			}

			clang::VarDecl* DevArrVar() { return m_dev_arr_var; }
			void PushSizeVar(clang::VarDecl* size_var)
			{
				m_size_vars.push_back(size_var);
			}

			clang::VarDecl* GetSizeVar(std::size_t idx)
			{
				assert(idx < m_size_vars.size());
				return m_size_vars[idx];
			}

		private:

			clang::VarDecl* m_dev_arr_var;
			std::vector<clang::VarDecl*> m_size_vars;
		};

		typedef std::unordered_map<const clang::VarDecl*, DevArray> HostToDevArrMap;
		typedef std::unordered_map<clang::ArraySubscriptExpr*, FlatArrSubIdx> FlatArrSubIdxMap;
		typedef std::unordered_map<clang::VarDecl*, std::size_t> ArraySubExprCountMap;

	public:

		KernelDefGen(
			clang::ASTContext& ast_ctx
			, NestAnalysisDriver& analysis_driver
			, InvocationStmtsGen& invoc_stmts_gen
			, const std::string& kernel_name);

		clang::DeclStmt* GetKernelDecl();
		clang::DeclStmt* GetKernelDef();

	private:

		/// Builds a single thread index expression: blockIdx.x/y/z * blockDim.x/y/z + threadIdx.x/y/z
		clang::Expr* GetThreadOffsetExpr(clang::FieldDecl* dim3_field) const;
		IdxVarInfo CreateIdxVarInfo(
			clang::VarDecl* host_idx_var
			, clang::VarDecl* dev_idx_var
			, std::size_t loop_idx) const;

		FlatArrSubIdx GetFlatArrSubIndex(clang::ArraySubscriptExpr* arr_sub_expr);
		std::size_t GetSeqInnerNestDepth() const;
		std::string GetNextArraySubIdxVarName(clang::VarDecl* hst_arr_var);
		IdxVarInfo* FindIdxVarInfo(clang::VarDecl* host_idx_var);

		/// Sets the array decl to the device array decl and index expression to the calculated
		/// flat index expression for the given array subscript
		void TransformArraySubExprs();

		void GenKernelHdr();
		void GenCudaVarDecls();
		void GenDevIdxAndBoundVars();
		void GenParSubnestIdxVarAndBndDeclStmts();
		void GenUpperBndsCheckStmt();
		void GenFlatArraySubIndices();

		/// Independent flat array indices are those that can be defined outside the sequential
		/// inner nest (if it exists)
		void GenDeclStmtsForIndependentFlatArrIdxExprs();
		void GenSeqSubnestIdxVarDeclStmts();
		void GenSeqSubnest();
		void GenKernelDeclStmts();
		void CompleteKernelDefinition();
		void RunGenerator();

		clang::ASTContext& m_ast_ctx;
		NestAnalysisDriver& m_analysis_driver;
		InvocationStmtsGen& m_invoc_stmts_gen;
		std::string m_kernel_name;
		enum { KERNEL_DECL, KERNEL_DEF, KERNEL_ENUM_SIZE };
		clang::FunctionDecl* m_kernel_decl_def[KERNEL_ENUM_SIZE];
		clang::DeclStmt* m_kernel_decl_stmts[KERNEL_ENUM_SIZE];
		const Transformation m_transform;

		/// Maps the each host array accessed in the nest body to the corresponding device
		/// array that is passed and used in the kernel
		HostToDevArrMap m_host_dev_arr_map;
		std::vector<clang::Stmt*> m_body_stmts;
		std::vector<IdxVarInfo> m_idx_vars_info;
		FlatArrSubIdxMap m_flat_arr_sub_idx_map;
		ArraySubExprCountMap m_arr_sub_count_map;
		std::vector<clang::VarDecl*> m_seq_subnest_idx_vec;
		clang::ForStmt* m_seq_nest_innermost_loop;

		enum { BLOCK_IDX, BLOCK_DIM, THREAD_IDX, CUDA_VARS_COUNT = 3 };
		enum { X, Y, Z, DIM3_FIELDS_COUNT = 3 };
		clang::VarDecl* m_cuda_vars[CUDA_VARS_COUNT];
		clang::FieldDecl* m_dim3_fields[DIM3_FIELDS_COUNT];
	};

} /// namespace gap

#endif /// GAP_FRONTEND_KERNEL_DEF_GEN_H
