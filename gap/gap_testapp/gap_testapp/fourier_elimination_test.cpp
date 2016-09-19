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

#include "fourier_elimination_test.h"
#include <gap_util/fourier_elimination.h>
#include "test_utils.h"
#include <cassert>
#include <vector>
#include <algorithm>
#include <limits>

using namespace gap::util;

static void TestFourierEliminationInternal(
	const MatrixExt<long long>& A,
	const RowVector<long long>& c,
	const std::vector<Bound>& b_expect,
	const std::vector<Bound>& B_expect,
	bool has_solution = true)
{
	std::vector<std::pair<MatrixExt<Rational<long long>>, RowVector<Rational<long long>>>> b, B;
	assert(PerformFourierElimination(A, c, b, B) == has_solution);

	assert(b.size() == b_expect.size() && B.size() == B_expect.size());
	assert(std::equal(b_expect.begin(), b_expect.end(), b.begin(),
		[](const std::pair<MatrixExt<Rational<long long>>, RowVector<Rational<long long>>> v1,
			const std::pair<MatrixExt<Rational<long long>>, RowVector<Rational<long long>>> v2)
		{
			return CompareMatrices(v1.first, v2.first) && CompareMatrices(v1.second, v2.second);
		}));
	assert(std::equal(B_expect.begin(), B_expect.end(), B.begin(),
		[](const std::pair<MatrixExt<Rational<long long>>, RowVector<Rational<long long>>> v1,
			const std::pair<MatrixExt<Rational<long long>>, RowVector<Rational<long long>>> v2)
	{
		return CompareMatrices(v1.first, v2.first) && CompareMatrices(v1.second, v2.second);
	}));
}

static void TestEnumerateSolutionsInternal(
	const MatrixExt<long long>& A,
	const RowVector<long long>& c,
	const std::vector<Bound>& b,
	const std::vector<Bound>& B,
	bool has_int_solution = true,
	bool is_finite = true)
{
	try
	{
		std::vector<std::vector<RowVector<long long>>> solution_sets;
		assert(EnumerateIntegerSolutions(b, B, solution_sets) == has_int_solution);
		if (has_int_solution)
		{
			assert(!solution_sets[0].empty());
			for (auto & solution_set : solution_sets)
			{
				for (const auto & solution : solution_set)
				{
					arma::umat comp_result = (solution * A) <= c;
					assert(arma::all(arma::vectorise(comp_result)));
				}
			}
		}
	}
	catch (const std::logic_error&)
	{
		assert(!is_finite);
	}
}

void TestFourierElimination()
{
	// Test case 1 (positive)
	TestFourierEliminationInternal(
	{ { -1, 2, 0 },{ 1, 0, -10 } },
	{ 0, 5, -23 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>(23, 10) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(23, 10) }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>(5, 2) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(1) }, { Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>() }))
	});

	// Test case 2 (positive)
	TestFourierEliminationInternal(
	{ { 1, 1, -1, 1 },{ 600, -600, 0, 0 } },
	{ 300, -200, 0, 100 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>() })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(1, 600) },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(1, 3) }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>(50) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(-1, 600) },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(1, 2) }))
	});

	// Test case 3 (positive)
	TestFourierEliminationInternal(
	{ { 2, 3, 5 },{ 3, -1, 0 } },
	{ 100, 2, 40 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::min()) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(3) },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(-2) }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>(8) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(-2, 3) },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(100, 3) }))
	});

	// Test case 4 (positive)
	TestFourierEliminationInternal(
	{ { 1, -1, 3 },{ 1, 2, 1 },{ -3, -1, -2 } },
	{ 1, 1, 1 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::min()) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::min()) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(1, 3), -1, Rational<long long>(3, 2) },
		{ Rational<long long>(1, 3), 2, Rational<long long>(1, 2) },{ 0, 0, 0 } }),
			RowVector<Rational<long long>>({ Rational<long long>(-1, 3), -1, Rational<long long>(-1, 2) }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::max()) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::max()) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() },{ Rational<long long>() },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::max()) }))
	});

	// Test case 5 (positive)
	TestFourierEliminationInternal(
	{ { 2, -1, 2, -1, 0, 0 },{ 3, 1, 1, 0, -1, 0 },{ -1, -1, 1, 0, 0, -1 } },
	{ 3, 2, 4, 0, 0, 0 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>() })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>() })),
		std::make_pair(MatrixExt<Rational<long long>>({ { 2, -1, 0 },{ 3, 1, 0 },{ 0, 0, 0 } }), RowVector<Rational<long long>>({ -3, -2, 0 }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>(7, 4) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { -1, Rational<long long>(-1, 2), -2 },{ 0, 0, 0 } }),
			RowVector<Rational<long long>>({ Rational<long long>(7, 4), 3, 4 })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(-2) },{ Rational<long long>(-1) },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(4) }))
	});

	// Test case 6 (positive)
	TestFourierEliminationInternal(
	{ { 3, -2, 1, 1, 0, 0 },{ -5, -1, -6, 0, 2, -3 },{ 1, 3, 4, -5, -3, 0 },{ 2, -8, 1, -7, 0, 5 } },
	{ 6, -10, 100, -30, 10, -100 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::min()) })),
		std::make_pair(
			MatrixExt<Rational<long long>>(
	{
		{
			Rational<long long>(30, 49), Rational<long long>(18, 77), Rational<long long>(-10, 19), Rational<long long>(3, 10),
			Rational<long long>(573, 924), Rational<long long>(823, 1372), Rational<long long>(35, 68), Rational<long long>(79, 133),
				Rational<long long>(95, 224), Rational<long long>(65, 392), Rational<long long>(-35, 208), Rational<long long>(15, 73)
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		}
	}),
			RowVector<Rational<long long>>(
	{
		Rational<long long>(-16, 7), Rational<long long>(-2720, 77), Rational<long long>(800, 19), -28, Rational<long long>(3, 11),
		Rational<long long>(-435, 343), Rational<long long>(235, 51), Rational<long long>(-12, 7), Rational<long long>(25, 3),
				Rational<long long>(250, 49), Rational<long long>(425, 13), Rational<long long>(400, 219)
	})),
		std::make_pair(
			MatrixExt<Rational<long long>>(
	{
		{ 0, Rational<long long>(23, 3), Rational<long long>(1, 5) },
		{ Rational<long long>(2, 3) , Rational<long long>(-35, 3), Rational<long long>(-21, 25) },
		{ 0, 0, 0 }
	}),
			RowVector<Rational<long long>>(
	{
		Rational<long long>(-10, 3), 6, 34
	})),
		std::make_pair(
			MatrixExt<Rational<long long>>(
	{
		{ Rational<long long>(-1, 4), Rational<long long>(1, 7) },
		{ Rational<long long>(-1, 8), 0 },
		{ Rational<long long>(3, 8), Rational<long long>(-5, 7) },
		{ 0, 0 }
	}),
			RowVector<Rational<long long>>(
	{
		Rational<long long>(5, 4), Rational<long long>(30, 7)
	}))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::max()) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::max()) })),
		std::make_pair(
			MatrixExt<Rational<long long>>(
	{
		{ Rational<long long>(-10, 7), Rational<long long>(-6, 35), Rational<long long>(2, 3), Rational<long long>(-8, 23) },
		{ 3, Rational<long long>(7, 5), Rational<long long>(29, 15), Rational<long long>(42, 23) },
		{ 0, 0, 0, 0 }
	}),
			RowVector<Rational<long long>>(
	{
		2, Rational<long long>(158, 7), Rational<long long>(-170, 3), Rational<long long>(670, 23)
	})),
		std::make_pair(
			MatrixExt<Rational<long long>>(
	{
		{ Rational<long long>(-3, 2), -1, 0 },
		{ Rational<long long>(5, 2), 6, Rational<long long>(3, 5) },
		{ Rational<long long>(-1, 2), -4, 0 },
		{ 0, 0, 0 }
	}),
			RowVector<Rational<long long>>(
	{
		3, 100, -20
	}))
	});

	// Test case 7 (positive)
	TestFourierEliminationInternal(
	{ { 3, -3, -3, -1, 0, 3, -3, -2 },{ 1, 1, -1, -1, -1, 0, 0, 0 } },
	{ 103, -1, -4, -2, -1, 101, -2, -2 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>(1) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { -3, -1, 0 },{ 0, 0, 0 } }),
			RowVector<Rational<long long>>({ 4, 2, 1 }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>(101, 3) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(-3), 3 },{ 0, 0 } }), RowVector<Rational<long long>>({ 103, -1 }))
	});

	// Test case 8 (positive and trivial)
	TestFourierEliminationInternal(
	{ { 0, 0, 0, 0, 0, 0, 0, 0 } },
	{ 103, 0, 4, 52, 11, 101, 99, 18892 },
	{},
	{});

	// Test case 9 (negative and trivial)
	TestFourierEliminationInternal(
	{ { 0, 0, 0, 0, 0, 0, 0, 0 } },
	{ 103, 0, 4, 52, 11, -101, 99, -18892 },
	{},
	{},
		false);
}

void TestEnumerateIntegerSolutions()
{
	// Test case 1 (negative)
	TestEnumerateSolutionsInternal(
	{ { -1, 2, 0 },{ 1, 0, -10 } },
	{ 0, 5, -23 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>(23, 10) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(23, 10) }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>(5, 2) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(1) },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>() }))
	},
		false);

	// Test case 2 (negative)
	TestEnumerateSolutionsInternal(
	{ { 1, 1, -1, 1 },{ 600, -600, 0, 0 } },
	{ 300, -200, 0, 100 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>() })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(1, 600) },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(1, 3) }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>(50) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(-1, 600) },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(1, 2) }))
	},
		false);

	// Test case 3 (infinite)
	TestEnumerateSolutionsInternal(
	{ { 2, 3, 5 },{ 3, -1, 0 } },
	{ 100, 2, 40 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::min()) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(3) },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(-2) }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>(8) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(-2, 3) },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(100, 3) }))
	},
		true,
		false);

	// Test case 4 (infinite)
	TestEnumerateSolutionsInternal(
	{ { 1, -1, 3 },{ 1, 2, 1 },{ -3, -1, -2 } },
	{ 1, 1, 1 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::min()) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::min()) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(1, 3), -1, Rational<long long>(3, 2) },
		{ Rational<long long>(1, 3), 2, Rational<long long>(1, 2) },{ 0, 0, 0 } }),
			RowVector<Rational<long long>>({ Rational<long long>(-1, 3), -1, Rational<long long>(-1, 2) }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::max()) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::max()) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() },{ Rational<long long>() },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::max()) }))
	},
		true,
		false);

	// Test case 5 (positive)
	TestEnumerateSolutionsInternal(
	{ { 2, -1, 2, -1, 0, 0 },{ 3, 1, 1, 0, -1, 0 },{ -1, -1, 1, 0, 0, -1 } },
	{ 3, 2, 4, 0, 0, 0 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>() })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>() })),
		std::make_pair(MatrixExt<Rational<long long>>({ { 2, -1, 0 },{ 3, 1, 0 },{ 0, 0, 0 } }), RowVector<Rational<long long>>({ -3, -2, 0 }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>(7, 4) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { -1, Rational<long long>(-1, 2), -2 },{ 0, 0, 0 } }),
			RowVector<Rational<long long>>({ Rational<long long>(7, 4), 3, 4 })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(-2) } ,{ Rational<long long>(-1) },{ Rational<long long>() } }),
			RowVector<Rational<long long>>({ Rational<long long>(4) }))
	});

	// Test case 7 (positive)
	TestEnumerateSolutionsInternal(
	{ { 3, -3, -3, -1, 0, 3, -3, -2 },{ 1, 1, -1, -1, -1, 0, 0, 0 } },
	{ 103, -1, -4, -2, -1, 101, -2, -2 },
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }), RowVector<Rational<long long>>({ Rational<long long>(1) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { -3, -1, 0 },{ 0, 0, 0 } }), RowVector<Rational<long long>>({ 4, 2, 1 }))
	},
	{
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>() } }),
		RowVector<Rational<long long>>({ Rational<long long>(101, 3) })),
		std::make_pair(MatrixExt<Rational<long long>>({ { Rational<long long>(-3), 3 },{ 0, 0 } }), RowVector<Rational<long long>>({ 103, -1 }))
	});
}