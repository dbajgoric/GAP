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

#include "dep_mdl_assign_stmt.h"
#include "perfect_loop_nest.h"
#include <gap_util/types.h>
#include <algorithm>
#include <limits>
#include <cassert>


namespace gap
{
	namespace
	{

		std::unique_ptr<std::pair<const clang::VarDecl*, IntMatVecPair>> ArrSubToMatVecPair(
			const PerfectLoopNest& loop_nest
			, const ArraySubscript& arr_sub)
		{
			auto var_decl_mat_vec_ptr = std::make_unique<std::pair<const clang::VarDecl*, IntMatVecPair>>(
				std::make_pair(
					arr_sub.GetArrDecl()
					, IntMatVecPair(
						util::IntMatrixType(loop_nest.GetNestDepth(), arr_sub.GetDimensionality())
						, util::IntRowVecType(arr_sub.GetDimensionality()))));

			auto & nest_idx_vector = loop_nest.GetNestIdxVec();
			for (std::size_t row = 0; row < nest_idx_vector.size(); ++row)
			{
				for (std::size_t col = 0; col < arr_sub.GetDimensionality(); ++col)
				{
					auto & arr_idx = arr_sub.GetLinearExpr(col);
					auto int_bool_pair = arr_idx.GetVarCoeff(nest_idx_vector[row]);
					if (int_bool_pair.second)
						var_decl_mat_vec_ptr->second.m_mat(row, col) = int_bool_pair.first.getExtValue();

					var_decl_mat_vec_ptr->second.m_vec(col) = arr_idx.GetConstant().getExtValue();
				}
			}
			return std::move(var_decl_mat_vec_ptr);
		}

	} /// Anonymous namespace


	DepMdlAssignStmt::DepMdlAssignStmt(const PerfectLoopNest& loop_nest, AssignStmt& assign_stmt)
		: m_assign_stmt(assign_stmt)
	{
		if (auto lhs = m_assign_stmt.GetLhs())
			m_lhs = ArrSubToMatVecPair(loop_nest, *lhs);
		for (auto & arr_sub : assign_stmt.GetRhs())
			m_rhs.push_back(std::move(*ArrSubToMatVecPair(loop_nest, arr_sub).release()));
	}

	AssignStmt& DepMdlAssignStmt::GetAssignStmt()
	{
		return m_assign_stmt;
	}

	const AssignStmt& DepMdlAssignStmt::GetAssignStmt() const
	{
		return m_assign_stmt;
	}

	const VarDeclMatVecPair* DepMdlAssignStmt::GetLhs() const
	{
		return m_lhs.get();
	}

	const std::vector<VarDeclMatVecPair>& DepMdlAssignStmt::GetRhs() const
	{
		return m_rhs;
	}

} /// namespace gap