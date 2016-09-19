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

#include "invocation_stmts_gen.h"
#include "perfect_loop_nest.h"
#include "nest_analysis_driver.h"
#include "sym_tbl_tree.h"
#include "ast_helpers.h"
#include "clang_allocator.h"
#include <clang/AST/ExprCXX.h>


namespace gap
{
	namespace
	{

		/// Builds a memcpy call expression
		clang::CallExpr* BuildMemcpyCallExpr(
			clang::ASTContext& ast_ctx
			, clang::Expr* dest_expr
			, clang::Expr* src_expr
			, clang::Expr* byte_size)
		{
			clang::FunctionDecl* memcpy_fndecl = NoLoadLookup<clang::FunctionDecl>(
				ast_ctx
				, *ast_ctx.getTranslationUnitDecl()
				, "memcpy");

			assert(memcpy_fndecl != nullptr && "unexpected lookup failure");
			return
				CreateCallExpr(
					ast_ctx
					, CreateDeclRefExpr(ast_ctx, memcpy_fndecl, memcpy_fndecl->getType())
					, { dest_expr, src_expr, byte_size }
			, memcpy_fndecl->getType());
		}

		clang::Stmt* BuildCudaMemcpyExpr(
			clang::ASTContext& ast_ctx
			, clang::VarDecl* dest_var
			, clang::VarDecl* src_var
			, clang::Expr* byte_size
			, const std::string& dir_str)
		{
			clang::FunctionDecl* cuda_memcpy_fndecl = NoLoadLookup<clang::FunctionDecl>(
				ast_ctx
				, *ast_ctx.getTranslationUnitDecl()
				, "cudaMemcpy");

			clang::EnumConstantDecl* dir_enum_constant = NoLoadLookup<clang::EnumConstantDecl>(
				ast_ctx
				, *ast_ctx.getTranslationUnitDecl()
				, dir_str);

			assert(cuda_memcpy_fndecl != nullptr && "cudaMemcpy should be in scope at this point");
			assert(dir_enum_constant != nullptr && "direction enum constant must be in scope");

			return
				CreateCallExpr(
					ast_ctx
					, CreateDeclRefExpr(ast_ctx, cuda_memcpy_fndecl, cuda_memcpy_fndecl->getType())
					,
					{
						CreateDeclRefExpr(ast_ctx, dest_var, dest_var->getType())
						, CreateDeclRefExpr(ast_ctx, src_var, src_var->getType())
					, byte_size
					, CreateDeclRefExpr(ast_ctx, dir_enum_constant, dir_enum_constant->getType())
					}
			, cuda_memcpy_fndecl->getReturnType());
		}

		clang::Expr* BuildFlatDimExpr(clang::ASTContext& ast_ctx, ArrayLikeIdentifier& arr_id)
		{
			auto sizeof_expr =
				CreateUnaryOrTypeTraitExpr(
					ast_ctx
					, clang::UETT_SizeOf
					, ast_ctx.getTrivialTypeSourceInfo(arr_id.GetBaseElemType())
					, ast_ctx.getSizeType());

			if (arr_id.GetDimensionality() == 1)
				return
				CreateBinOp(
					ast_ctx
					, arr_id.GetSize(0)
					, sizeof_expr
					, clang::BO_Mul
					, ast_ctx.getSizeType());

			ClangAllocator allocator(ast_ctx);
			clang::BinaryOperator* dim_expr = CreateBinOp(
				ast_ctx
				, arr_id.GetSize(0)
				, allocator.Alloc<clang::GNUNullExpr>(clang::Stmt::EmptyShell())
				, clang::BO_Mul
				, arr_id.GetSize(0)->getType());

			for (std::size_t i = 1; i < arr_id.GetDimensionality(); ++i)
			{
				dim_expr->setRHS(arr_id.GetSize(i));
				if (i < arr_id.GetDimensionality() - 1)
					dim_expr = CreateBinOp(
						ast_ctx
						, dim_expr
						, allocator.Alloc<clang::GNUNullExpr>(clang::Stmt::EmptyShell())
						, clang::BO_Mul
						, dim_expr->getType());
			}
			return CreateBinOp(
				ast_ctx
				, dim_expr
				, sizeof_expr
				, clang::BO_Mul
				, ast_ctx.getSizeType());
		}

		clang::CallExpr* GetCudaMallocCallExpr(
			clang::ASTContext& ast_ctx
			, ArrayLikeIdentifier& arr_id
			, clang::Expr* flat_dim_expr
			, clang::VarDecl* device_array)
		{
			clang::FunctionDecl* cuda_malloc_fndecl = NoLoadLookup<clang::FunctionDecl>(
				ast_ctx
				, *ast_ctx.getTranslationUnitDecl()
				, "cudaMalloc");

			assert(cuda_malloc_fndecl != nullptr && "cudaMalloc must be in scope at this point");
			return
				CreateCallExpr(
					ast_ctx
					, CreateDeclRefExpr(ast_ctx, cuda_malloc_fndecl, cuda_malloc_fndecl->getType())
					,
					{
						CreateUnaryOp(
						ast_ctx
							, CreateDeclRefExpr(ast_ctx, device_array, device_array->getType())
							, clang::UO_AddrOf
							, ast_ctx.getPointerType(device_array->getType()))

					, flat_dim_expr
					}
			, cuda_malloc_fndecl->getReturnType());
		}

		clang::DeclStmt* GetMallocDeclStmt(
			clang::ASTContext& ast_ctx
			, ArrayLikeIdentifier& arr_id
			, clang::Expr* flat_dim_expr)
		{
			clang::VarDecl* tmp_var = CreateVarDecl(
				ast_ctx
				, "__tmp_" + arr_id.GetName()
				, ast_ctx.getPointerType(arr_id.GetBaseElemType()));

			clang::FunctionDecl* malloc_fndecl = NoLoadLookup<clang::FunctionDecl>(
				ast_ctx
				, *ast_ctx.getTranslationUnitDecl()
				, "malloc");

			assert(malloc_fndecl != nullptr && "malloc must be in scope at this point");
			tmp_var->setInit(
				CreateCStyleCastExpr(
					ast_ctx
					, tmp_var->getType()
					, clang::CK_Dependent
					, CreateCallExpr(
						ast_ctx
						, CreateDeclRefExpr(ast_ctx, malloc_fndecl, malloc_fndecl->getType())
						, { flat_dim_expr }
			, malloc_fndecl->getReturnType())));

			return CreateDeclStmt(ast_ctx, std::vector<clang::VarDecl*>({ tmp_var }));
		}

		void BuildLoopIdxVarsDecls(
			clang::ASTContext& ast_ctx
			, std::size_t count
			, clang::QualType var_type
			, std::vector<clang::VarDecl*>& var_decls
			, const std::string& base_name = "i")
		{
			for (std::size_t i = 0; i < count; ++i)
				var_decls.push_back(
					CreateVarDecl(
						ast_ctx
						, "__" + base_name + std::to_string(i)
						, var_type));
		}

		/// Produces a linear expression involving the tmp flat host array, that relates
		/// this array to the multidim host array. When multidim host array is flattened
		/// this expr represents the offset in the tmp host array where data should be 
		/// copied to, and when produced 1D result is being copied back to original
		/// multidim array this expr represents the offset in the tmp array from where
		/// the data should be taken in each nest iteration
		clang::Expr* BuildTmpHostArrayOffsetExpr(
			clang::ASTContext& ast_ctx
			, ArrayDeclInfo& arr_decl_info
			, const std::vector<clang::VarDecl*>& loop_idx_vec)
		{
			assert(arr_decl_info.ArrayId().GetDimensionality() > 1);
			clang::FunctionDecl* memcpy_fndecl = NoLoadLookup<clang::FunctionDecl>(
				ast_ctx
				, *ast_ctx.getTranslationUnitDecl()
				, "memcpy");

			clang::BinaryOperator* dest_expr = nullptr;
			std::size_t dim = arr_decl_info.ArrayId().GetDimensionality();
			ClangAllocator allocator(ast_ctx);
			for (std::size_t i = 0; i < dim - 1; ++i)
			{
				clang::QualType idx_var_type = loop_idx_vec[i]->getType();
				clang::BinaryOperator* mul_op = CreateBinOp(
					ast_ctx
					, CreateDeclRefExpr(ast_ctx, loop_idx_vec[i], idx_var_type)
					, arr_decl_info.ArrayId().GetSize(i + 1)
					, clang::BO_Mul
					, idx_var_type);

				for (std::size_t j = i + 2; j < dim; ++j)
					mul_op = CreateBinOp(
						ast_ctx
						, mul_op
						, arr_decl_info.ArrayId().GetSize(j)
						, clang::BO_Mul
						, idx_var_type);

				if (dest_expr == nullptr)
					dest_expr = mul_op;
				else
					dest_expr = CreateBinOp(
						ast_ctx
						, dest_expr
						, mul_op
						, clang::BO_Add
						, idx_var_type);
			}

			return
				CreateBinOp(
					ast_ctx
					, CreateDeclRefExpr(ast_ctx, arr_decl_info.TmpHostVar(), arr_decl_info.TmpHostVar()->getType())
					, dest_expr
					, clang::BO_Add
					, arr_decl_info.TmpHostVar()->getType());
		}

		/// Builds an array subscript expression for the given var decl and the vector of
		/// loop index variables. It is required to specify how many of these variables
		/// should be used
		clang::ArraySubscriptExpr* BuildArrSubForLoopIdxVec(
			clang::ASTContext& ast_ctx
			, clang::VarDecl* var_decl
			, const std::vector<clang::VarDecl*>& loop_idx_vec
			, std::size_t array_sub_depth)
		{
			assert(array_sub_depth < loop_idx_vec.size());
			std::vector<clang::Expr*> decl_ref_exprs;
			decl_ref_exprs.reserve(array_sub_depth);

			for (std::size_t i = 0; i < array_sub_depth; ++i)
				decl_ref_exprs.push_back(
					CreateDeclRefExpr(ast_ctx, loop_idx_vec[i], loop_idx_vec[i]->getType()));

			return
				CreateArrSubExpr(
					ast_ctx
					, var_decl
					, decl_ref_exprs
					, ast_ctx.getPointerType(ast_ctx.getBaseElementType(var_decl->getType())));
		}

		/// Produces an expression that represents a byte size of the innermost array dimension.
		/// This expr is required when copying multidim host array to 1D tmp host array and the
		/// other way around
		clang::Expr* InnermostDimSizeToByteCount(
			clang::ASTContext& ast_ctx
			, ArrayLikeIdentifier& array_id)
		{
			return
				CreateBinOp(
					ast_ctx
					, array_id.GetSize(array_id.GetDimensionality() - 1)
					, CreateUnaryOrTypeTraitExpr(
						ast_ctx
						, clang::UETT_SizeOf
						, ast_ctx.getTrivialTypeSourceInfo(array_id.GetBaseElemType())
						, ast_ctx.getSizeType())

					, clang::BO_Mul
					, ast_ctx.getSizeType());
		}

		/// In case the host array has 2 or more dimensions, it is flattened by copying it
		/// to a temporary 1D host array that is then copied in one go to the device memory.
		/// The similar procedure is performed when copying 1D tmp host array holding the
		/// result of kernel execution back to multidim host array
		clang::ForStmt* BuildFlatteningNestHelper(
			clang::ASTContext& ast_ctx
			, ArrayDeclInfo& arr_decl_info
			, std::vector<clang::VarDecl*>& loop_idx_vec
			, clang::Expr* memcpy_flattening_expr)
		{
			clang::ForStmt* outermost_loop = nullptr;
			clang::ForStmt* curr_loop = nullptr;
			auto array_dim = arr_decl_info.ArrayId().GetDimensionality();
			for (std::size_t i = 0; i < array_dim - 1; ++i)
			{
				clang::QualType idx_var_type = loop_idx_vec[i]->getType();
				clang::ForStmt* tmp_for_stmt =
					CreateForStmt(
						ast_ctx
						, CreateBinOp(
							ast_ctx
							, CreateDeclRefExpr(ast_ctx, loop_idx_vec[i], idx_var_type)
							, CreateIntLiteral(ast_ctx, GetUnsigned(ast_ctx.getIntWidth(idx_var_type), 0), idx_var_type)
							, clang::BO_Assign
							, idx_var_type)

						, CreateBinOp(
							ast_ctx
							, CreateDeclRefExpr(ast_ctx, loop_idx_vec[i], idx_var_type)
							, arr_decl_info.ArrayId().GetSize(i)
							, clang::BO_LT
							, ast_ctx.BoolTy)

						, CreateUnaryOp(
							ast_ctx
							, CreateDeclRefExpr(ast_ctx, loop_idx_vec[i], idx_var_type)
							, clang::UO_PreInc
							, idx_var_type)

						, nullptr);

				if (outermost_loop == nullptr)
				{
					outermost_loop = curr_loop = tmp_for_stmt;
					continue;
				}

				curr_loop->setBody(tmp_for_stmt);
				curr_loop = tmp_for_stmt;
			}

			/// curr_loop is innermost loop at this point
			curr_loop->setBody(memcpy_flattening_expr);
			return outermost_loop;
		}

		clang::ForStmt* BuildFlatteningNest(
			clang::ASTContext& ast_ctx
			, ArrayDeclInfo& arr_decl_info
			, std::vector<clang::VarDecl*>& loop_idx_vec
			, InvocationStmtsGen::MemcpyDir memcpy_dir)
		{
			clang::Expr* tmp_arr_offset = BuildTmpHostArrayOffsetExpr(ast_ctx, arr_decl_info, loop_idx_vec);
			clang::ArraySubscriptExpr* host_arr_sub =
				BuildArrSubForLoopIdxVec(
					ast_ctx
					, &arr_decl_info.HostVar()
					, loop_idx_vec
					, arr_decl_info.ArrayId().GetDimensionality() - 1);

			clang::Expr* byte_count = InnermostDimSizeToByteCount(ast_ctx, arr_decl_info.ArrayId());
			return
				BuildFlatteningNestHelper(
					ast_ctx
					, arr_decl_info
					, loop_idx_vec
					, BuildMemcpyCallExpr(
						ast_ctx
						, memcpy_dir == InvocationStmtsGen::HOST_DEVICE ? tmp_arr_offset : host_arr_sub
						, memcpy_dir == InvocationStmtsGen::HOST_DEVICE ? host_arr_sub : tmp_arr_offset
						, byte_count));
		}

		/// This method uses the following reasoning: if the nest contains only one loop
		/// that carries no dependence assign 512 threads per block x dim, if there are
		/// are two such loops assign 32 and 16 threads to block x and y dim, and if there
		/// are three such loops assign 8, 8 and 8 threads to x, y and z dim
		clang::VarDecl* BuildBlockDimVarDecl(
			clang::ASTContext& ast_ctx
			, clang::RecordDecl* dim3_struct
			, const Transformation& transform)
		{
			std::vector<clang::Expr*> init_exprs;
			std::size_t dep_free_loops_count = transform.GetDepFreeLoopsCnt();
			assert(
				dep_free_loops_count > 0
				&& "this point should never be reached for a nest where each loop carries a dependence");

			std::vector<std::size_t> thread_cnts;
			if (dep_free_loops_count == 1)
				thread_cnts.push_back(512);
			else if (dep_free_loops_count == 2)
				thread_cnts = { 32, 16 };
			else
				thread_cnts = { 8, 8, 8 };

			for (auto thread_cnt : thread_cnts)
				init_exprs.push_back(
					CreateIntLiteral(
						ast_ctx
						, GetUnsigned(ast_ctx.getIntWidth(ast_ctx.UnsignedIntTy), thread_cnt)
						, ast_ctx.UnsignedIntTy));

			return CreateVarDecl(
				ast_ctx
				, "__block_dim"
				, ast_ctx.getRecordType(dim3_struct)
				, CreateInitListExpr(ast_ctx, init_exprs));
		}

		/// Produces an expression that represents the number of iterations of the loop.
		/// Note that function assumes that loop has '<' conditional operator so loop range
		/// is simply upp_bnd - low_bnd. Also note that this expression will always evaluate
		/// to non-negative number and is cast to unsigned type
		clang::Expr* BuildLoopIterCountExpr(
			clang::ASTContext& ast_ctx
			, clang::ForStmt* for_stmt)
		{
			auto init_op = clang::dyn_cast<clang::BinaryOperator>(for_stmt->getInit());
			auto cond_op = clang::dyn_cast<clang::BinaryOperator>(for_stmt->getCond());
			assert(
				init_op != nullptr
				&& cond_op != nullptr
				&& "should never reach this point as this for loop header is unsupported");

			assert(
				init_op->getOpcode() == clang::BO_Assign
				&& cond_op->getOpcode() == clang::BO_LT
				&& "should never reach this point as this for loop header is unsupported");

			auto idx_var = GetVarDecl(clang::cast<clang::DeclRefExpr>(init_op->getLHS()));
			return
				CreateCStyleCastExpr(
					ast_ctx
					, ast_ctx.getIntTypeForBitwidth(ast_ctx.getIntWidth(idx_var->getType()), 0)
					, clang::CK_IntegralCast
					, CreateParenExpr(
						ast_ctx
						, CreateBinOp(
							ast_ctx
							, cond_op->getRHS()
							, init_op->getRHS()
							, clang::BO_Sub
							, init_op->getRHS()->getType())));
		}

		/// Produces an expression that represents a number of thread blocks along a single
		/// grid dimension: (loop_iter_count + block_dim.(x/y/z) - 1) / block_dim.(x/y/z)
		clang::BinaryOperator* BuildSingleGridDimExpr(
			clang::ASTContext& ast_ctx
			, clang::Expr* loop_iter_count
			, clang::VarDecl* block_dim_var
			, clang::FieldDecl* block_dim_field)
		{
			auto block_dim_mem_expr = CreateMemberExpr(
				ast_ctx
				, CreateDeclRefExpr(ast_ctx, block_dim_var, block_dim_var->getType())
				, false
				, block_dim_field);

			assert(block_dim_mem_expr != nullptr);
			return
				CreateBinOp(
					ast_ctx
					, CreateParenExpr(
						ast_ctx
						, CreateBinOp(
							ast_ctx
							, CreateBinOp(
								ast_ctx
								, loop_iter_count
								, block_dim_mem_expr
								, clang::BO_Add
								, loop_iter_count->getType())

							, CreateIntLiteral(ast_ctx, GetUnsigned(ast_ctx.getIntWidth(block_dim_field->getType()), 1), block_dim_field->getType())
							, clang::BO_Sub
							, loop_iter_count->getType()))

					, block_dim_mem_expr
					, clang::BO_Div
					, loop_iter_count->getType());
		}

		/// The innermost loop (of the sub-nest with no carried dependences) is always
		/// mapped to the x-dim, the outer loop to y-dim and so on. Thus, parallelizable
		/// sub-nest is mapped in reverse to CUDA thread dimensions
		clang::VarDecl* BuildGridDimVarDecl(
			clang::ASTContext& ast_ctx
			, clang::RecordDecl* dim3_struct
			, clang::VarDecl* block_dim_var
			, PerfectLoopNest& nest
			, const Transformation& transform)
		{
			std::size_t dep_free_loops_count = transform.GetDepFreeLoopsCnt();
			Transformation::TransformType transform_type = transform.GetTransformType();
			ForLoopHeader* dep_free_subnest_loop = nullptr;

			if (transform_type == Transformation::TRANSFORM_OUTER_PAR)
				dep_free_subnest_loop = nest.GetLoopHdr(dep_free_loops_count - 1);
			else
				dep_free_subnest_loop == nest.GetLoopHdr(nest.GetNestDepth() - 1);

			std::size_t i = 0;
			const char* block_dim_field_names[] = { "x", "y", "z" };
			std::vector<clang::Expr*> grid_dim_exprs;
			grid_dim_exprs.reserve(dep_free_loops_count);
			/// Parallelizable sub nests with up to 3 loops are currently supported
			while (i < dep_free_loops_count && i < 4)
			{
				assert(dep_free_subnest_loop != nullptr && "should never happen");
				auto loop_iter_count = BuildLoopIterCountExpr(ast_ctx, dep_free_subnest_loop->GetStmt());
				auto block_dim_field =
					NoLoadLookup<clang::FieldDecl>(
						ast_ctx
						, *dim3_struct
						, block_dim_field_names[i]);

				assert(block_dim_field != nullptr);
				grid_dim_exprs.push_back(
					BuildSingleGridDimExpr(
						ast_ctx
						, loop_iter_count
						, block_dim_var
						, block_dim_field));

				dep_free_subnest_loop = dep_free_subnest_loop->GetParent();
				++i;
			}
			/// Fill the unused grid dimensions with ones to explicitly initialize them
			for (std::size_t i = grid_dim_exprs.size(); i < 3; ++i)
				grid_dim_exprs.push_back(
					CreateIntLiteral(ast_ctx, GetUnsigned(ast_ctx.getIntWidth(ast_ctx.UnsignedIntTy), 1), ast_ctx.UnsignedIntTy));

			return CreateVarDecl(
				ast_ctx
				, "__grid_dim"
				, ast_ctx.getRecordType(dim3_struct)
				, CreateInitListExpr(ast_ctx, grid_dim_exprs));
		}

		/// Creates a dummy kernel function declaration without specifying its params,
		/// used to generate kernel call expression
		clang::FunctionDecl* CreateDummyKernelDecl(
			clang::ASTContext& ast_ctx
			, const std::string& kernel_name)
		{
			return
				CreateFunDecl(
					ast_ctx
					, kernel_name
					, ast_ctx.VoidTy
					, {}
			, {});
		}

		/// Copies the outermost loop of the given nest and fills its body with the list of
		/// provided statements
		clang::ForStmt* BuildSnglLoopUsingOutermostLoop(
			clang::ASTContext& ast_ctx
			, clang::ForStmt* outermost_loop
			, const std::vector<clang::Stmt*>& stmts)
		{
			return CreateForStmt(
				ast_ctx
				, outermost_loop->getInit()
				, outermost_loop->getCond()
				, outermost_loop->getInc()
				, CreateCmpndStmt(
					ast_ctx
					, stmts));
		}

		clang::CallExpr* GetCallExpr(
			clang::ASTContext& ast_ctx
			, const std::string& fn_name
			, const std::vector<clang::Expr*>& args = {})
		{
			auto fn_decl = NoLoadLookup<clang::FunctionDecl>(
				ast_ctx
				, *ast_ctx.getTranslationUnitDecl()
				, fn_name);

			assert(fn_decl != nullptr && "unexpected decl lookup failure");
			return CreateCallExpr(
				ast_ctx
				, CreateDeclRefExpr(ast_ctx, fn_decl, fn_decl->getType())
				, args
				, fn_decl->getType());
		}

		/// Returns cudaMemcpy source var decl based on the memcpy direction and the host
		/// array dimension
		clang::VarDecl* GetCudaMemcpySrcVarDecl(
			InvocationStmtsGen::MemcpyDir memcpy_dir
			, ArrayDeclInfo& array_decl_info)
		{
			if (memcpy_dir == InvocationStmtsGen::HOST_DEVICE)
				return
				array_decl_info.ArrayId().GetDimensionality() == 1
				? &array_decl_info.HostVar()
				: array_decl_info.TmpHostVar();

			return &array_decl_info.DeviceVar();
		}

		/// Returns cudaMemcpy destination var decl based on the memcpy direction and the host
		/// array dimension
		clang::VarDecl* GetCudaMemcpyDestVarDecl(
			InvocationStmtsGen::MemcpyDir memcpy_dir
			, ArrayDeclInfo& array_decl_info)
		{
			if (memcpy_dir == InvocationStmtsGen::DEVICE_HOST)
				return
				array_decl_info.ArrayId().GetDimensionality() == 1
				? &array_decl_info.HostVar()
				: array_decl_info.TmpHostVar();

			return &array_decl_info.DeviceVar();
		}

		std::string CudaMemcpyDirToStr(InvocationStmtsGen::MemcpyDir memcpy_dir)
		{
			return
				memcpy_dir == InvocationStmtsGen::HOST_DEVICE
				? "cudaMemcpyHostToDevice"
				: "cudaMemcpyDeviceToHost";
		}

	} /// Anonymous namespace


	InvocationStmtsGen::InvocationStmtsGen(
		clang::ASTContext& ast_ctx
		, NestAnalysisDriver& analysis_driver
		, SymTblTree& sym_tbl_tree
		, const std::string& kernel_name)

		: m_ast_ctx(ast_ctx)
		, m_analysis_driver(analysis_driver)
		, m_sym_tbl_tree(sym_tbl_tree)
		, m_kernel_name(kernel_name)
		, m_generated_block(nullptr)
	{
		assert(
			m_analysis_driver.GetTransformedNest().GetNestDepth() <= 4
			&& "nests with depth > 4 are currently not supported");

		std::vector<std::string> cuda_fns
		{
			"cudaDeviceSynchronize"
			, "cudaPeekAtLastError"
		};
		assert(cuda_fns.size() == CUDA_FNS_SIZE);
		for (std::size_t i = CUDA_DEV_SYNC; i < CUDA_FNS_SIZE; ++i)
			m_cuda_fns[i] = GetCallExpr(m_ast_ctx, cuda_fns[i]);

		/// Run the generator
		RunGenerator();
	}

	ArrayLikeIdentifier* InvocationStmtsGen::FindArrayIdWithSpecifiedDims(
		const clang::Stmt* nest_enclosing_scope
		, const std::string& array_name)
	{
		auto id = m_sym_tbl_tree.FindIdentifier(nest_enclosing_scope, array_name);
		auto array_id = CastAsArrayId(id);
		assert(array_id != nullptr && "this is a bug");
		if (!array_id->HasSizeForEachDim())
			throw std::runtime_error(
				"nest references the '"
				+ array_id->GetName()
				+ "' variable that has at least one unknown dimension length");

		return array_id;
	}

	void InvocationStmtsGen::BuildKernelArgList(std::vector<clang::Expr*>& kernel_arg_list)
	{
		auto transform_type = m_analysis_driver.GetTransformation().GetTransformType();
		auto & nest_idx_vec = m_analysis_driver.GetNewIdxVec();

		if (transform_type == Transformation::TRANSFORM_INNER_PAR)
		{
			/// Outermost loop index variable is kernel's first arg
			kernel_arg_list =
			{
				CreateDeclRefExpr(m_ast_ctx, nest_idx_vec[0], nest_idx_vec[0]->getType())
			};
		}
		for (auto & host_arr_info_pair : m_host_array_info_map)
		{
			/// Pass in a pointer to device array decl to the kernel
			clang::VarDecl* dev_arr_decl = &host_arr_info_pair.second.DeviceVar();
			kernel_arg_list.push_back(
				CreateDeclRefExpr(
					m_ast_ctx
					, dev_arr_decl
					, dev_arr_decl->getType()));

			/// Followed by the list of args representing its size along each dimension
			auto & array_id = host_arr_info_pair.second.ArrayId();
			for (std::size_t i = 0; i < array_id.GetDimensionality(); ++i)
				kernel_arg_list.push_back(array_id.GetSize(i));
		}
	}

	void InvocationStmtsGen::GenMemAllocStmtsHelper(
		const std::vector<clang::VarDecl*>& array_var_decls
		, VarDeclArrInfoMap& var_decl_arr_info_map)
	{
		PerfectLoopNest& nest = m_analysis_driver.GetTransformedNest();
		auto node_bool_pair = m_sym_tbl_tree.Search(nest.GetOutermostLoopHdr().GetStmt());
		assert(node_bool_pair.second && "tree search failed unexecpectedly");
		auto parent_node = node_bool_pair.first->GetParentNode();
		assert(parent_node != nullptr && "loop nest must have a parent scope");

		for (auto array_decl : array_var_decls)
		{
			auto array_id = FindArrayIdWithSpecifiedDims(parent_node->GetScopeStmt(), array_decl->getName().str());
			auto flat_dim_expr = BuildFlatDimExpr(m_ast_ctx, *array_id);
			auto device_array = CreateVarDecl(
				m_ast_ctx
				, "__dev_" + array_id->GetName()
				, m_ast_ctx.getPointerType(array_id->GetBaseElemType()));

			m_produced_stmts.push_back(CreateDeclStmt(m_ast_ctx, std::vector<clang::VarDecl*>({ device_array })));
			m_produced_stmts.push_back(GetCudaMallocCallExpr(m_ast_ctx, *array_id, flat_dim_expr, device_array));

			clang::VarDecl* tmp_host_var_decl = nullptr;
			if (array_id->GetDimensionality() > 1)
			{
				auto tmp_decl_stmt = GetMallocDeclStmt(m_ast_ctx, *array_id, flat_dim_expr);
				m_produced_stmts.push_back(tmp_decl_stmt);
				tmp_host_var_decl = clang::cast<clang::VarDecl>(*tmp_decl_stmt->getDeclGroup().begin());
			}

			var_decl_arr_info_map.insert(
				std::make_pair(
					array_decl
					, ArrayDeclInfo(
						*array_id
						, *array_decl
						, *device_array
						, tmp_host_var_decl
						, *flat_dim_expr)));
		}
	}

	void InvocationStmtsGen::GenMemAllocStmts()
	{
		auto & nest_in_vars = m_analysis_driver.GetTransformedNest().GetNestInputVars();
		GenMemAllocStmtsHelper(
			nest_in_vars
			, m_input_host_array_info_map);

		m_host_array_info_map = m_input_host_array_info_map;
		auto & nest_out_vars = m_analysis_driver.GetTransformedNest().GetNestOutputVars();
		std::vector<clang::VarDecl*> only_out_vars;
		for (auto out_var : nest_out_vars)
		{
			if (std::find(nest_in_vars.begin(), nest_in_vars.end(), out_var) != nest_in_vars.end())
				m_output_host_array_info_map.insert(*m_input_host_array_info_map.find(out_var));
			else
				only_out_vars.push_back(out_var);
		}

		GenMemAllocStmtsHelper(
			only_out_vars
			, m_output_host_array_info_map);

		for (auto out_var : only_out_vars)
			m_host_array_info_map.insert(*m_output_host_array_info_map.find(out_var));
	}

	void InvocationStmtsGen::GenDataMemcpyHelper(MemcpyDir memcpy_dir)
	{
		auto & array_decl_info_map =
			memcpy_dir == HOST_DEVICE
			? m_input_host_array_info_map
			: m_output_host_array_info_map;

		for (auto & var_decl_info_pair : array_decl_info_map)
		{
			ArrayDeclInfo& arr_decl_info = var_decl_info_pair.second;
			if (arr_decl_info.ArrayId().GetDimensionality() == 1)
			{
				m_produced_stmts.push_back(
					BuildCudaMemcpyExpr(
						m_ast_ctx
						, GetCudaMemcpyDestVarDecl(memcpy_dir, arr_decl_info)
						, GetCudaMemcpySrcVarDecl(memcpy_dir, arr_decl_info)
						, &arr_decl_info.FlatDimExpr()
						, CudaMemcpyDirToStr(memcpy_dir)));

				continue;
			}

			std::vector<clang::Stmt*> stmts;
			std::vector<clang::VarDecl*> loop_idx_vec;
			std::size_t dim = arr_decl_info.ArrayId().GetDimensionality();
			loop_idx_vec.reserve(dim - 1);

			BuildLoopIdxVarsDecls(m_ast_ctx, dim - 1, m_ast_ctx.getSizeType(), loop_idx_vec);
			auto loop_vec_decl_stmt = CreateDeclStmt(m_ast_ctx, loop_idx_vec);
			auto flattening_nest = BuildFlatteningNest(m_ast_ctx, arr_decl_info, loop_idx_vec, memcpy_dir);
			auto cuda_memcpy_expr =
				BuildCudaMemcpyExpr(
					m_ast_ctx
					, GetCudaMemcpyDestVarDecl(memcpy_dir, arr_decl_info)
					, GetCudaMemcpySrcVarDecl(memcpy_dir, arr_decl_info)
					, &arr_decl_info.FlatDimExpr()
					, CudaMemcpyDirToStr(memcpy_dir));

			if (memcpy_dir == HOST_DEVICE)
				stmts = { loop_vec_decl_stmt, flattening_nest, cuda_memcpy_expr };
			else
				stmts = { cuda_memcpy_expr, loop_vec_decl_stmt, flattening_nest };

			m_produced_stmts.push_back(
				CreateCmpndStmt(m_ast_ctx, stmts));
		}

		/// Add cudaDeviceSync call after the last cudaMemcpy as copying memory from
		/// host to device is not synchronous
		m_produced_stmts.push_back(m_cuda_fns[CUDA_DEV_SYNC]);
	}

	void InvocationStmtsGen::GenHostToDevDataMemcpy()
	{
		GenDataMemcpyHelper(HOST_DEVICE);
	}

	void InvocationStmtsGen::GenDevToHostDataMempcy()
	{
		GenDataMemcpyHelper(DEVICE_HOST);
	}

	void InvocationStmtsGen::GenBlockGridDimVarDecls()
	{
		auto dim3_struct_decl = NoLoadLookup<clang::RecordDecl>(
			m_ast_ctx
			, *m_ast_ctx.getTranslationUnitDecl()
			, "dim3");

		assert(dim3_struct_decl != nullptr && "dim3 struct must be in scope");
		m_kernel_config[BLOCK_DIM] = BuildBlockDimVarDecl(
			m_ast_ctx
			, dim3_struct_decl
			, m_analysis_driver.GetTransformation());

		m_kernel_config[GRID_DIM] = BuildGridDimVarDecl(
			m_ast_ctx
			, dim3_struct_decl
			, m_kernel_config[BLOCK_DIM]
			, m_analysis_driver.GetTransformedNest()
			, m_analysis_driver.GetTransformation());
	}

	void InvocationStmtsGen::GenKernelCallSegment()
	{
		auto dummy_kernel_decl = CreateDummyKernelDecl(m_ast_ctx, m_kernel_name);
		Transformation::TransformType transform_type = m_analysis_driver.GetTransformation().GetTransformType();

		/// Decl stmt for block_dim and grid_dim has not yet been added
		std::vector<clang::Stmt*> stmts
		{
			CreateDeclStmt(m_ast_ctx, std::vector<clang::VarDecl*>({ m_kernel_config[BLOCK_DIM] }))
			, CreateDeclStmt(m_ast_ctx, std::vector<clang::VarDecl*>({ m_kernel_config[GRID_DIM] }))
		};

		std::vector<clang::Expr*> kernel_args;
		auto & nest_idx_vec = m_analysis_driver.GetNewIdxVec();

		BuildKernelArgList(kernel_args);
		auto kernel_call_expr = CreateCUDAKernelCallExpr(
			m_ast_ctx
			, CreateDeclRefExpr(m_ast_ctx, dummy_kernel_decl, dummy_kernel_decl->getType())
			,
			{
				CreateDeclRefExpr(m_ast_ctx, m_kernel_config[GRID_DIM], m_kernel_config[GRID_DIM]->getType())
				, CreateDeclRefExpr(m_ast_ctx, m_kernel_config[BLOCK_DIM], m_kernel_config[BLOCK_DIM]->getType())
			}
		, kernel_args);

		if (transform_type == Transformation::TRANSFORM_INNER_PAR)
		{
			stmts.push_back(
				CreateDeclStmt(
					m_ast_ctx
					, std::vector<clang::VarDecl*>({
						m_analysis_driver.GetNewIdxVec().front() })));

			stmts.push_back(
				BuildSnglLoopUsingOutermostLoop(
					m_ast_ctx
					, m_analysis_driver.GetTransformedNest().GetOutermostLoopHdr().GetStmt()
					, { kernel_call_expr, m_cuda_fns[CUDA_DEV_SYNC] }));
		}
		else
		{
			stmts.push_back(kernel_call_expr);
			stmts.push_back(m_cuda_fns[CUDA_DEV_SYNC]);
		}

		/// Wrap all of these in a compound statement to avoid any re-definition issues
		m_produced_stmts.push_back(
			CreateCmpndStmt(m_ast_ctx, stmts));
	}

	void InvocationStmtsGen::GenMemFreeStmtsHelper(
		clang::FunctionDecl* free_fn_decl
		, clang::FunctionDecl* cudafree_fn_decl
		, VarDeclArrInfoMap& var_decl_arr_info_map)
	{
		for (auto & var_decl_info_pair : var_decl_arr_info_map)
		{
			auto device_var = &var_decl_info_pair.second.DeviceVar();
			auto tmp_host_var = var_decl_info_pair.second.TmpHostVar();

			m_produced_stmts.push_back(
				CreateCallExpr(
					m_ast_ctx
					, CreateDeclRefExpr(m_ast_ctx, cudafree_fn_decl, cudafree_fn_decl->getType())
					, { CreateDeclRefExpr(m_ast_ctx, device_var, device_var->getType()) }
			, cudafree_fn_decl->getType()));

			if (tmp_host_var != nullptr)
				m_produced_stmts.push_back(
					CreateCallExpr(
						m_ast_ctx
						, CreateDeclRefExpr(m_ast_ctx, free_fn_decl, free_fn_decl->getType())
						, { CreateDeclRefExpr(m_ast_ctx, tmp_host_var, tmp_host_var->getType()) }
			, free_fn_decl->getType()));
		}
	}

	void InvocationStmtsGen::GenMemFreeStmts()
	{
		auto free_fn_decl = NoLoadLookup<clang::FunctionDecl>(
			m_ast_ctx
			, *m_ast_ctx.getTranslationUnitDecl()
			, "free");

		auto cudafree_fn_decl = NoLoadLookup<clang::FunctionDecl>(
			m_ast_ctx
			, *m_ast_ctx.getTranslationUnitDecl()
			, "cudaFree");

		assert(
			free_fn_decl != nullptr
			&& cudafree_fn_decl != nullptr
			&& "both free and cudaFree functions must be in scope");

		GenMemFreeStmtsHelper(free_fn_decl, cudafree_fn_decl, m_host_array_info_map);
	}

	void InvocationStmtsGen::CompleteGeneration()
	{
		/// Enclose all generated code into a compound stmt to avoid any name clashes
		m_generated_block =
			CreateCmpndStmt(
				m_ast_ctx, m_produced_stmts);
	}

	void InvocationStmtsGen::RunGenerator()
	{
		GenMemAllocStmts();
		GenHostToDevDataMemcpy();
		GenBlockGridDimVarDecls();
		GenKernelCallSegment();
		GenDevToHostDataMempcy();
		GenMemFreeStmts();
		CompleteGeneration();
	}

	clang::CompoundStmt* InvocationStmtsGen::GetGenBlock()
	{
		return m_generated_block;
	}

	VarDeclArrInfoMap& InvocationStmtsGen::GetHostVarArrDeclInfoMap()
	{
		return m_host_array_info_map;
	}

} /// namespace gap