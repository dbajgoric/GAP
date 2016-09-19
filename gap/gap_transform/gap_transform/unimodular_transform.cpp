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

#include "unimodular_transform.h"
#include <functional>
#include <algorithm>
#include <stdexcept>

namespace gap
{
namespace tran
{
namespace
{

	/**
	 * @brief For the set of dependence distance vectors d, this methods finds a column vector u such that d*u >= 1 for every
	 * vector d.
	 *
	 * @param
	 *        -> const std::vector<RowVector<int>>& - vector of dependence distance vectors
	 *        -> ColVector<int>& - column vector u that satisfies d*u >= 1 for every d
	 *
	 * @retval Value m (D.n_cols or number of loops in nest) is returned if there are no loop carried dependencies in given loop
	 *         nest. Value k < m is returned if there are such dependencies, and uk is the first nonzero value in vector u starting
	 *         from the back: u = (um, um-1,...,uk,...,u1).
	 * @see Loop Transformations for Restructuring Compilers, Loop Parallelization (Algorithm 3.1 - Hyperplane Method).
	 */
	unsigned int CalcDistanceMultiplierVec(
		const std::vector<gap::util::RowVector<long long>>& distance_vectors
		, gap::util::ColVector<long long>& u)
	{
		size_t m(distance_vectors.begin()->n_cols);

		/**																																*/
		/* Partition set of all distance vectors D into sets D1, D2,..., Dm. Set Di contains all distance vectors with level i. The		*/
		/* sets are stored in such a way that Dm comes first and D1 comes last.															*/
		/**																																*/
		std::vector<std::vector<std::reference_wrapper<const gap::util::RowVector<long long>>>> distance_partitions(m);
		for (auto & dist_vec : distance_vectors)
		{
			arma::uword lev(dist_vec.GetLevel());
			if (lev >= m)
				continue; /** Zero distance vectors are ignored.																		*/

			distance_partitions[m - (lev + 1)].push_back(std::ref(dist_vec));
		}

		unsigned int i(m - 1);
		unsigned int idx_first_nonzero_el = m;
		u.zeros(m, 1); /** Initially filled with zeros.																					*/
		for (auto & partition : distance_partitions)
		{
			if (partition.empty())
			{
				/** If partition is empty, then -oo < ui < +oo. Set ui = 0 in this case.												*/
				u(i) = 0;
			}
			else
			{
				if (idx_first_nonzero_el == m)
					idx_first_nonzero_el = i;

				/** Calculate lower bound for bi <= ui, by taking maximum: max{ (1 - di+1ui+1 - ... - dmum) / di } where d is from Di.	*/
				std::vector<long long> lower_bounds(partition.size());
				std::transform(partition.begin(), partition.end(), lower_bounds.begin(),
					[&u, i](const std::reference_wrapper<const gap::util::RowVector<long long>>& d)
				{
					return std::ceil((1 - d.get() * u) / static_cast<double>(d(i)));
				});

				long long max_low_bound = *std::max_element(lower_bounds.begin(), lower_bounds.end());
				u(i) = max_low_bound <= 1 ? 1 : max_low_bound;
			}

			--i;
		}

		return idx_first_nonzero_el;
	}

} // unnamed namespace

	unsigned int FindInnerLoopTransformMatrix(
		const std::vector<gap::util::RowVector<long long>>& distance_vectors
		, gap::util::MatrixExt<long long>& U)
	{
		if (distance_vectors.empty())
			throw std::logic_error("FindInnerLoopTransformMatrix(): Set of distance vectors cannot be empty.");
		else if (distance_vectors.begin()->n_cols <= 1)
			throw std::logic_error("FindInnerLoopTransformMatrix(): Inner loop unimodular transformation cannot be used for 1-level deep nests.");

		size_t m(distance_vectors.begin()->n_cols);

		/** Find vector u for which d*u >= 1 for every distance vector d.																*/
		gap::util::ColVector<long long> u(m);
		unsigned int idx_first_nonzero_el = CalcDistanceMultiplierVec(distance_vectors, u);
		if (idx_first_nonzero_el == m)
		{
			/** There are no loop carried dependencies in this loop nest. Set U to mxm identity matrix.									*/
			U = std::move(gap::util::MatrixExt<long long>(m, m, arma::fill::eye));
			return m;
		}

		/** Form transformation matrix U by enlarging (m-1 x m-1) identity matrix (to which U was initialized).							*/
		U = std::move(gap::util::MatrixExt<long long>(m - 1, m - 1, arma::fill::eye));
		U.insert_rows(idx_first_nonzero_el, 1, true);
		U.insert_cols(0, u);

		return m - 1;
	}

	unsigned int FindOuterLoopTransformMatrix(
		const std::vector<gap::util::RowVector<long long>>& distance_vectors
		, gap::util::MatrixExt<long long>& U)
	{
		if (distance_vectors.empty())
			throw std::logic_error("FindOuterLoopTransformMatrix(): Set of distance vectors cannot be empty.");
		else if (distance_vectors.begin()->n_cols <= 1)
			throw std::logic_error("FindOuterLoopTransformMatrix(): Outer loop unimodular transformation cannot be used for 1-level deep nests.");

		size_t m(distance_vectors.begin()->n_cols);

		/** 1. Build transpose of Nxm D, mxN matrix D'.																					*/
		gap::util::MatrixExt<long long> D_transpose(m, distance_vectors.size());
		for (auto i = 0U; i < distance_vectors.size(); ++i)
			D_transpose.col(i) = distance_vectors[i].t();

		/** 2. Find mxm matrix V and mxN matrix S such that: V*D' = S where V is unimodular and S is echelon matrix.					*/
		gap::util::MatrixExt<long long> V(m, m);
		gap::util::MatrixExt<long long> S(m, D_transpose.n_cols);
		unsigned int rank_d = D_transpose.ReduceToEchelon(V, S);

		if (rank_d == m)
		{
			/** rank(D) = m -> return 0 as no outer loops in this nest can be parallelized.												*/
			return 0;
		}

		/** 3. Set n = m - rank_d, so that n is number of zero rows in S (or the number of linearly independent columns of D').			*/
		unsigned int n(m - rank_d);

		/** 4. Find vector u for which d*u >= 1 for every distance vector d.															*/
		gap::util::ColVector<long long> u(m);
		unsigned int idx_first_nonzero_el = CalcDistanceMultiplierVec(distance_vectors, u);
		if (idx_first_nonzero_el == m)
		{
			/** There are no loop carried dependencies in this loop nest. Set U to mxm identity matrix and return m.					*/
			U = std::move(gap::util::MatrixExt<long long>(m, m, arma::fill::eye));
			return m;
		}

		/** 5. Construct an m x (n+1) matrix A whose first n columns are last n rows of matrix V, and (n+1)th column is vector u.		*/
		gap::util::MatrixExt<long long> A(m, n + 1);
		for (auto i = 0U; i < n; ++i)
			A.col(i) = V.row(rank_d + i).t();

		A.col(n) = u;

		/** 																															*/
		/* 6. Find mxm unimodular matrix u and mx(n+1) echelon matrix T such that A = U*T. In case diagonal element t(n+1)(n+1) < 0,	*/
		/* multiply row (n+1) of T and column (n+1) of U with -1 (this element will always be nonzero). This unimodular matrix U is		*/	
		/* is the loop nest transformation matrix (we don't really care about matrix T here so it's not modified).						*/
		/**																																*/
		U = std::move(gap::util::MatrixExt<long long>(m, m));
		gap::util::MatrixExt<long long> T(m, n + 1);
		A.ReduceToEchelon2(U, T);
	
		if (T(n, n) < 0)
		{
			U.col(n) *= -1;
		}

		return n;
	}

	void CalculateTransformedNestLimits(
		const gap::util::MatrixExt<long long>& U,
		const gap::util::RowVector<long long>& p0,
		const gap::util::MatrixExt<long long>& P,
		const gap::util::RowVector<long long>& q0,
		const gap::util::MatrixExt<long long>& Q,
		std::vector<gap::util::Bound>& b,
		std::vector<gap::util::Bound>& B)
	{
		if (U.n_rows != U.n_cols || U.n_rows != P.n_rows || U.n_rows != P.n_cols || U.n_rows != Q.n_rows || U.n_rows != Q.n_cols ||
			U.n_rows != p0.n_cols || U.n_rows != q0.n_cols)
			throw std::logic_error("CalculateTransformedNestLimits(): Matrices U, P, Q and vectors p0, q0 are not consistent");

		unsigned int m(U.n_rows);

		/** 1. Calculate matrices V = U^-1 * P and W = U^-1 * Q.																		*/
		gap::util::MatrixExt<long long> U_inv(
			std::move(gap::util::MatrixExt<double>(
				arma::inv(std::move(gap::util::MatrixExt<double>(U))).eval())));

		gap::util::MatrixExt<long long> V(std::move(U_inv * P));
		gap::util::MatrixExt<long long> W(std::move(U_inv * Q));

		/** 2. Form system xA <= c from system p0 <= V*P and W*Q <= q0.																	*/
		gap::util::MatrixExt<long long> A(m, 2 * m);
		gap::util::RowVector<long long> c(2 * m);

		A(arma::span(0, m - 1), arma::span(0, m - 1)) = -1 * V(arma::span(0, m - 1), arma::span(0, m - 1));
		A(arma::span(0, m - 1), arma::span(m, 2 * m - 1)) = W(arma::span(0, m - 1), arma::span(0, m - 1));
		c(0, arma::span(0, m - 1)) = -1 * p0(0, arma::span(0, m - 1));
		c(0, arma::span(m, 2 * m - 1)) = q0(0, arma::span(0, m - 1));

		/** 3. Use PerformFourierElimination() to find lower and upper limits for K1,K2,...,Km.											*/
		if (!gap::util::PerformFourierElimination(A, c, b, B))
		{
			/** This should never happen as long as limits of nest L are correctly specified (even if L -> LU is invalid).				*/
			throw std::runtime_error("CalculateTransformedNestLimits(): Failed to calculate limits for LU (should never happen)");
		}
	}

} /// namespace tran
} /// namespace gap