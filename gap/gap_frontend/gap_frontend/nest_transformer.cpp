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

#include "nest_transformer.h"
#include "dependence_mdl.h"
#include "perfect_loop_nest.h"
#include <gap_util/dependence_analyser.h>
#include <iostream>
#include <exception>
#include <algorithm>
#include <iterator>


namespace gap
{
	namespace
	{

		std::string TransformationToStr(Transformation::TransformType transform_type)
		{
			switch (transform_type)
			{
			case Transformation::TRANSFORM_NONE:
				return "NO TRANSFORMATION";
			case Transformation::TRANSFORM_INNER_PAR:
				return "INNER LOOP PARALLELIZATION";
			case Transformation::TRANSFORM_OUTER_PAR:
				return "OUTER LOOP PARALLELIZATION";
			default:
				return "UNKNOWN TRANSFORMATION";
			}
		}

		void ExtractDistancesFromDepInfoVec(
			std::vector<dep::DependenceInfo>&& dep_info_vec
			, std::vector<util::IntRowVecType>& distance_vecs)
		{
			/// There may be duplicates in distance vectors but don't bother discovering that
			/// as loop transformation won't be affected by it (there will be more computations
			/// to perform as distance matrix will be larger than it could be)
			distance_vecs.reserve(distance_vecs.size() + dep_info_vec.size());
			std::transform(
				dep_info_vec.begin()
				, dep_info_vec.end()
				, std::back_inserter(distance_vecs)
				, [](dep::DependenceInfo& dep_info) { return std::move(dep_info.d); });
		}

		void SelectDependenceAnalyser(
			const IntMatVecPair& low_bnd
			, const IntMatVecPair& upp_bnd
			, const IntMatVecPair& first_arr_sub
			, const IntMatVecPair& second_arr_sub
			, std::vector<util::IntRowVecType>& distance_vecs
			, bool subs_are_in_distinct_stmts
			, bool is_regular_or_rect_nest)
		{
			std::vector<dep::DependenceInfo> dep_info_vec;
			if (is_regular_or_rect_nest && util::Equal(first_arr_sub.m_mat, second_arr_sub.m_mat))
				dep::UniformLinearDependenceTest(
					first_arr_sub.m_mat
					, first_arr_sub.m_vec
					, second_arr_sub.m_vec
					, low_bnd.m_mat
					, low_bnd.m_vec
					, upp_bnd.m_vec
					, dep_info_vec
					, dep_info_vec
					, subs_are_in_distinct_stmts);
			else
				dep::GeneralLinearDependenceTest(
					first_arr_sub.m_mat
					, first_arr_sub.m_vec
					, second_arr_sub.m_mat
					, second_arr_sub.m_vec
					, low_bnd.m_mat
					, low_bnd.m_vec
					, upp_bnd.m_mat
					, upp_bnd.m_vec
					, dep_info_vec
					, dep_info_vec
					, subs_are_in_distinct_stmts);

			ExtractDistancesFromDepInfoVec(std::move(dep_info_vec), distance_vecs);
		}

		void RunDependenceAnalyser(
			const DependenceMdl& dep_mdl
			, std::vector<util::IntRowVecType>& distance_vecs)
		{
			/// Each LHS array sub of each assign stmt has to be checked against each LHS
			/// array sub of other assign stmts (excluding its own stmt) as well as against
			/// RHS array subs in all assign stmts (including its own stmt)
			bool is_regular_or_rect_nest(
				util::Equal(dep_mdl.GetLowerBnd().m_mat, dep_mdl.GetUpperBnd().m_mat));

			for (auto & curr_assign_stmt : dep_mdl.GetAssignStmts())
			{
				if (curr_assign_stmt.GetLhs() == nullptr)
					continue;

				const VarDeclMatVecPair& lhs_arr_sub = *curr_assign_stmt.GetLhs();
				for (auto & other_assign_stmt : dep_mdl.GetAssignStmts())
				{
					if (&curr_assign_stmt != &other_assign_stmt
						&& other_assign_stmt.GetLhs() != nullptr
						&& lhs_arr_sub.first == other_assign_stmt.GetLhs()->first)
						SelectDependenceAnalyser(
							dep_mdl.GetLowerBnd()
							, dep_mdl.GetUpperBnd()
							, lhs_arr_sub.second
							, other_assign_stmt.GetLhs()->second
							, distance_vecs
							, true
							, is_regular_or_rect_nest);

					for (auto & rhs_arr_sub : other_assign_stmt.GetRhs())
						if (lhs_arr_sub.first == rhs_arr_sub.first)
							SelectDependenceAnalyser(
								dep_mdl.GetLowerBnd()
								, dep_mdl.GetUpperBnd()
								, lhs_arr_sub.second
								, rhs_arr_sub.second
								, distance_vecs
								, &curr_assign_stmt != &other_assign_stmt
								, is_regular_or_rect_nest);
				}
			}
		}

		/// Follows the logic documented in nest_transformer.h
		Transformation TransformLoopNest(
			const DependenceMdl& dep_mdl
			, const std::vector<util::IntRowVecType> distance_vecs
			, util::IntMatrixType& transform_mat)
		{
			std::size_t nest_depth = dep_mdl.GetNest().GetNestDepth();
			if (distance_vecs.empty())
			{
				/// There are no dependences in the nest
				transform_mat = util::IntMatrixType(nest_depth, nest_depth, arma::fill::eye);
				return Transformation(Transformation::TRANSFORM_NONE, nest_depth);
			}

			Transformation transform(
				Transformation::TRANSFORM_OUTER_PAR
				, tran::FindOuterLoopTransformMatrix(distance_vecs, transform_mat));

			if (transform.GetDepFreeLoopsCnt() == 0)
				transform = Transformation(
					Transformation::TRANSFORM_INNER_PAR
					, tran::FindInnerLoopTransformMatrix(distance_vecs, transform_mat));

			return transform;
		}

		void CalculateNewBnds(
			const util::IntMatrixType& transform_mat
			, const DependenceMdl& dep_mdl
			, std::vector<util::Bound>& new_low_bnd
			, std::vector<util::Bound>& new_up_bnd)
		{
			tran::CalculateTransformedNestLimits(
				transform_mat
				, dep_mdl.GetLowerBnd().m_vec
				, dep_mdl.GetLowerBnd().m_mat
				, dep_mdl.GetUpperBnd().m_vec
				, dep_mdl.GetUpperBnd().m_mat
				, new_low_bnd
				, new_up_bnd);
		}

	} /// Anonymous namespace


	NestTransformer::NestTransformer(const DependenceMdl& dep_mdl)
		: m_transform_mat(0, 0)
	{
		RunDependenceAnalyser(dep_mdl, m_distance_vecs);
		/*for (auto & distance : m_distance_vecs)
			distance.print();*/

		m_transform = TransformLoopNest(dep_mdl, m_distance_vecs, m_transform_mat);
		if (m_transform.GetDepFreeLoopsCnt() == 0)
			throw std::runtime_error(
				"the nest must have at least two dependence free loops to be "
				"considered for parallelization");

		CalculateNewBnds(m_transform_mat, dep_mdl, m_lower_bnd, m_upper_bnd);
	}

	const util::IntMatrixType& NestTransformer::GetTransformMat() const
	{
		return m_transform_mat;
	}

	const std::vector<util::Bound>& NestTransformer::GetLowerBnd() const
	{
		return m_lower_bnd;
	}

	const std::vector<util::Bound>& NestTransformer::GetUpperBnd() const
	{
		return m_upper_bnd;
	}

	const Transformation& NestTransformer::GetTransformation() const
	{
		return m_transform;
	}

	void NestTransformer::Dump() const
	{
		/*for (auto & distance : m_distance_vecs)
			distance.print();*/

		std::cout << "Selected transformation: " << TransformationToStr(m_transform.GetTransformType()) << "\n\n";
		m_transform_mat.print("transform_mat =");
		std::cout << "\n";
		for (std::size_t i = 0; i < m_lower_bnd.size(); ++i)
		{
			m_lower_bnd[i].second.print("low_bnd_vec_loop_" + std::to_string(i) + ":");
			std::cout << "\n";
			m_lower_bnd[i].first.print("low_bnd_mat_loop_" + std::to_string(i) + ":");
			std::cout << "\n";
		}
		for (std::size_t i = 0; i < m_upper_bnd.size(); ++i)
		{
			m_upper_bnd[i].second.print("upp_bnd_vec_loop_" + std::to_string(i) + ":");
			std::cout << "\n";
			m_upper_bnd[i].first.print("upp_bnd_mat_loop_" + std::to_string(i) + ":");
			std::cout << "\n";
		}
	}

} /// namespace gap