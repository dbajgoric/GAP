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

#ifndef GAP_FRONTEND_NEST_ANALYSIS_DRIVER_H
#define GAP_FRONTEND_NEST_ANALYSIS_DRIVER_H

#include <clang/AST/ASTContext.h>
#include "nest_transformer.h"
#include "nest_rewriter.h"

namespace gap
{

	/// Forward declaration
	class PerfectLoopNest;

	/// Drives the task of nest data dependence analysis and loop transformation.
	/// For the given perfect loop nest, it builds a dependence model that is used
	/// to perform dependence analysis that is in turn used for loop transformation.
	/// Selected transformation is then used to map the original loop nest to the
	/// transformed loop nest. All of above happens during the driver's construction
	/// and exception is thrown in case of failure
	class NestAnalysisDriver
	{
	public:

		explicit NestAnalysisDriver(
			clang::ASTContext& ast_ctx
			, PerfectLoopNest& original_nest);

		const Transformation& GetTransformation() const;
		PerfectLoopNest& GetTransformedNest();
		const PerfectLoopNest& GetTransformedNest() const;
		const std::vector<clang::VarDecl*>& GetNewIdxVec() const;

	private:

		NestTransformer m_nest_transformer;
		NestRewriter m_nest_rewriter;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_NEST_ANALYSIS_DRIVER_H
