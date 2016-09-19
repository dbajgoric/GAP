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

#ifndef GAP_DEP_DEPENDENCE_INFO_H
#define GAP_DEP_DEPENDENCE_INFO_H

#include "row_vector.h"

namespace gap
{
namespace dep
{

	/**
	 * The DependenceInfo structure defines an object used to store calculated dependence information. The struct has following memembers:
	 *
	 * 1) RowVector i that will always hold the value of the loop index vector instance corresponding to statement S (statement instance is S(i))
	 * 2) RowVector j that will always hold the value of the loop index vector instance corresponding to statement T
	 *    NOTE: It is assumed that eithe S < T or S = T, that is either statement S preceeds statement T in program, or they are the same statement.
	 *    Points 1 and 2 suggest that all of the cases i < j, i == j and i > j (lexicographical comparison) are possible. The particular case for
	 *    the given DependenceInfo object will be obvious from the context as these objects are returned as part of two dependence sets. One of
	 *    these corresponds to dependence of T on S (where i <= j) and the other correspond to dependence of S on T (here always i > j).
	 * 3) RowVector d = j - i or d = i - j, the dependence distance.
	 * 4) RowVector sig = signum(d), the dependence direction vector.
	 * 5) uword lev, the level of dependence.
	 *
	 * NOTE: the last three members can be calculated using the first two, however returning all of them to the caller makes his life easier. Constructor
	 * that will derive these three components out of first two is provided.
	 *
	 * @throw If i.n_cols != j.n_cols OR i.n_cols == 0, std::logic_error is thrown. The same exception is thrown when i > j lexicographically.
	 */
	struct DependenceInfo
	{
		util::RowVector<long long> i;
		util::RowVector<long long> j;
		util::RowVector<long long> d;
		util::RowVector<long long> sig;
		arma::uword lev;

		DependenceInfo(
			const util::RowVector<long long>& _i
			, const util::RowVector<long long>& _j)
		
			: i(_i), j(_j)
			, d(_i.n_cols)
			, sig(_i.n_cols)
		{
			if (i.n_cols != j.n_cols || i.n_cols == 0)
				throw std::logic_error("c2cuda::DependenceInfo(): i.n_cols should be equal to j.n_cols and greater than 0");

			d = std::move(util::RowVector<long long>::CompareLexicographically(i, j, lev) == util::RowVector<long long>::RightGreater ?
				j - i : i - j);
			sig = std::move(arma::sign(dynamic_cast<const arma::Mat<long long>&>(d)).eval());
		}

		DependenceInfo(
			const util::RowVector<long long>& _i
			, const util::RowVector<long long>& _j
			, const util::RowVector<long long>& _d
			, arma::uword _lev)

			: i(_i)
			, j(_j)
			, lev(_lev)
			, d(_d), sig(arma::sign(dynamic_cast<const arma::Mat<long long>&>(d)).eval())
		{
			if (i.n_cols != j.n_cols || i.n_cols == 0)
				throw std::logic_error("c2cuda::DependenceInfo(): i.n_cols should be equal to j.n_cols and greater than 0");
		}

		/** The following constructor can be used to initialise uniform dependence object when i and j have zero columns. */
		DependenceInfo(
			const util::RowVector<long long>& _d
			, arma::uword _lev)
		
			: i(0)
			, j(0)
			, d(_d)
			, lev(_lev)
			, sig(arma::sign(dynamic_cast<const arma::Mat<long long>&>(d)).eval())
		{
		}
	};

} /// namespace dep
} /// namespace gap

#endif /// GAP_DEP_DEPENDENCE_INFO_H