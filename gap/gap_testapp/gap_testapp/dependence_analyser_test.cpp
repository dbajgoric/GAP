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

#include "dependence_analyser_test.h"
#include <gap_util/dependence_analyser.h>
#include <gap_util/fourier_elimination.h>
#include <gap_transform/unimodular_transform.h>
#include <gap_util/row_vector.h>
#include <cassert>
#include <vector>
#include <unordered_set>
#include "test_utils.h"

using namespace gap::util;
using namespace gap::dep;
using namespace gap::tran;

static void TestGeneralDependenceTestInternal(const MatrixExt<long long>& A,
	const RowVector<long long>& a0,
	const MatrixExt<long long>& B,
	const RowVector<long long>& b0,
	const MatrixExt<long long>& P,
	const RowVector<long long>& p0,
	const MatrixExt<long long>& Q,
	const RowVector<long long>& q0,
	bool are_distinct = true,
	bool dependence_exists = true)
{
	std::vector<DependenceInfo> T_on_S, S_on_T;
	assert(GeneralLinearDependenceTest(A, a0, B, b0, P, p0, Q, q0, T_on_S, S_on_T, are_distinct) == dependence_exists);
	dependence_exists ? assert(!T_on_S.empty() || !S_on_T.empty()) : assert(T_on_S.empty() && S_on_T.empty());

	for (auto * dep_vector : { &T_on_S, &S_on_T })
	{
		for (auto & dep_info : *dep_vector)
		{
			RowVector<long long>& lower = dep_vector == &T_on_S ? dep_info.i : dep_info.j;
			RowVector<long long>& greater = dep_vector == &T_on_S ? dep_info.j : dep_info.i;

			RowVector<long long>::ComparisonResult comp_res(RowVector<long long>::CompareLexicographically(lower, greater));
			assert(comp_res == RowVector<long long>::RightGreater || comp_res == RowVector<long long>::Equal);
			assert(CompareMatrices(greater - lower, dep_info.d));
			assert(dep_info.d.GetLevel() == dep_info.lev);
			assert(CompareMatrices(dep_info.sig, ::signum(dep_info.d)));
			assert(A.n_rows == dep_info.i.n_cols && dep_info.i.n_cols == dep_info.j.n_cols);

			/** Most importantly, check if variables X(iA + a0) and X(jB + b0) really represent the same memory location for i and j. */
			assert(CompareMatrices(dep_info.i * A + a0, dep_info.j * B + b0));
		}
	}
}

static void TestUniformLinearDependenceInternal(const MatrixExt<long long>& A,
	const RowVector<long long>& a0,
	const RowVector<long long>& b0,
	const MatrixExt<long long>& P,
	const RowVector<long long>& p0,
	const RowVector<long long>& q0,
	bool are_distinct = true,
	bool dependence_exists = true)
{
	std::vector<DependenceInfo> T_on_S, S_on_T;
	assert(UniformLinearDependenceTest(A, a0, b0, P, p0, q0, T_on_S, S_on_T, are_distinct) == dependence_exists);
	dependence_exists ? assert(!T_on_S.empty() || !S_on_T.empty()) : assert(T_on_S.empty() && S_on_T.empty());

	/**
	* Uniform linear dependence test doesn't give us vectors i and j but only uniform dependence distance vector that can correspond to
	* many (i,j) pairs. Generate all possible loop vectors I (this might be very inefficient for big loop nests) and check if distance
	* d between vectors instances i and j (either d = j - i or d = i - j) leads to dependence between statements S and T.
	*/
	MatrixExt<long long> Z(P.n_rows, 2 * P.n_cols);
	RowVector<long long> v0(2 * p0.n_cols);
	Z(arma::span::all, arma::span(0, P.n_cols - 1)) = P;
	Z(arma::span::all, arma::span(P.n_cols, 2 * P.n_cols - 1)) = std::move(-1LL * P);
	v0(0, arma::span(0, P.n_cols - 1)) = q0;
	v0(0, arma::span(P.n_cols, 2 * P.n_cols - 1)) = std::move(-1LL * p0);

	std::vector<Bound> lower_bound, upper_bound;
	assert(PerformFourierElimination(Z, v0, lower_bound, upper_bound));
	std::vector<std::vector<RowVector<long long>>> all_iteration_sets;
	assert(EnumerateIntegerSolutions(lower_bound, upper_bound, all_iteration_sets));

	for (auto * dep_vector : { &T_on_S, &S_on_T })
	{
		for (auto & dep_info : *dep_vector)
		{
			assert(dep_info.i.n_cols == 0 && dep_info.j.n_cols == 0);

			unsigned short count(0);
			for (auto & all_iterations : all_iteration_sets)
			{
				for (auto & lower : all_iterations)
				{
					RowVector<long long> greater(std::move(lower + dep_info.d));
					assert(dep_info.d.GetLevel() == dep_info.lev);
					assert(CompareMatrices(dep_info.sig, ::signum(dep_info.d)));

					/** Most importantly, check if variables X(iA + a0) and X(jA + b0) really represent the same memory location for i and j. */
					if (dep_vector == &T_on_S)
						assert(CompareMatrices(lower * A + a0, greater * A + b0));
					else
						assert(CompareMatrices(greater * A + a0, lower * A + b0));

					if (++count >= 1000) /** Check up to 1000 loop vectors for single uniform distance vector. */
						break;
				}
			}
		}
	}
}

void TestGeneralLinearDependenceTest()
{
	// Test case 1 (dependence exists)
	TestGeneralDependenceTestInternal(
	{ { 2, 1 } },
	{ -2, 3 },
	{ { -1, 2 } },
	{ 300, 9 },
	{ { 1 } },
	{ 10 },
	{ { 1 } },
	{ 200 });

	// Test case 2 (dependence exists)
	TestGeneralDependenceTestInternal(
	{ { 2, 0, 0 },{ 0, 5, 1 } },
	{ 3, -1, 0 },
	{ { 1, 2, 0 },{ 0, 0, 3 } },
	{ -1, -6, 2 },
	{ { 1, 0 },{ 0, 1 } },
	{ 10, 7 },
	{ { 1, 0 },{ 0, 1 } },
	{ 200, 167 });

	// Test case 3 (dependence exists)
	TestGeneralDependenceTestInternal(
	{ { 2, 1 } },
	{ -2, 1 },
	{ { 2, 1 } },
	{ 2, 3 },
	{ { 1 } },
	{ 10 },
	{ { 1 } },
	{ 100 },
		false);

	// Test case 4 (no dependence)
	TestGeneralDependenceTestInternal(
	{ { 2, 1 } },
	{ -2, 1 },
	{ { 2, 1 } },
	{ -1, -3 },
	{ { 1 } },
	{ 10 },
	{ { 1 } },
	{ 100 },
		false,
		false);

	// Test case 5 (no dependence)
	TestGeneralDependenceTestInternal(
	{ { 1, 0, 1, 1 },{ 0, 1, 1, 1 } },
	{ 3, -1, 1, -5 },
	{ { 1, 1, 0, 1 },{ 1, 1, 1, 1 } },
	{ 1, 1, 2, -7 },
	{ { 1, 0 },{ 0, 1 } },
	{ 1, 1 },
	{ { 1, 0 },{ 0, 1 } },
	{ 200, 200 },
		true,
		false);

	// Test case 6 (dependence exists)
	TestGeneralDependenceTestInternal(
	{ { 2, 0, 0 },{ 0, 5, 1 } },
	{ 3, -1, 0 },
	{ { 1, 2, 0 },{ 0, 0, 3 } },
	{ -1, -6, 2 },
	{ { 1, 0 },{ 0, 1 } },
	{ 0, 0 },
	{ { 1, 0 },{ 0, 1 } },
	{ 200, 300 });

	// Test case 7 (dependence exists)
	TestGeneralDependenceTestInternal(
	{ { 2, 0, 0 },{ 0, 5, 1 } },
	{ 3, -1, 0 },
	{ { 1, 2, 0 },{ 0, 0, 3 } },
	{ -1, -6, 2 },
	{ { 1, 0 },{ 0, 1 } },
	{ -100, -10 },
	{ { 1, 0 },{ 0, 1 } },
	{ 10, 250 });

	// Test case 8 (no dependence)
	TestGeneralDependenceTestInternal(
	{ { 2, 0, 0 },{ 0, 5, 1 } },
	{ 3, -1, 0 },
	{ { 1, 2, 0 },{ 0, 0, 3 } },
	{ -1, -6, 2 },
	{ { 1, -1 },{ 0, 1 } },
	{ 0, 0 },
	{ { 1, -1 },{ 0, 1 } },
	{ 200, 150 },
		true,
		false);

	// Test case 9 (dependence exists)
	TestGeneralDependenceTestInternal(
	{ { 2, 0, 0 },{ 0, 5, 1 } },
	{ 3, -1, 0 },
	{ { 1, 2, 0 },{ 0, 0, 3 } },
	{ -1, -6, 2 },
	{ { 1, 0 },{ 0, 1 } },
	{ 0, 0 },
	{ { 1, -1 },{ 0, 1 } },
	{ 200, 150 });

	// Test case 10 (scalar variables) THROWS EXCEPTION
	/*TestGeneralDependenceTestInternal(
	{ { 0 } },
	{ 0 },
	{ { 0 } },
	{ 0 },
	{ { 1 } },
	{ 0 },
	{ { 1 } },
	{ 0 });*/

	/**
	* Add tests that cover the case where S = T and/or S < T when dependence happen for i = j. Also add test cases for loop nests
	* with 3 and more loops with dependence on various levels.
	*/
}

void TestUniformLinearDependenceTest()
{
	// Test case 1 (dependence exists)
	TestUniformLinearDependenceInternal(
	{ { 2, 0 },{ 0, 5 } },
	{ 3, -1 },
	{ -1, -6 },
	{ { 1, 0 },{ 0, 1 } },
	{ 10, 7 },
	{ 200, 167 });

	// Test case 2 (dependence exists)
	TestUniformLinearDependenceInternal(
	{ { 2, 3 },{ 3, 4 } },
	{ -2, 1 },
	{ 1, 3 },
	{ { 1, 0 },{ 0, 1 } },
	{ 1, 71 },
	{ 1000, 300 });

	// Test case 3 (dependence exists)
	TestUniformLinearDependenceInternal(
	{ { 2 },{ 3 } },
	{ 12 },
	{ -5 },
	{ { 1, -1 },{ 0, 1 } },
	{ 0, 0 },
	{ 100, 50 });

	// Test case 4 (dependence exists)
	TestUniformLinearDependenceInternal(
	{ { 1 },{ 0 } },
	{ 0 },
	{ 0 },
	{ { 1, 0 },{ 0, 1 } },
	{ 0, 0 },
	{ 100, 200 },
		false);
}

void ThesisInitialProgram()
{
	/************************ Initial program. ***************************/
	const int m = 3;
	const long long size_x = 1024, size_y = 1024, size_z = 32;
	RowVector<long long> p0({ 0, 0, 0 });
	RowVector<long long> q0({ size_x - 1, size_y - 1, size_z - 1 });
	MatrixExt<long long> P(3, 3, arma::fill::eye);

	// Output and input_1 variable
	RowVector<long long> a0({ 0 }); // a0 = b0 for these variables
	MatrixExt<long long> A({ { 1 },{ size_y },{ size_x * size_y } }); // Also B
	std::vector<DependenceInfo> T_on_S, S_on_T; // S and T are the same statement here
	std::vector<RowVector<long long>> distance_vecs;
	bool dep_exists = UniformLinearDependenceTest(
		A,
		a0,
		a0,
		P,
		p0,
		q0,
		T_on_S,
		S_on_T,
		false/*Not distinct*/); // No dependence (not carried, same statement)

	if (dep_exists)
	{
		for (auto & dep_info : T_on_S)
			if (!CompareMatrices(dep_info.d, RowVector<long long>(dep_info.d.n_cols))) // Only count it if loop carried
				distance_vecs.push_back(dep_info.d);
		for (auto & dep_info : S_on_T)
			if (!CompareMatrices(dep_info.d, RowVector<long long>(dep_info.d.n_cols))) // Only count it if loop carried
				distance_vecs.push_back(dep_info.d);
	}

	// Output and input_2 variable
	RowVector<long long> b0({ size_x * size_y }); // A = B for these two as well
	T_on_S.clear();
	S_on_T.clear();
	dep_exists = UniformLinearDependenceTest(
		A,
		a0,
		b0,
		P,
		p0,
		q0,
		T_on_S,
		S_on_T,
		false); // Single uniform dependence d = (0, 0, 1), output dependent on input

	if (dep_exists)
	{
		for (auto & dep_info : T_on_S)
			if (!CompareMatrices(dep_info.d, RowVector<long long>(dep_info.d.n_cols))) // Only count it's if loop carried
				distance_vecs.push_back(dep_info.d);
		for (auto & dep_info : S_on_T)
			if (!CompareMatrices(dep_info.d, RowVector<long long>(dep_info.d.n_cols))) // Only count it's if loop carried
				distance_vecs.push_back(dep_info.d);
	}

	if (distance_vecs.empty())
	{
		std::cout << "There are no carried dependences in Initial Test program\n\n";
		return;
	}

	MatrixExt<long long> U(1, 1);
	unsigned int n = FindOuterLoopTransformMatrix(distance_vecs, U);
	if (m - n == 1)
	{
		// Choose outer loop parallelization if innermost loop carries all the dependences.
		std::cout << "Outer loop parallelization chosen. Number of outer doall loops: " << n << std::endl << std::endl;
		U.print("Transformation matrix U:");
		std::cout << "\n";

		std::vector<Bound> b, B;
		CalculateTransformedNestLimits(
			U,
			p0,
			P,
			q0,
			P,
			b,
			B);

		for (int i = 0U; i < m; ++i)
		{
			std::cout << "Loop L" << i + 1 << ":\n";
			std::cout << "Lower bound:\n";
			b[i].first.print("b" + std::to_string(i + 1) + "_mat:");
			b[i].second.print("b" + std::to_string(i + 1) + "_const:");
			std::cout << std::endl << std::endl;

			std::cout << "Upper bound:\n";
			B[i].first.print("B" + std::to_string(i + 1) + "_mat:");
			B[i].second.print("B" + std::to_string(i + 1) + "_const:");
			std::cout << std::endl << std::endl;
		}
	}
	else
	{
		// Otherwise, choose inner loop parallelization.
		FindInnerLoopTransformMatrix(distance_vecs, U);
	}
}

void ThesisMatrixMult()
{
	/************************ Matrix multiplication. ***************************/
	const int m = 3;
	const long long a_n_rows = 256, a_n_cols = 256, b_n_cols = 128;
	RowVector<long long> p0({ 0, 0, 0 });
	RowVector<long long> q0({ a_n_rows, b_n_cols, a_n_cols });
	MatrixExt<long long> P(3, 3, arma::fill::eye);

	// Output and input variable
	RowVector<long long> a0({ 0, 0 }); // a0 = b0 = 0 for these variables
	MatrixExt<long long> A({ { 1, 0 },{ 0, 1 },{ 0, 0 } }); // Also B
	std::vector<DependenceInfo> T_on_S, S_on_T; // S and T are the same statement here
	std::vector<RowVector<long long>> distance_vecs;
	bool dep_exists = UniformLinearDependenceTest(
		A,
		a0,
		a0,
		P,
		p0,
		q0,
		T_on_S,
		S_on_T,
		false/*Not distinct*/); // There is loop carried dependence 

	if (dep_exists)
	{
		for (auto & dep_info : T_on_S)
			if (!CompareMatrices(dep_info.d, RowVector<long long>(dep_info.d.n_cols))) // Only count it if loop carried
				distance_vecs.push_back(dep_info.d);
		for (auto & dep_info : S_on_T)
			if (!CompareMatrices(dep_info.d, RowVector<long long>(dep_info.d.n_cols))) // Only count it if loop carried
				distance_vecs.push_back(dep_info.d);
	}

	if (distance_vecs.empty())
	{
		std::cout << "There are no carried dependences in Matrix multiplication program\n\n";
		return;
	}

	MatrixExt<long long> U(1, 1);
	unsigned int n = FindOuterLoopTransformMatrix(distance_vecs, U);
	if (m - n == 1)
	{
		// Choose outer loop parallelization if innermost loop carries all the dependences.
		std::cout << "Outer loop parallelization chosen. Number of outer doall loops: " << n << std::endl << std::endl;
		U.print("Transformation matrix U:");
		std::cout << "\n";

		std::vector<Bound> b, B;
		CalculateTransformedNestLimits(
			U,
			p0,
			P,
			q0,
			P,
			b,
			B);

		for (int i = 0U; i < m; ++i)
		{
			std::cout << "Loop L" << i + 1 << ":\n";
			std::cout << "Lower bound:\n";
			b[i].first.print("b" + std::to_string(i + 1) + "_mat:");
			b[i].second.print("b" + std::to_string(i + 1) + "_const:");
			std::cout << std::endl << std::endl;

			std::cout << "Upper bound:\n";
			B[i].first.print("B" + std::to_string(i + 1) + "_mat:");
			B[i].second.print("B" + std::to_string(i + 1) + "_const:");
			std::cout << std::endl << std::endl;
		}
	}
	else
	{
		// Otherwise, choose inner loop parallelization.
		FindInnerLoopTransformMatrix(distance_vecs, U);
	}
}

void ThesisMRI_Q_Computation()
{
	const int m = 2;
	const long long numK = 256, numX = 1024;
	RowVector<long long> p0({ 0, 0 });
	RowVector<long long> q0({ numK - 1, numX - 1 });
	MatrixExt<long long> P(2, 2, arma::fill::eye);

	// Output and input variable
	RowVector<long long> a0({ 0 }); // a0 = b0 = 0 for these variables
	MatrixExt<long long> A({ { 0 },{ 1 } }); // Also B
	std::vector<DependenceInfo> T_on_S, S_on_T; // S and T are the same statement here
	std::vector<RowVector<long long>> distance_vecs;
	bool dep_exists = UniformLinearDependenceTest(
		A,
		a0,
		a0,
		P,
		p0,
		q0,
		T_on_S,
		S_on_T,
		false/*Not distinct*/); // There is loop carried dependence 

	if (dep_exists)
	{
		for (auto & dep_info : T_on_S)
			if (!CompareMatrices(dep_info.d, RowVector<long long>(dep_info.d.n_cols))) // Only count it if loop carried
				distance_vecs.push_back(dep_info.d);
		for (auto & dep_info : S_on_T)
			if (!CompareMatrices(dep_info.d, RowVector<long long>(dep_info.d.n_cols))) // Only count it if loop carried
				distance_vecs.push_back(dep_info.d);
	}

	if (distance_vecs.empty())
	{
		std::cout << "There are no carried dependences in Matrix multiplication program\n\n";
		return;
	}

	MatrixExt<long long> U(1, 1);
	unsigned int n = FindOuterLoopTransformMatrix(distance_vecs, U);
	if (m - n == 1)
	{
		// Choose outer loop parallelization if innermost loop carries all the dependences.
		std::cout << "Outer loop parallelization chosen. Number of outer doall loops: " << n << std::endl << std::endl;
		U.print("Transformation matrix U:");
		std::cout << "\n";

		std::vector<Bound> b, B;
		CalculateTransformedNestLimits(
			U,
			p0,
			P,
			q0,
			P,
			b,
			B);

		for (int i = 0U; i < m; ++i)
		{
			std::cout << "Loop L" << i + 1 << ":\n";
			std::cout << "Lower bound:\n";
			b[i].first.print("b" + std::to_string(i + 1) + "_mat:");
			b[i].second.print("b" + std::to_string(i + 1) + "_const:");
			std::cout << std::endl << std::endl;

			std::cout << "Upper bound:\n";
			B[i].first.print("B" + std::to_string(i + 1) + "_mat:");
			B[i].second.print("B" + std::to_string(i + 1) + "_const:");
			std::cout << std::endl << std::endl;
		}
	}
	else
	{
		// Otherwise, choose inner loop parallelization.
		FindInnerLoopTransformMatrix(distance_vecs, U);
	}
}