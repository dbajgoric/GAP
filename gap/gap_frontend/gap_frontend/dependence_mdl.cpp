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

#include "dependence_mdl.h"
#include "perfect_loop_nest.h"
#include <iostream>
#include <gap_util/types.h>


namespace gap
{
	namespace
	{

		void SetBndMatVecElement(
			std::size_t row
			, std::size_t col
			, const LinearExpr& bnd_expr
			, const clang::VarDecl* idx_var
			, IntMatVecPair& bnd)
		{
			auto int_bool_pair = bnd_expr.GetVarCoeff(idx_var);
			if (int_bool_pair.second)
				bnd.m_mat(row, col) = -1 * int_bool_pair.first.getExtValue();

			bnd.m_vec(col) = bnd_expr.GetConstant().getExtValue();
		}

		void ExtractBndsFromNest(
			const PerfectLoopNest& loop_nest
			, IntMatVecPair& lower_bnd
			, IntMatVecPair& upper_bnd)
		{
			lower_bnd = upper_bnd = IntMatVecPair(
				util::IntMatrixType(loop_nest.GetNestDepth(), loop_nest.GetNestDepth(), arma::fill::eye)
				, util::IntRowVecType(loop_nest.GetNestDepth()));

			auto & nest_idx_vector = loop_nest.GetNestIdxVec();
			for (std::size_t row = 0; row < nest_idx_vector.size(); ++row)
			{
				const ForLoopHeader* loop_hdr = &loop_nest.GetOutermostLoopHdr();
				for (std::size_t col = 0; loop_hdr != nullptr; ++col, loop_hdr = loop_hdr->GetChild())
				{
					SetBndMatVecElement(row, col, loop_hdr->GetLowerBound(), nest_idx_vector[row], lower_bnd);
					SetBndMatVecElement(row, col, loop_hdr->GetUpperBound(), nest_idx_vector[row], upper_bnd);
				}
			}
		}

	} /// Anonymous namespace


	DependenceMdl::DependenceMdl(PerfectLoopNest& loop_nest)
		: m_loop_nest(loop_nest)
	{
		ExtractBndsFromNest(loop_nest, m_lower_nest_bnd, m_upper_nest_bnd);
		for (auto & assign_stmt : loop_nest.GetAssignStmts())
			m_dep_mdl_assgn_stmts.push_back(DepMdlAssignStmt(loop_nest, assign_stmt));
	}

	const PerfectLoopNest& DependenceMdl::GetNest() const
	{
		return m_loop_nest;
	}

	const IntMatVecPair& DependenceMdl::GetLowerBnd() const
	{
		return m_lower_nest_bnd;
	}

	const IntMatVecPair& DependenceMdl::GetUpperBnd() const
	{
		return m_upper_nest_bnd;
	}

	const std::vector<DepMdlAssignStmt>& DependenceMdl::GetAssignStmts() const
	{
		return m_dep_mdl_assgn_stmts;
	}

	void DependenceMdl::Dump() const
	{
		m_lower_nest_bnd.m_vec.print("low_bnd_vec =");
		std::cout << "\n";
		m_upper_nest_bnd.m_vec.print("upp_bnd_vec =");
		std::cout << "\n";
		m_lower_nest_bnd.m_mat.print("low_bnd_mat =");
		std::cout << "\n";
		m_upper_nest_bnd.m_mat.print("upp_bnd_mat =");
		std::cout << "\n";

		for (std::size_t i = 0; i < m_dep_mdl_assgn_stmts.size(); ++i)
		{
			auto lhs = m_dep_mdl_assgn_stmts[i].GetLhs();
			auto & rhs = m_dep_mdl_assgn_stmts[i].GetRhs();

			if (lhs == nullptr && rhs.empty())
				continue;

			std::cout << "Assignment stmt num. " << i + 1 << ":\n\n";
			std::cout << "Left-hand side:\n\n";
			if (lhs != nullptr)
			{
				lhs->second.m_mat.print("subscript_mat =");
				std::cout << "\n";
				lhs->second.m_vec.print("subscript_vec =");
			}
			else
			{
				std::cout << "Empty\n";
			}

			std::cout << "\nRight-hand side:\n\n";
			if (!rhs.empty())
			{
				for (std::size_t j = 0; j < rhs.size(); ++j)
				{
					rhs[j].second.m_mat.print("subscript_mat_" + std::to_string(j + 1) + " =");
					std::cout << "\n";
					rhs[j].second.m_vec.print("subscript_vec_" + std::to_string(j + 1) + " =");

					if (j < rhs.size() - 1)
						std::cout << "\n";
				}
			}
			else
			{
				std::cout << "Empty\n";
			}

			if (i < m_dep_mdl_assgn_stmts.size() - 1)
				std::cout << "\n";
		}
	}

} /// namespace gap