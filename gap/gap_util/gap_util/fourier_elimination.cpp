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

#include "fourier_elimination.h"
#include "util.h"
#include <limits>
#include <stdexcept>
#include <algorithm>

namespace gap
{
namespace util
{

	template<typename _T>
	bool PerformFourierElimination(const MatrixExt<_T>& A,
		const RowVector<_T>& c,
		std::vector<Bound>& b,
		std::vector<Bound>& B)
	{
		if (A.n_rows == 0 || A.n_cols == 0 || A.n_cols != c.n_cols)
			throw std::logic_error("c2cuda::PerformFourierElimination(): A.n_rows and A.n_cols cannot be zero and A.n_cols must be equal to c.n_cols");

		/** STEP 1: initialize coefficients matrix T and constants vector q that we'll work with throughout the algorithm. */
		MatrixExt<Rational<long long>> T(A);
		RowVector<Rational<long long>> q(c);

		uword curr_variable(A.n_rows);		/** Parameter r in algorithm. */
		uword num_inequalities(A.n_cols);	/** Parameter s in algorithm. */

		/**
		 * Reserve memory for pos_coefficients, neg_coefficients and zero_coefficients up-front, to minimize the number of
		 * reallocations during algorithm. We're assuming any of these vectors will never have more than 2*A.n_cols elements
		 * (number of inequalities in starting system). If this assumption is broken, reallocation will happen.
		 */
		std::vector<uword> pos_coefficients,
						   neg_coefficients,
						   zero_coefficients;

		pos_coefficients.reserve(2 * A.n_cols);
		neg_coefficients.reserve(2 * A.n_cols);
		zero_coefficients.reserve(2 * A.n_cols);

		/** Reserve memory for actual number of unknown variables A.n_rows (for vectors b and B) */
		b.reserve(A.n_rows);
		B.reserve(A.n_rows);
		b.clear();
		B.clear();

		/** STEP 2: set lower and upper boundary for each variable to -oo and +00 respectivelly (these are only initial values). */
		for (auto i = 0U; i < A.n_rows; ++i)
		{
			b.push_back(std::make_pair(MatrixExt<Rational<long long>>(i + 1, 1), RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::min()) })));
			B.push_back(std::make_pair(MatrixExt<Rational<long long>>(i + 1, 1), RowVector<Rational<long long>>({ Rational<long long>(std::numeric_limits<long long>::max()) })));
		}

		//std::cout << std::endl << std::endl << "FOURIER ANALYSIS START\n\n";

		do
		{
			pos_coefficients.clear();
			neg_coefficients.clear();
			zero_coefficients.clear();

			/*std::cout << "Iter start (CURR_VARIABLE = " << curr_variable << ")\n";
			T.print("T(iteration start):");
			q.print("q(iteration start):");*/

			/** STEP 3: calculate pos_coefficients, neg_coefficients and zero_coefficients vectors for this iteration. */
			for (auto j = 0U; j < num_inequalities; ++j)
			{
				if (T(curr_variable - 1, j) > Rational<_T>())
				{
					/** Coefficient trj (inequality j) of variable xr is > 0. */
					pos_coefficients.push_back(j);
				}
				else if (T(curr_variable - 1, j) < Rational<_T>())
				{
					/** Coefficient trj (inequality j) of variable xr is < 0. */
					neg_coefficients.push_back(j);
				}
				else
				{
					/** Coefficient trj (inequality j) of variable xr is == 0. */
					zero_coefficients.push_back(j);
					continue;
				}

				/**
				 * STEP 4: if xr coefficient trj (in inequality j) is != 0, divide entire inequality with trj. In terms of matrix T
				 * and vector q, this means transforming entire column j of matrix T by dividing it with value trj, and transforming
				 * element j of vector q by dividing it with trj.
				 */
				//std::cout << "T(" << curr_variable - 1 << "," << j << ") = " << T(curr_variable - 1) << std::endl;
				q(j) /= T(curr_variable - 1, j);
				T(span(0, curr_variable - 1), j) /= T(curr_variable - 1, j);
				//T.print("T(middle of iteration):");
				//q.print("q(middle of iteration):");
			}

			/**
			 * STEP 5.1: using the pos_coefficients index vector, build upper boundary for unknown xr. This upper boundary consists of
			 * two parts, matrix whose each column corresponds to left side of inequality j where trj > 0 and vector whose each element
			 * corresponds to the right side of inequality j where trj > 0. This step happens after system has been rearranged so that
			 * variable xr is the only element on the left side of inequality j (that's the reason behind -1 * T()).
			 */
			if (pos_coefficients.size() > 0)
			{
				B[curr_variable - 1] = std::make_pair(MatrixExt<Rational<long long>>(curr_variable, pos_coefficients.size()), RowVector<Rational<long long>>(pos_coefficients.size()));
				for (auto i = 0U; i < pos_coefficients.size(); ++i)
				{
					std::transform(T.begin_col(pos_coefficients[i]), T.begin_col(pos_coefficients[i]) + curr_variable - 1, B[curr_variable - 1].first.begin_col(i),
						[](const Rational<long long>& _value) { return -1LL * _value; });
					
					B[curr_variable - 1].second(i) = q(pos_coefficients[i]);
				}
			}
			/** Otherwise, variable xr is not bounded from above (upper boundary is +oo). */

			/** STEP 5.2: similarly, calculate the lower boundary for unknown xr. */
			if (neg_coefficients.size() > 0)
			{
				b[curr_variable - 1] = std::make_pair(MatrixExt<Rational<long long>>(curr_variable, neg_coefficients.size()), RowVector<Rational<long long>>(neg_coefficients.size()));
				for (auto i = 0U; i < neg_coefficients.size(); ++i)
				{
					std::transform(T.begin_col(neg_coefficients[i]), T.begin_col(neg_coefficients[i]) + curr_variable - 1, b[curr_variable - 1].first.begin_col(i),
						[](const Rational<long long>& _value) { return -1LL * _value; });
					
					b[curr_variable - 1].second(i) = q(neg_coefficients[i]);
				}
			}
			/** Otherwise, variable xr is not bounded from below (lower boundary is -oo). */
			
			if (curr_variable == 1)
			{
				/**
				 * STEP 6: we are down to the last variable x1. Find maximum of lower bounds b1_max and minimum of upper bounds B1_min. Check if
				 * all of the inequalities without variables identified by zero_coefficients vector are satisfied (they are of 0 <= qj form). If
				 * any of these doesn't hold or b1_max > B1_min, system DOESN'T HAVE A SOLUTION.
				 */

				bool trivial_ineq_hold(true);
				for (auto i : zero_coefficients)
				{
					if (q(i) < 0LL)
					{
						trivial_ineq_hold = false;	/** No solution. */
						break;
					}
				}
				
				if (A.n_rows == 1 && pos_coefficients.empty() && neg_coefficients.empty())
				{
					/**
					 * In case A.n_rows == 1 and both pos_coefficients and neg_coefficients vectors are empty, we have the situation where solver
					 * was invoked only to verify if the system of n inequalities of form 0 <= cj is trivially satisfied. Vectors b and B should
					 * be empty in this case.
					 */
					b.clear();
					B.clear();
					return trivial_ineq_hold;
				}

				if (!trivial_ineq_hold)
					return false;

				Rational<long long> b1_max(b[curr_variable - 1].second.max());
				Rational<long long> B1_min(B[curr_variable - 1].second.min());

				if (b1_max > B1_min)
					return false;		/** No solution. */

				/** Replace lower and upper boundaries of x1 with b1_max and B1_min values and return true for success. */
				b[curr_variable - 1].first = { { Rational<long long>(0) } };
				b[curr_variable - 1].second = { Rational<long long>(b1_max) };
				B[curr_variable - 1].first = { { Rational<long long>(0) } };
				B[curr_variable - 1].second = { Rational<long long>(B1_min) };

				return true;
			}

			/** STEP 7: calculate the number of inequalities in the new system. */
			uword num_inequalities_new = pos_coefficients.size() * neg_coefficients.size() + zero_coefficients.size();
			if (num_inequalities_new == 0)
			{
				/** 
				 * There are no new inequalities and system has a solution. Variables xj, j=r,...,m are bounded by
				 * the corresponding elements of b and B, while variables xi, i=1,...,r-1 can be chosen arbitrarily.
				 */
				return true;
			}

			/** STEP 8: prepare new system of inequalities by modifying matrix T (coefficients) and vector q (right side of the system). */
			if (num_inequalities_new > num_inequalities)
			{
				/**
				 * New system has num_inequalities_new - num_inequalities more inequalities than the previous one. Extend matrix T and vector
				 * q while preserving existing data (inequalities where trj coefficient is equal to zero are carried over to next iteration).
				 */
				T.resize(T.n_rows, num_inequalities_new);
				q.resize(1, num_inequalities_new);
			}

			/*std::cout << std::endl << "T and q before final modifications" << std::endl;
			T.print("T(before final):");
			q.print("q(before final:");*/

			/**
			 * Copy inequalities identified by zero_coefficients vector to the beginning of matrix T and vector q. This is the easiest way to
			 * avoid over-writting these parts of matrix T and vector q in the step that follows.
			 */
			uword col_index(0);
			for (auto i : zero_coefficients)
			{
				if (i != col_index)
				{
					T(span(0, curr_variable - 2), col_index) = T(span(0, curr_variable - 2), i);
					q(col_index) = q(i);
				}
				++col_index;
			}

			/*std::cout << std::endl << "T and q after moving zero_coefficients to beginning" << std::endl;
			T.print("T:");
			q.print("q:");*/

			/** Form a new system by setting each lower boundary of xr <= than each upper boundary of xr. Append these to matrix T and vector q. */
			for (auto i = 0U; i < b[curr_variable - 1].first.n_cols; ++i)
			{
				for (auto j = 0U; j < B[curr_variable - 1].first.n_cols; ++j)
				{
					std::transform(b[curr_variable - 1].first.begin_col(i), b[curr_variable - 1].first.begin_col(i) + curr_variable - 1,
						B[curr_variable - 1].first.begin_col(j), T.begin_col(col_index),
						[](const Rational<long long>& b_val, const Rational<long long>& B_val) { return b_val - B_val; });

					q(col_index++) = B[curr_variable - 1].second(j) - b[curr_variable - 1].second(i);
				}
			}

			//std::cout << "\n\n\n";
			/*T.print("T(iteration end):");
			q.print("q(iteration end):");*/

			--curr_variable;
			num_inequalities = num_inequalities_new;
		} while (curr_variable >= 1);

		return false;
	}

	namespace
	{

		void EnumerateHelper(const std::vector<Bound>& b,
			const std::vector<Bound>& B,
			const RowVector<Rational<long long>>& partial_solution_vector,
			uword current_variable, /** Counting starts from 0. */
			std::vector<std::vector<RowVector<long long>>>& solution_sets,
			size_t& solution_set_idx)
		{
			if (current_variable >= b.size() || partial_solution_vector.n_cols != current_variable + 1 || partial_solution_vector(current_variable) != Rational<long long>(0))
				throw std::logic_error("c2cuda::EnumerateIntegerSolutions(): internal error occured");

			auto & lower_bound = b[current_variable];
			auto & upper_bound = B[current_variable];

			if (lower_bound.first.n_rows != current_variable + 1 || lower_bound.second.n_rows != 1 || upper_bound.first.n_rows != current_variable + 1 ||
				upper_bound.second.n_rows != 1)
				throw std::logic_error("c2cuda::EnumerateIntegerSolutions(): form of lower and/or upper bounds is not valid");

			/** Both lower and upper bound have to be finite. */
			if (lower_bound.second(0) == std::numeric_limits<long long>::min() || upper_bound.second(0) == std::numeric_limits<long long>::max())
				throw std::logic_error("c2cuda::EnumerateIntegerSolutions(): solution set is infinite");

			/** Find max of lower bounds rounded up to nearest integer.  */
			long long max_lower_bound = static_cast<long long>(std::ceil((partial_solution_vector * lower_bound.first + lower_bound.second).max()));

			/** Find min of upper bounds rounded down to nearest integer. */
			long long min_upper_bound = static_cast<long long>(std::floor((partial_solution_vector * upper_bound.first + upper_bound.second).min()));
			
			if (max_lower_bound > min_upper_bound)
				return;		/** No integer solution found. */

			if (current_variable < b.size() - 1)
			{
				/**
				 * If current_variable < b.size() - 1 we still didn't reach the last variable. For each current variable's integer value
				 * go level deeper to evaluate the rest of the variables.
				 */
				RowVector<Rational<long long>> new_solution_vector(arma::resize(partial_solution_vector, 1, partial_solution_vector.n_cols + 1).eval());
				new_solution_vector(current_variable + 1) = 0;
				for (auto int_val = max_lower_bound; int_val <= min_upper_bound; ++int_val)
				{
					new_solution_vector(current_variable) = int_val;
					EnumerateHelper(b, B, new_solution_vector, current_variable + 1, solution_sets, solution_set_idx);
				}
			}
			else
			{
				/**
				 * We reached the last variable and this is where recursion stops (as there are no more variables to evaluate). At this
				 * point we have values of all variables including the last one. Expand solution_set with min_upper_bound - max_lower_bound + 1
				 * new integer solutions.
				 */
				RowVector<long long> complete_solution_vector(partial_solution_vector);
				for (auto int_val = max_lower_bound; int_val <= min_upper_bound; ++int_val)
				{
					complete_solution_vector(current_variable) = int_val;
					try
					{
						/** If this fails then we're out of continous memory space in current solution set. Switch to next one. */
						solution_sets[solution_set_idx].push_back(complete_solution_vector);
					}
					catch (const std::bad_alloc&)
					{
						/** Add new solution set as we're out of memory space in current one. */
						solution_sets.push_back(std::vector<RowVector<long long>>());
						solution_sets[++solution_set_idx].push_back(complete_solution_vector);
					}
				}
			}
		}
	} // unnamed namespace

	bool EnumerateIntegerSolutions(const std::vector<Bound>& b,
		const std::vector<Bound>& B,
		std::vector<std::vector<RowVector<long long>>>& solution_sets)
	{
		if (b.size() != B.size() || b.empty() || B.empty())
			throw std::logic_error("c2cuda::EnumerateIntegerSolutions(): vectors b and B must have the same size and be non-empty");

		solution_sets.push_back(std::vector<RowVector<long long>>());
		size_t set_idx(0);
		EnumerateHelper(b, B, { 0 }, 0, solution_sets, set_idx);
		return !solution_sets[0].empty();
	}

	//template bool PerformFourierElimination<int>(const MatrixExt<int>& A, const RowVector<int>& c, std::vector<Bound>& b, std::vector<Bound>& B);
	template bool PerformFourierElimination<long long>(const MatrixExt<long long>& A, const RowVector<long long>& c, std::vector<Bound>& b, std::vector<Bound>& B);

} /// namespace util
} /// namespace gap
