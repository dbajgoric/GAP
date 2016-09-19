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

#ifndef GAP_FRONTEND_DEP_MDL_ASSIGN_STMT_H
#define GAP_FRONTEND_DEP_MDL_ASSIGN_STMT_H

#include <memory>
#include <vector>
#include <utility>
#include "mat_vec_pair.h"
#include <clang/AST/Decl.h>

namespace gap
{

	/// Forward declaration
	class AssignStmt;
	class PerfectLoopNest;

	typedef std::pair<const clang::VarDecl*, IntMatVecPair> VarDeclMatVecPair;
	typedef std::unique_ptr<VarDeclMatVecPair> MatVecPairUniquePtr;

	/// This class represents an assignment statement in a way suitable for
	/// dependence analysis. While second compilation pass used linear exprs
	/// to model array subscripts, this class uses matrices for this purpose
	class DepMdlAssignStmt
	{
	public:

		DepMdlAssignStmt(const PerfectLoopNest& loop_nest, AssignStmt& assign_stmt);

		/// Accessors
		AssignStmt& GetAssignStmt();
		const AssignStmt& GetAssignStmt() const;
		const VarDeclMatVecPair* GetLhs() const;
		const std::vector<VarDeclMatVecPair>& GetRhs() const;

	private:

		AssignStmt& m_assign_stmt;
		MatVecPairUniquePtr m_lhs;
		std::vector<VarDeclMatVecPair> m_rhs;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_DEP_MDL_ASSIGN_STMT_H
