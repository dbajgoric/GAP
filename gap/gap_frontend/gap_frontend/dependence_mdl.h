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

#ifndef GAP_FRONTEND_DEPENDENCE_MDL_H
#define GAP_FRONTEND_DEPENDENCE_MDL_H

#include "dep_mdl_assign_stmt.h"
#include "mat_vec_pair.h"

namespace gap
{

	/// Forward declaration
	class PerfectLoopNest;

	/// Builds a dependence model from the supplied perfect loop nest. The
	/// builder produces lower and upper nest bounds as well as a vector of
	/// assignment stmts where each array subscript is represented via an
	/// instance of MatVecPair structure
	class DependenceMdl
	{
	public:

		explicit DependenceMdl(PerfectLoopNest& loop_nest);

		/// Accessors
		const PerfectLoopNest& GetNest() const;
		const IntMatVecPair& GetLowerBnd() const;
		const IntMatVecPair& GetUpperBnd() const;
		const std::vector<DepMdlAssignStmt>& GetAssignStmts() const;

		/// Dump
		void Dump() const;

	private:

		const PerfectLoopNest& m_loop_nest;
		IntMatVecPair m_lower_nest_bnd;
		IntMatVecPair m_upper_nest_bnd;
		std::vector<DepMdlAssignStmt> m_dep_mdl_assgn_stmts;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_DEPENDENCE_MDL_H
