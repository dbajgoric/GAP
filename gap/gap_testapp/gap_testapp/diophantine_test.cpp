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

#include "diophantine_test.h"
#include "test_utils.h"
#include <gap_util/diophantine.h>
#include <gap_util/row_vector.h>
#include <cassert>
#include <string>

using namespace arma;
using namespace gap::util;

static void TestSolveDiophantineEquationInternal(const ColVector<long long>& A, const long long c, bool has_solution)
{
	MatrixExt<long long> U(A.n_rows, A.n_rows);
	long long t1;
	bool result(false);

	if ((result = SolveDiophantineEquation(A, c, U, t1)) != has_solution)
	{
		result ? assert(!"Diophantine equation actually has a solution (solver didn't find it") :
			assert(!"Diophantine equation actually doesn't have a solution (solver somehow found it?!?)");
	}
	else if (has_solution)
	{
		// Test solution with certain number of random vectors t=[t1,t2,...,tm] vectors (only t1 is fixed)
		for (auto i = 0U; i < 20U; ++i)
		{
			RowVector<long long> t(randi<Mat<long long> >(1, A.n_rows));
			t(0) = t1;
			if (t*U*A != c)
				assert(!"Valid solution vector t doesn't satisfy the equation");
		}
	}
}

static void TestSolveDiophantineSystemInternal(const MatrixExt<long long>& A, const RowVector<long long>& c, bool has_solution)
{
	MatrixExt<long long> U(A.n_rows, A.n_rows);
	RowVector<long long> t(A.n_rows);
	uword rank_s(0);
	bool result(false);

	if ((result = SolveDiophantineSystem(A, c, U, t, rank_s)) != has_solution)
	{
		result ? assert(!"Diophantine system actually has a solution (solver didn't find it") :
			assert(!"Diophantine system actually doesn't have a solution (solver somehow found it?!?)");
	}
	else if (has_solution)
	{
		// Test solution with certain number of random vectors t=[t1,t2,...,tm] (first rank_s components of
		// vector t are determined and the rest is randomly chosen).
		for (auto i = 0U; i < 20U; ++i)
		{
			RowVector<int> t_rand(randi<Mat<int> >(1, t.n_cols - rank_s));
			for (auto j = rank_s, k = static_cast<uword>(0); j < t.n_cols; ++j, ++k)
				t(j) = t_rand(k);

			if (!CompareMatrices(t*U*A, c))
				assert(!"Valid solution vector t doesn't satisfy one of the equations");
		}
	}
}

void TestSolveDiophantineEquation()
{
	TestSolveDiophantineEquationInternal({ 4, 6, 4 }, 8, true);
	TestSolveDiophantineEquationInternal({ 3, -3 }, 6, true);
	TestSolveDiophantineEquationInternal({ 10, 14 }, 15, false);
	TestSolveDiophantineEquationInternal({ 55, -89, 41 }, 17, true);
	TestSolveDiophantineEquationInternal({ 14, 21, -35, 28 }, -42, true);
}

void TestSolveDiophantineSystem()
{
	TestSolveDiophantineSystemInternal(
	{ { 2, 1, 0 },{ 6, 3, -2 },{ 4, 0, 3 },{ -2, 5, -1 } },
	{ 4, 2, 8 },
		true);

	TestSolveDiophantineSystemInternal(
	{ { 3, 10 },{ -3, 14 } },
	{ 6, 15 },
		false);

	TestSolveDiophantineSystemInternal(
	{ { 3, 55 },{ 14, -89 },{ 0, 41 } },
	{ 15, 17 },
		true);

	TestSolveDiophantineSystemInternal(
	{ { 10, 55 },{ 13, -89 },{ 0, 41 } },
	{ 15, 17 },
		true);

	TestSolveDiophantineSystemInternal(
	{ { 1, -2, 5 },{ 3, -1, 2 },{ -2, 1, 0 },{ 1, 2, -3 } },
	{ 5, 8, 8 },
		true);

	TestSolveDiophantineSystemInternal(
	{ { 1, 0, 1, 1 },{ 0, 1, 1, 1 },{ -1, -1, 0, -1 },{ -1, -1, -1, -1 } },
	{ -2, 2, 1, -2 },
		true);
}
