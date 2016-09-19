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

#ifndef GAP_FRONTEND_NEST_REWRITER_H
#define GAP_FRONTEND_NEST_REWRITER_H

#include <clang/AST/ASTContext.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Decl.h>
#include <vector>

namespace gap
{

	/// Forward declaration
	class NestTransformer;
	class PerfectLoopNest;

	/// Applies the loop transformation to the original loop nest. This includes
	/// changing the nest bounds, updating all the array subscripts that reference
	/// any of the loop index variables as well as any other occurences of the loop
	/// index variables (they may appear on RHS of assignment stmts)
	///
	/// IMPROVEMENTS:
	/// 1) Calling ceilf and floorf when calculating nest bound is rather inefficient. It
	/// is surely possible to do this rounding without these functions
	class NestRewriter
	{
	public:

		/// Rewrites the loop nest as part of the object construction
		NestRewriter(
			clang::ASTContext& ast_ctx
			, PerfectLoopNest& original_nest
			, const NestTransformer& transformer);

		PerfectLoopNest& GetTransformedNest();
		std::vector<clang::VarDecl*>& GetNewIdxVec();
		const PerfectLoopNest& GetTransformedNest() const;
		const std::vector<clang::VarDecl*>& GetNewIdxVec() const;

	private:

		/// Generates new nest index vector for the transformed loop nest. This is done to make
		/// mainly to keep the original loop variables intact as they may be reused somewhere
		/// after the loop nest. Otherwise, compiler would have to scan the rest of the enclosing
		/// function for places where loop variables are used again. The new index variables have
		/// form __i0. The type of the variables is also forced to long long.
		///
		/// NOTE: no checks are performed to make sure that variables with these names don't exist
		/// already. This won't be a problem as code generator will create a dedicated block for
		/// the nest and put definition of the index variables within this block.
		/// FIXME: there can be a problem with unsigned loop variables in the transformed
		/// nest as there are no guarantees that transformed nest loop variables will not
		/// have negative values in this case. For now I'll simply force the variable type
		/// to long long in the transformed nest, but this could lead to overflow issues
		/// with transformed nest's loop variables
		void GenerateNewNestIdxVector(
			clang::ASTContext& ast_ctx
			, std::size_t nest_depth
			, const std::string& idx_var_base_name = "i");

		PerfectLoopNest& m_transformed_nest;
		std::vector<clang::VarDecl*> m_new_nest_idx_vec;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_NEST_REWRITER_H
