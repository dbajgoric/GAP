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

#include "unimodular_transform_test.h"
#include "test_utils.h"
#include <gap_transform/unimodular_transform.h>
#include <gap_util/row_vector.h>
#include <cassert>

using namespace gap::util;
using namespace gap::tran;

namespace
{

	void TestFindUnimodularTransformMatrixInternal(const std::vector<RowVector<long long>>& distance_vectors,
		const unsigned int m,
		bool search_inner_transform = true,
		bool dependence_exists = true, /** false if there are no loop carried dependencies or no dependencies at all */
		bool should_throw = false,
		bool transform_exists = true)
	{
		try
		{
			unsigned int rank_d(0);
			MatrixExt<long long> D(0, 0);

			if (!distance_vectors.empty())
			{
				D.set_size(distance_vectors.size(), m);
				for (auto i = 0U; i < distance_vectors.size(); ++i)
					D.row(i) = distance_vectors[i];

				rank_d = D.ComputeRank();
			}

			MatrixExt<long long> U(m, m);
			if (search_inner_transform)
			{
				FindInnerLoopTransformMatrix(distance_vectors, U);
				MatrixExt<long long> DU(std::move(D * U));
				// Check if every row of matrix DU has level 0 (first element of each row is nonzero)
				if (dependence_exists)
					for (auto i = 0U; i < DU.n_rows; ++i)
						assert(DU.GetRowLevel(i) == 0);
				else
					assert(CompareMatrices(D, D * U));
			}
			else
			{
				unsigned int n = FindOuterLoopTransformMatrix(distance_vectors, U);
				assert(n == m - rank_d);
				assert((transform_exists && n > 0) || (!transform_exists && n == 0));
				MatrixExt<long long> DU(n > 0 ? std::move(D * U) : D);
				// Verify that each row has level = n + 1 where n = m - rank_d (first n columns of DU are zero columns)
				if (dependence_exists && n > 0)
					for (auto i = 0U; i < DU.n_rows; ++i)
						assert(DU.GetRowLevel(i) == n);
				else if (!dependence_exists)
					assert(CompareMatrices(D, D * U));
			}

			assert(!should_throw);
		}
		catch (...)
		{
			assert(should_throw);
		}
	}

	void TestCaclTranformedNestLimitsInternal(
		const RowVector<long long>& p0,
		const MatrixExt<long long>& P,
		const RowVector<long long>& q0,
		const MatrixExt<long long>& Q,
		const MatrixExt<long long>& U)
	{
		// Firstly, run CalculateTransformedNestLimits() on limits of L with U as identity matrix, to get reference lower and upper bounds
		std::vector<Bound> b_L;
		std::vector<Bound> B_L;
		CalculateTransformedNestLimits(MatrixExt<long long>(P.n_rows, P.n_rows, arma::fill::eye),
			p0,
			P,
			q0,
			Q,
			b_L,
			B_L);

		// Now, run it once again to obtain limits of transformed loop nest LU (with transformation matrix U)
		std::vector<Bound> b_LU;
		std::vector<Bound> B_LU;
		CalculateTransformedNestLimits(
			U,
			p0,
			P,
			q0,
			Q,
			b_LU,
			B_LU);

		// Run EnumerateIntegerSolutions() on both sets of lower/upper bounds to get all possible integer vectors in both nests
		//std::vector<c2cuda::RowVector<long long>> loop_vectors_L;
		//std::vector<c2cuda::RowVector<long long>> loop_vectors_LU;
		std::vector<std::vector<RowVector<long long>>> loop_vectors_L;
		std::vector<std::vector<RowVector<long long>>> loop_vectors_LU;
		assert(EnumerateIntegerSolutions(b_L, B_L, loop_vectors_L));
		assert(EnumerateIntegerSolutions(b_LU, B_LU, loop_vectors_LU));

		// Verify that both loop nests L and LU have some number of iterations (iterations of L and LU are mapped 1-to-1)
		size_t num_iter_L = 0;
		size_t num_iter_LU = 0;
		for (auto & i : loop_vectors_L)
			num_iter_L += i.size();
		for (auto & i : loop_vectors_LU)
			num_iter_LU += i.size();

		assert(num_iter_L == num_iter_LU);
	}

}

void TestFindInnerLoopTransformMatrix()
{
	// Test case 1 (dependence at level 2)
	TestFindUnimodularTransformMatrixInternal(
		{ { 0, 5 }, { 0, 2 } },
		2);

	// Test case 2 (dependence at level 1)
	TestFindUnimodularTransformMatrixInternal(
		{ { 4, 3 }, { 2, 0 }, { 3, -6 }, { 1, -12 } },
		2);

	// Test case 3 (dependence at levels 1 and 2)
	TestFindUnimodularTransformMatrixInternal(
		{ { 0, 4 }, { 1, 0 } },
		2);

	// Test case 4 (dependence at levels 1 and 2)
	TestFindUnimodularTransformMatrixInternal(
		{ { 0, 3 }, { 4, 2 }, { 2, 0 } },
		2);

	// Test case 5 (dependence at levels 1 and 2)
	TestFindUnimodularTransformMatrixInternal(
		{ { 0, 3 }, { 1, 1 } },
		2);

	// Test case 6 (dependence at levels 1 and 2)
	TestFindUnimodularTransformMatrixInternal(
		{ { 0, 1 }, { 2, -3 } },
		2);

	// Test case 7 (no loop carried dependences)
	TestFindUnimodularTransformMatrixInternal(
		{ { 0, 0 }, { 0, 0 }, { 0, 0 } },
		2,
		true,
		false);

	// Test case 8 (distance vectors set empty)
	TestFindUnimodularTransformMatrixInternal(
		{ },
		3,
		true,
		false,
		true);

	// Test case 9 (dependence at levels 1, 2 and 3)
	TestFindUnimodularTransformMatrixInternal(
		{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } },
		3);

	// Test case 10 (dependence at levels 1, 2 and 3)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2, 0, 0 }, { 0, 3, 0 }, { 0, 0, 4 } },
		3);

	// Test case 11 (dependence at level 1)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2, 4, 6 } },
		3);

	// Test case 12 (dependence at levels 1 and 2)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2, 4, 6 }, { 0, 1, -1 } },
		3);

	// Test case 13 (dependence at levels 1, 2 and 3)
	TestFindUnimodularTransformMatrixInternal(
		{ { 1, -2, -3, -1 }, { 0, 1, -2, -3 }, { 0, 0, 1, -2 } },
		4);

	// Test case 14 (dependence at levels 1, 2 and 4)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2, -1, -2, 1 }, { 4, 0, 1, 0 }, { 0, 3, 1, -2 }, { 0, 1, -2, 3 }, { 0, 0, 0, 4 }, { 0, 0, 0, 5 } },
		4);

	// Test case 15 (dependence at levels 1, 2 and 4)
	TestFindUnimodularTransformMatrixInternal(
		{ { 0, 0, 0, 2 }, { 0, 3, 1, -2 }, { 0, 4, -6, 0 }, { 1, -5, 3, 1 }, { 2, 1, 0, 0 }, { 3, 0, -2, 1 } },
		4);

	// Test case 16 (dependence at level 1, single loop nest)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2 }, { 5 } },
		1,
		true,
		true,
		true);
}

void TestFindOuterLoopTransformMatirx()
{
	// Test case 1 (rank_d = 1, m = 2, dependence at level 1)
	TestFindUnimodularTransformMatrixInternal(
		{ { 3, 5 } },
		2,
		false);

	// Test case 2 (rank_d = 2, m = 2, dependence at levels 1 and 2)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2, 4 }, { 1, -2 }, { 0, 1 } },
		2,
		false,
		true,
		false,
		false);

	// Test case 3 (rank_d = 1, m = 2, dependence at level 1)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2, -4 }, { 3, -6 }, { 4, -8 } },
		2,
		false);

	// Test case 4 (rank_d = 3, m = 3, dependence at levels 1 and 2)
	TestFindUnimodularTransformMatrixInternal(
		{ { 1, 2, 3 }, { 0, 1, -2 }, { 4, -2, 0 } },
		3,
		false,
		true,
		false,
		false);

	// Test case 5 (rank_d = 2, m = 3, dependence at level 1)
	TestFindUnimodularTransformMatrixInternal(
		{ { 4, -2, 1 }, { 4, 1, -1 }, { 8, 5, -4 } },
		3,
		false);

	// Test case 6 (rank_d = 2, m = 3, dependence at level 1 and 2)
	TestFindUnimodularTransformMatrixInternal(
		{ { 6, 4, 2 }, { 0, 1, -1 }, { 1, 0, 1 } },
		3,
		false);

	// Test case 7 (rank_d = 2, m = 3, dependence at level 1 and 2)
	TestFindUnimodularTransformMatrixInternal(
		{ { 6, 4, 2 }, { 0, 1, -1 }, { 1, 0, 1 } },
		3,
		false);

	// Test case 8 (rank_d = 0, m = 2, no loop carried dependences)
	TestFindUnimodularTransformMatrixInternal(
	{ { 0, 0 }, { 0, 0 }, { 0, 0 } },
		2,
		false,
		false);

	// Test case 9 (distance vectors set empty)
	TestFindUnimodularTransformMatrixInternal(
		{},
		3,
		false,
		false,
		true);

	// Test case 10 (dependence at level 1, single loop nest)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2 },{ 5 } },
		1,
		false,
		true,
		true);

	// Test case 11 (rank_d = 3, m = 3, dependence at levels 1, 2 and 3)
	TestFindUnimodularTransformMatrixInternal(
		{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } },
		3,
		false,
		true,
		false,
		false);

	// Test case 12 (rank_d = 3, m = 3, dependence at levels 1, 2 and 3)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2, 0, 0 }, { 0, 3, 0 }, { 0, 0, 4 } },
		3,
		false,
		true,
		false,
		false);

	// Test case 13 (rank_d = 1, m = 3, dependence at level 1)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2, 4, 6 } },
		3,
		false);

	// Test case 14 (rank_d = 2, m = 3, dependence at levels 1 and 2)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2, 4, 6 }, { 0, 1, -1 } },
		3,
		false);

	// Test case 15 (rank_d = 3, m = 4, dependence at levels 1, 2 and 3)
	TestFindUnimodularTransformMatrixInternal(
		{ { 1, -2, -3, -1 }, { 0, 1, -2, -3 }, { 0, 0, 1, -2 } },
		4,
		false);

	// Test case 16 (rank_d = 4, m = 4, dependence at levels 1, 2 and 4)
	TestFindUnimodularTransformMatrixInternal(
		{ { 2, -1, -2, 1 }, { 4, 0, 1, 0 }, { 0, 3, 1, -2 }, { 0, 1, -2, 3 }, { 0, 0, 0, 4 }, { 0, 0, 0, 5 } },
		4,
		false,
		true,
		false,
		false);

	// Test case 17 (rank_d = , m = 4, dependence at levels 1, 2 and 4)
	TestFindUnimodularTransformMatrixInternal(
		{ { 0, 3, 1, 0 }, { 0, 6, 2, 0 }, { 1, -5, 3, 0 }, { 2, -10, 6, 0 } },
		4,
		false);
}

void TestCalcTransformedNestLimits()
{
	// Test case 1
	/*TestCaclTranformedNestLimitsInternal(
		{ 3, -50, 21 },
		{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1} },
		{ 70, 45, 50 },
		{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1} },
		{ { 3, 9, 1 }, { -1, -2, 0 }, { 2, 6, 1 } });*/

	// Test case 2
	TestCaclTranformedNestLimitsInternal(
		{ 0, 0 },
		{ { 1, -1 }, { 0, 1 } },
		{ 10, 10 },
		{ { 1, -1 }, { 0, 1 } },
		{ { 2, 3 }, { 3, 4 } });

	// Test case 3
	TestCaclTranformedNestLimitsInternal(
		{ 0, 0 },
		{ { 1, 0 }, { 0, 1 } },
		{ 20, 0 },
		{ { 1, -2 }, { 0, 1 } },
		{ { -1, 1 }, { 1, -2 } });

	// Test case 4
	TestCaclTranformedNestLimitsInternal(
		{ 1, 1, 1 },
		{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } },
		{ 100, 100, 20 },
		{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } },
		{ { 1, 0, 3 }, { 0, 1, 0 }, { 0, 0, 1 } });

	// Test case 5
	TestCaclTranformedNestLimitsInternal(
		{ 0, 0, 0 },
		{ { 1, -1, 0 }, { 0, 1, -1 }, { 0, 0, 1 } },
		{ 20, 0, 0 },
		{ { 1, -1, 0 }, { 0, 1, -1 }, { 0, 0, 1 } },
		{ { 2, 1, 1 }, { 1, 1, 0 }, { 2, 1, 2 } });

	// Test case 6
	/*TestCaclTranformedNestLimitsInternal(
		{ -10, 3, 10, -5},
		{ { 1, 0, 0, 0 }, { 0, 1, 0, 0}, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } },
		{ 0, 10, 25, 10 },
		{ { 1, 0, 0, 0 },{ 0, 1, 0, 0 },{ 0, 0, 1, 0 },{ 0, 0, 0, 1 } },
		{ { 14, 0, -8, 3 },{ 1, 0, 0, 0 }, { -3, 0, 3, -1 }, { 0, 1, 0, 0 } });*/
}