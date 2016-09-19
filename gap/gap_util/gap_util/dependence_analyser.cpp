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

#include "dependence_analyser.h"
#include "diophantine.h"
#include "fourier_elimination.h"
#include <vector>
#include <list>

namespace gap
{
namespace dep
{

namespace
{
	void ValidateArguments(
		const util::MatrixExt<long long>& A,
		const util::RowVector<long long>& a0,
		const util::MatrixExt<long long>& B,
		const util::RowVector<long long>& b0,
		const util::MatrixExt<long long>& P,
		const util::RowVector<long long>& p0,
		const util::MatrixExt<long long>& Q,
		const util::RowVector<long long>& q0)
	{
		if (A.is_empty() || a0.is_empty() || B.is_empty() || b0.is_empty() || P.is_empty() || p0.is_empty() || Q.is_empty() || q0.is_empty())
			throw std::logic_error("c2cuda::ValidateArguments(): matrices A, B, P and Q and vectors a0, b0, p0 and q0 must be non-empty");

		if (A.n_rows != B.n_rows || A.n_cols != B.n_cols || a0.n_cols != b0.n_cols || A.n_cols != a0.n_cols || B.n_cols != b0.n_cols)
			throw std::logic_error("c2cuda::ValidateArguments(): matrices A, B and vectors a0, b0 are not compatible");

		if (P.n_rows != P.n_cols || Q.n_rows != Q.n_cols || P.n_rows != Q.n_rows || p0.n_cols != q0.n_cols || p0.n_cols != P.n_rows)
			throw std::logic_error("c2cuda::ValidateArguments(): matrices P, Q and vectors p0 and q0 are not compatible");

		if (A.n_rows != P.n_rows)
			throw std::logic_error("c2cuda::ValidateArguments(): matrices P,Q must have the same number of rows as matrices A, B");
	}

} /// unnamed namespace

	bool GeneralLinearDependenceTest(
		const util::MatrixExt<long long>& A,
		const util::RowVector<long long>& a0,
		const util::MatrixExt<long long>& B,
		const util::RowVector<long long>& b0,
		const util::MatrixExt<long long>& P,
		const util::RowVector<long long>& p0,
		const util::MatrixExt<long long>& Q,
		const util::RowVector<long long>& q0,
		std::vector<DependenceInfo>& T_on_S,
		std::vector<DependenceInfo>& S_on_T,
		bool are_distinct)
	{

		/** Validate arguments. */
		ValidateArguments(A, a0, B, b0, P, p0, Q, q0);

		/** STEP 1: form a system of diophantine equations suitable for diophantine solver. W is formed by appending rows of -B to matrix A. */
		util::MatrixExt<long long> W(arma::resize(A, 2 * A.n_rows, A.n_cols));
		W(arma::span(A.n_rows, W.n_rows - 1), arma::span(0, W.n_cols - 1)) = -1LL * B;

		/** STEP 2: solve this system of diophantine equations. */
		util::MatrixExt<long long> U(W.n_rows, W.n_rows);
		util::RowVector<long long> t(W.n_rows);
		arma::uword rank_s;

		if (!SolveDiophantineSystem(W, b0 - a0, U, t, rank_s))
		{
			/** System has no integer solution. These variables cannot cause dependence between S and T. */
			return false;
		}

		/**
		 * STEP 3, 4 and 5.1 and 5.2(part 1): calculate matrices U1*P, U1*Q, U2*P and U2*Q part of system of inequalities formed by loop
		 * nest bounds. Matrix U is composed of two (equaly sized) matrices [U1 | U2] that correspond to vectors i and j respectively.
		 */
		util::MatrixExt<long long> U1(U(arma::span(0, U.n_rows - 1), arma::span(0, A.n_rows - 1)));
		util::MatrixExt<long long> U2(U(arma::span(0, U.n_rows - 1), arma::span(A.n_rows, U.n_rows - 1)));

		util::MatrixExt<long long>
			IP(U1 * P),
			IQ(U1 * Q),
			JP(U2 * P),
			JQ(U2 * Q);

		/**
		 * Form a system of 4m inequalities (m is number of loops) with (4m - rank_s) variables -> number of undetermined components of vector t.
		 * System must be in t_unkown*Z <= v form, where Z is (4m - rank_s) x (4m) matrix and v is (1 x 4m) row vector.
		 *
		 * NOTE: in case where every component of t is determined, we only have to verify that solution vectors i (t*U1) and j (t*U2) trivially
		 * satisfies the system of inequalities imposed by loop nest boundaries. In this case Z is set to (1 x 4m) zero matrix.
		 */
		util::MatrixExt<long long> Z(U.n_rows == rank_s ? 1 : U.n_rows - rank_s, 4 * A.n_rows);
		util::RowVector<long long> v(4 * A.n_rows);

		if (U.n_rows > rank_s)
		{
			/**
			 * There are 4m - rank_s undetermined components of t. We have to fill in the matrix Z so that it will contain proper coefficients for
			 * these components in the system t_unknown*Z <= v.
			 */
			arma::uword start_col(0);
			for (auto i : { &IP, &JP, &IQ, &JQ })
			{
				/**
				 * In case of matrices U1*P and U2*P, the lower 4m - rank_s rows (that are actually the coefficients for undetermined components
				 * of vector t) has to be moved to left side of the system. This is why these are multiplied by -1.
				 */
				int sign(1);
				if (i == &IP || i == &JP)
					sign = -1;

				Z(arma::span(0, Z.n_rows - 1), arma::span(start_col, start_col + P.n_cols - 1)) =
					sign * (*i)(arma::span(rank_s, U.n_rows - 1), arma::span(0, P.n_cols - 1));
				start_col += P.n_cols;
			}
		}

		/** Similarly, we have to fill in the vector v that will represent the right-hand side of the system of inequalities t_unknown*Z <= v. */
		{
			arma::uword start_col(0);
			for (auto i : { &IP, &JP, &IQ, &JQ })
			{
				/**
				 * In case of matrices U1*Q and U2*Q, the upper rank_s rows (that corresponds to part of the right-hand side of the system of inequalites)
				 * need to be moved to the right side. This is why these are multiplied by -1 after multiplication with the known component of t.
				 */
				int sign(1);
				if (i == &IQ || i == &JQ)
					sign = -1;

				/** We must chose either p0 or q0 depending on the matrix we currently work with. */
				const util::RowVector<long long>& p0_or_q0(i == &IP || i == &JP ? p0 : q0);

				v(0, arma::span(start_col, start_col + P.n_cols - 1)) =
					sign * t(0, arma::span(0, rank_s - 1)) * (*i)(arma::span(0, rank_s - 1), arma::span(0, P.n_cols - 1)) +
					-sign * p0_or_q0(0, arma::span(0, p0_or_q0.n_cols - 1));
				start_col += P.n_cols;
			}
		}

		std::vector<util::Bound> low_bound, upper_bound;
		if (!util::PerformFourierElimination(Z, v, low_bound, upper_bound))
		{
			/**
			 * System of inequalities has no REAL solution. There are two possibilites:
			 * 1) All components of t are determined, however corresponding vectors i and j are outside the loop nest boundary
			 * 2) It is impossible to find values for undetermined components of t such that corresponding solution vectors i
			 *    and j will fall within loop nest boundary.
			 *
			 * Variables cannot cause dependence between S and T.
			 */
			return false;
		}

		/**
		 * STEPS 5.2 (part 2), 6 and 7: form set that represents dependence of T on S, and S on T. For every dependence, calculate
		 * dependence distance and direction vectors, as well as level of dependence.
		 */
		if (U.n_rows == rank_s)
		{
			/**
			 * Vectors i = tU1 and j=tU2 trivially satisfy the loop nest bounds. As all components of t are determined, there is only
			 * one solution and we have to decide if T depend on S or the other way around.
			 */
			util::RowVector<long long> i(t*U1);
			util::RowVector<long long> j(t*U2);
			switch (util::RowVector<long long>::CompareLexicographically(i, j))
			{
			case util::RowVector<long long>::RightGreater:	/** i < j (lexicographically). */
				T_on_S.push_back(DependenceInfo(i, j));
				break;

			case util::RowVector<long long>::LeftGreater:	/** i > j (lexicographically). */
				S_on_T.push_back(DependenceInfo(i, j));
				break;

			default:							/** i == j leads to dependence only when S < T (S and T are distinct statements). */
				if (are_distinct)
					T_on_S.push_back(DependenceInfo(i, j));
				break;
			}
		}
		else
		{
			/**
			 * Invoke EnumerateIntegerSolutions() to find all integer values of undetermined components of vector t, for which solution
			 * vectors i and j fall within loop limits. If there are no integer solutions, these variable cannot cause dependence between
			 * statements S and T.
			 */
			//std::vector<RowVector<long long>> all_t_undetermined;
			std::vector<std::vector<util::RowVector<long long>>> all_t_undetermined_sets;
			if (!EnumerateIntegerSolutions(low_bound, upper_bound, all_t_undetermined_sets))
				return false;

			/** Reserve some memory to minimize the number of reallocations when pushing new elements to vectors. */
			//T_on_S.reserve(static_cast<size_t>(all_t_undetermined.size() * 0.7));
			//S_on_T.reserve(static_cast<size_t>(all_t_undetermined.size() * 0.7));
			T_on_S.reserve(static_cast<size_t>(all_t_undetermined_sets[0].size() * 0.7));
			S_on_T.reserve(static_cast<size_t>(all_t_undetermined_sets[0].size() * 0.7));

			/** We're taking into consideration that all integers solutions might not be able to fit into single std::vector. */
			for (auto & all_t_undetermined : all_t_undetermined_sets)
			{
				for (const auto & single_t : all_t_undetermined)
				{
					t(0, arma::span(rank_s, t.n_cols - 1)) = single_t;
					util::RowVector<long long> i(t*U1);
					util::RowVector<long long> j(t*U2);
					arma::uword lev;

					switch (util::RowVector<long long>::CompareLexicographically(i, j, lev))
					{
					case util::RowVector<long long>::RightGreater:	/** i < j (lexicographically). */
						T_on_S.push_back(DependenceInfo(i, j, j - i, lev));
						break;

					case util::RowVector<long long>::LeftGreater:	/** i > j (lexicographically). */
						S_on_T.push_back(DependenceInfo(i, j, i - j, lev));
						break;

					default:							/** i == j leads to dependence only when S < T (S and T are distinct statements). */
						if (are_distinct)
							T_on_S.push_back(DependenceInfo(i, j, util::RowVector<long long>(i.n_cols), lev));
						break;
					}
				}
			}
		}

		return !(T_on_S.empty() && S_on_T.empty());
	}

	bool UniformLinearDependenceTest(
		const util::MatrixExt<long long>& A,
		const util::RowVector<long long>& a0,
		const util::RowVector<long long>& b0,
		const util::MatrixExt<long long>& P,
		const util::RowVector<long long>& p0,
		const util::RowVector<long long>& q0,
		std::vector<DependenceInfo>& T_on_S,
		std::vector<DependenceInfo>& S_on_T,
		bool are_distinct)
	{
		/** Validate arguments. */
		ValidateArguments(A, a0, A, b0, P, p0, P, q0);

		/** STEP 1: solve system of diophantine equations kA = a0 - b0 where k = j - i. */
		util::MatrixExt<long long> U(A.n_rows, A.n_rows);
		util::RowVector<long long> t(A.n_rows);
		arma::uword rank_s;

		if (!SolveDiophantineSystem(A, a0 - b0, U, t, rank_s))
		{
			/** Variables X(iA + a0) and X(jA + b0) don't cause dependence between S and T. */
			return false;
		}

		/**
		 * STEP 2: verify if system of inequalities p0 - q0 <= tUP <= q0 - p0. We must transform this into t_unknown * Z <= v system
		 * where Z is (2m - rank_s x 2m) coefficient matrix and v is (1 x 2m) vector representing right-hand side of the system.
		 *
		 * NOTE: when m == rank_s every component of vector t is determined. In this case we only need to check if the system of form
		 * 0 <= v - t_unknown * Z is trivially satisfied. In this case matrix Z is (1 * 2m) zero matrix.
		 */
		util::MatrixExt<long long> Z(U.n_rows == rank_s ? 1 : U.n_rows - rank_s, 2 * U.n_rows);
		util::RowVector<long long> v(2 * U.n_rows);
		util::MatrixExt<long long> UP(std::move(U * P));

		if (U.n_rows > rank_s)
		{
			/**
			 * Fill matrix Z using bottom 2m - rank_s rows of matrix U*P from system: -tUP <= q0 - p0 and tUP <= q0 - p0. First m columns
			 * come from tUP <= q0 - p0 taking bottom wm - rank_s rows of matrix U*P. Last m colums come from -tUP <= q0 - p0 so they are
			 * inverted copy of first m columns.
			 */
			Z(arma::span::all, arma::span(0, UP.n_rows - 1)) = UP(arma::span(rank_s, UP.n_rows - 1), arma::span::all);
			Z(arma::span::all, arma::span(UP.n_rows, 2 * UP.n_rows - 1)) = -1 * Z(arma::span::all, arma::span(0, UP.n_rows - 1));
		}

		{
			/**
			 * Fill vector v by combining determined part of vector t*U*P with vectors p0 and q0. First m columns are obtained from tUP <= q0 - p0
			 * inequality. Last m columns are obtained from -tUP <= q0 - p0 inequality.
			 */
			util::RowVector<long long> q0_minus_p0(std::move(q0 - p0));
			util::RowVector<long long> tmp(std::move((t(0, arma::span(0, rank_s - 1)) * UP(arma::span(0, rank_s - 1), arma::span::all)).eval()));
			v(0, arma::span(0, UP.n_rows - 1)) = std::move(-1LL * tmp + q0_minus_p0);
			v(0, arma::span(UP.n_rows, 2 * UP.n_rows - 1)) = std::move(tmp + q0_minus_p0);
		}

		std::vector<util::Bound> lower_bound, upper_bound;
		if (!util::PerformFourierElimination(Z, v, lower_bound, upper_bound))
		{
			/**
			 * System is either not trivially satisfied or it is impossible to find such undetermined components of t so that system of inequalities
			 * p0 - q0 <= tUP <= q0 - p0 will be satisfied. Variable X(iA + a0) and X(jA + b0) cannot cause dependence between S and T.
			 */
			return false;
		}

		/** STEP 3: extract dependence info. */
		if (U.n_rows == rank_s)
		{
			/** STEP 3.1: There is single solution vector t based on which dependence info can be calculated. */
			util::RowVector<long long> k(std::move(t * U));
			arma::uword lev(0);
			switch (util::RowVector<long long>::CompareLexicographically(k, util::RowVector<long long>(k.n_cols), lev))
			{
			case util::RowVector<long long>::LeftGreater:		/** k > 0 (means i < j lexicographically) */
				T_on_S.push_back(DependenceInfo(k, lev));
				break;

			case util::RowVector<long long>::RightGreater:		/** k < 0 ( means i > j) */
				S_on_T.push_back(DependenceInfo(-1LL * k, lev));
				break;

			default:								/** k == 0 (means i == j) leads to dependence only when S and T are distinct statements */
				if (are_distinct)
					T_on_S.push_back(DependenceInfo(k, lev));
				break;
			}
		}
		else
		{
			/** STEPs 2.2 and 3.2: enumerate all integer solutions for unknown components of t and extract dependence info. */
			//std::vector<RowVector<long long>> all_t_undetermined;
			std::vector<std::vector<util::RowVector<long long>>> all_t_undetermined_sets;
			if (!EnumerateIntegerSolutions(lower_bound, upper_bound, all_t_undetermined_sets))
				return false;

			/** Reserve some memory to minimize the number of reallocations when pushing new elements to vectors. */
			T_on_S.reserve(static_cast<size_t>(all_t_undetermined_sets[0].size() * 0.7));
			S_on_T.reserve(static_cast<size_t>(all_t_undetermined_sets[0].size() * 0.7));

			for (auto & all_t_undetermined : all_t_undetermined_sets)
			{
				for (const auto & single_t : all_t_undetermined)
				{
					t(0, arma::span(rank_s, t.n_cols - 1)) = single_t;
					util::RowVector<long long> k(std::move(t * U));
					arma::uword lev;

					switch (util::RowVector<long long>::CompareLexicographically(k, util::RowVector<long long>(k.n_cols), lev))
					{
					case util::RowVector<long long>::LeftGreater:		/** k > 0 (means i < j lexicographically) */
						T_on_S.push_back(DependenceInfo(k, lev));
						break;

					case util::RowVector<long long>::RightGreater:		/** k < 0 ( means i > j) */
						S_on_T.push_back(DependenceInfo(-1LL * k, lev));
						break;

					default:								/** k == 0 (means i == j) leads to dependence only when S and T are distinct statements */
						if (are_distinct)
							T_on_S.push_back(DependenceInfo(k, lev));
						break;
					}
				}
			}
		}

		return !(T_on_S.empty() && S_on_T.empty());
	}

} /// namespace dep
} /// namespace gap