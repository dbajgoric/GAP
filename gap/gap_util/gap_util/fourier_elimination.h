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

#ifndef GAP_UTIL_FOURIER_ELIMINATION_H
#define GAP_UTIL_FOURIER_ELIMINATION_H

#include <vector>
#include <utility>
#include "row_vector.h"
#include "rational_number.hpp"

namespace gap
{
namespace util
{

	/**
		* Solves the system of n inequalities with m variables by applying Fourier elimination method. System has the following
		* form:
		*
		* a11x1 + a21x2 + ... + am1xm <= c1
		* a12x1 + a22x2 + ... + am2xm <= c2
		* -
		* -
		* -
		* a1nx1 + a2nx2 + ... + amnxm <= cn
		*
		* or in matrix form: xA = c, where A is (m x n) coefficients matrix and c is (1 x n) integer row vector representing the
		* right-hand side of the system. The steps of the algorithm are as follows:
		*
		* 1) Set r = A.n_rows, s = A.n_cols, T = A, q = C, b = Array<Pair<Mat,Vec>>(A.n_rows), B = Array<Pair<Mat,Vec>>(A.n_rows)
		* 2) For every variable xr, r=1,...,A.n_rows initialize Mat and Vec in b(r) and B(r) in the following way:
		*    2.a) In the b(r) initialize Mat to a (r x 1) zero matrix and Vec to (1 x 1) vector whose only element has the value
		*         of -oo. This means that initially, lower boundary of each variable will be set to -oo. Please check step 5 of
		*         this algorithm for more info about the lower and upper boundaries.
		*    2.b) In the B(r) initialize Mat to a (r x 1) zero matrix and Vec to (1 x 1) vector whose only element has the value
		*         of +00. This means that initially, lower boundary of each variable will be set to +00.
		*
		*    During the course of algorithm these min/max values might change for some variables.
		*
		* 3) Form three vectors, pos_coefficients, neg_coefficients and zero_coefficients in the following way:
		*    - pos_coefficients  -> holds indexes of columns where trj > 0
		*    - neg_coefficients  -> holds indexes of columns where trj < 0
		*    - zero_coefficients -> holds indexes of columns where trj = 0
		*
		*    Looking closely, pos_coefficients identify those inequalities that will determine upper bound of xr and neg_coefficients
		*    identify those inequalities that will determine lower bound of xr, while zero_coefficients identify those inequalities
		*    that have zero value for xr coefficient (they don't affect the value of xr).
		*
		* 4) In this step we need to divide inequalities identified by pos_coefficients and neg_coefficients by the trj coefficient.
		*    If we'd rearrange the inequalities in such a way that those with positive trj go first, followed by those with negative
		*    trj and then those with trj equal to zero, after step 4 we'd get the following system:
		*
		*    (t11/tr1)x1 + (t21/tr1)x2 + ... + (t(r-1)1/tr1)x(r-1) + xr <= c1/tr1
		*    -
		*    -
		*    -
		*    (t1f/trf)x1 + (t2f/trf)x2 + ... + (t(r-1)f/trf)x(r-1) + xr <= cf/trf                                         ====> (here ends the group whose trj > 0)
		*    (t1(f+1)/tr(f+1))x1 + (t2(f+1)/tr(f+1))x2 + ... + (t(r-1)(f+1)/tr(f+1))x(r-1) + xr >= c(f+1)/tr(f+1)
		*    -
		*    -
		*    -
		*    (t1k/trk)x1 + (t2k/trk)x2 + ... + (t(r-1)k/trk)x(r-1) + xr >= ck/trk                                         ====> (here ends the group whose trj < 0)
		*    t1(k+1)x1 + t2(k+1)x2 + ... + t(r-1)(k+1)x(r-1) + 0xr <= c(k+1)
		*    -
		*    -
		*    -
		*    t1nx1 + t2nx2 + ... + t(r-1)nx(r-1) + 0xr <= cn                                                              ====> (here ends the group whose trj = 0)
		*
		* 5) As we're trying to solve the system of INequalities, the solution will be a range of valid values for every variable x1...xm.
		*    These ranges are identified by Arrays b and B, where b will identify lower boundary and B will identify upper boundary for
		*    variables. For value r, br and Br are functions of (r-1) variables xi where i=1,...,r-1. It is obvious that in general
		*    case we cannot determine the concrete lower/upper boundary for xr without knowing conrete values of these variables. This is
		*    why we must form boundary functions br and Br that will evaluate to concrete values once variables xi where i=0...r-1
		*    are set. The question is how to form these functions in a convenient way? The idea is a follows:
		*
		*    Once vector [x1,...,xr-1] is known we want to get boundary b(xr) by means of simple matrix operations. We need to take into
		*    consideration that variable xr can have many 'lower' boundaries - basically, each inequality whose trj is negative will
		*    produce a 'lower' boundary of xr. Looking at the system from step 4) and transforming it to leave only xr at left side leads to:
		*
		*    xr <= c1/tr1 - [(t11/tr1)x1 + (t21/tr1)x2 + ... + (t(r-1)1/tr1)x(r-1)]
		*    xr <= c2/tr2 - [(t12/tr2)x1 + (t22/tr2)x2 + ... + (t(r-1)2/tr2)x(r-1)]
		*    -
		*    -
		*    -
		*    xr <= cf/trf - [(t1f/trf)x1 + (t2f/trf)x2 + ... + (t(r-1)f/trf)x(r-1)]
		*
		*    We obviously have two distinct parts: a const part cj/trj and variable part -1 * [(t1j/trj)x1 + (t2j/trj)x2 + ... + (t(r-1)j/trj)x(r-1)].
		*    The right-hand side of this system can be written via matrices in the following way:
		*
		*                            ___                                          ___
		*                           | t11/tr1       t12/tr2      - - -   t1f/trf     |
		*                           |                                                |
		*                           | t21/tr1       t22/tr2      - - -   t2f/trf     |
		*                           |    -                                  -        |
		*    [x1, x2,...,xr-1, 0] * |    -                                  -        | + [c1/tr1, c2/tr2, ..., cf/trf]
		*                           |    -                                  -        |
		*                           | t(r-1)1/tr1   t(r-1)2/tr2  - - -   t(r-1)f/trf |
		*                           |                                                |
		*                           |    0              0        - - -       0       |
		*                           ----                                          ----
		*
		*    NOTE: we extended vector of variables [x1,...,xr-1] with additional zero column and matrix (tij) with additional zero row to make it
		*    easy to handle the case when r = 1. When r = 1 we are down to the last variable and both lower and upper boundary will be numerical
		*    constants and not linear functions like in the case of variables xr where r=2,...,A.n_rows. Expanding this vector and matrix means that
		*    produc vec * mat will always have at LEAST one row, so the matrix addition presented above will make sense.
		*
		*    Now, even though at this state there are many possible lower boundaries, there can of ofcourse be only one after values x1,...,xm
		*    are computed. The lower boundary has to be chosen in such a way that resulting range for the variable will be minimal or in other
		*    words that the resulting set of values will have the minimal number of elements (only by chosing this lower boundary we will satisfy
		*    all the inequalities). Therefore, we must choose the MAXIMUM of all possible lower boundaries as this boundary will result in the
		*    narrowest set of allowed values when combined with the upper boundary.
		*
		*    Almost the same reasoning apply for the set of upper boundaries B with one note: in the case of upper boundaries we also must
		*    choose the upper boundary in such a way that resulting range for the variable will be minimal or in other words that the result set
		*    will have the minimal number of elements. Therefore, we must choose the MINIMUM of all possible upper boundaries as this boundary
		*    will result in the narrowest set of allowed values whene combined with the lower boundary.
		*
		* 6) If r > 1 go to step 7). Here we have r = 1 which means that we are down to the last variable x1. In this case the matrix that represent
		*    the variable part of the elements of b1 and B1 (that is the matrix formed from the coefficients of the left side of every inequality whose
		*    trj coefficient is not zero) is to be taken as zero - as we have only one variable left.
		*
		*    Find the maximum of lower bounds of b(x1) and store it as b1_max. Find the minimum of upper bounds of B(x1) and store it as B1_min - this
		*    is easy and possible to do as there is only one variable left. Finally, check the indexes stored in zero_coefficients array which now point
		*    to those inequalities without variables of form: 0 <= cj/t1j. IF any of such inequalities is not satisfied or b1_max > B1_min which indicates
		*    that the range for the values of variable x1 is empty set, THE SYSTEM DOESN'T HAVE a solution. TERMINATE THE ALGORITHM.
		*
		*    IF none of these two conditions is satisfied, the system has a solution that is identified by the set of lower and upper boundary functions:
		*
		*    bi(x1,x2,...,xi-1) <= xi <= Bi(x1,x2,...,xi-1) (1 <= i <= m)
		*
		* 7) Now we must calculate the number of inequalities that we will have once we eliminate xr variable. The elimination of the xr variable is
		*    done by setting each lower boundary in br <= than each upper boundary in Br. Remember, the number of lower boundaries is identified by
		*    the number of elements in neg_coefficients, while the length of pos_coefficients identify the number of upper boundaries. We can also have
		*    inequalities whose coefficient trj is zero and their number is identified by the length of zero_coefficients.
		*
		*    So, the number of inequalities is: s' = pos_coefficients.len * neg_coefficients.len + zero_coefficients.
		*
		*    If s' > 0 go to step 8. Here we have s' = 0 which means that new system doesn't have any inequalities. One example of such situation is when
		*    xr doesn't have lower boundary (neg_coefficients is empty), it has upper boundary (pos_coefficients is non-empty) and there are no inequalities
		*    where coefficients trj is zero (zero_coefficients is empty). In such a case s' = 0, and there are no constraints on the variables xi where
		*    1 <= i <= r which means that values for these variables can be chosen arbitrarily! The system HAS A SOLUTION that is identified by the boundary
		*    functions for variables xi where r <= i <= m:
		*
		*    bi(x1,x2,...,xi-1) <= xi <= Bi(x1,x2,...,xi-1) (r <= i <= m). TERMINATE THE ALGORITHM.
		*
		* 8) If s' > s the new system that is formed after eliminating xr has more inequalities than the previous system. Expand the matrix T by (s' - s)
		*    columns so that we have place for additional inequalities (this is an optimization to avoid creating completely new matrix from scratch). New
		*    system of inequalities is formed by setting all lower boundaries in br <= than all upper boundaries of Br. Form new system of inequalities by
		*    moving variable part of Br to the left side (which will actually mean br - Br) and move constant part of br to the right side for each new
		*    inequality. In this way every inequality will be in proper form t1jx1 + t2jx2 + ... + tr-1jxr-1 <= qj. Append all the inequalities for which
		*    coefficient trj was 0 (identified by zero_coeffficients).
		*
		*    Set s = s', r = r - 1 and go to step 3.
		*
		* @param There are two input params: coefficients matrix A and vector of constants that represent the right-hand side of the system. There are two
		* output parameters: std::vector of pairs (matrix,vector) b that identifies two components of the lower bound for each variable - the matrix that
		* represents the variable part for every possible lower bound and the vector that holds constant values for each of the lower bounds. For variable
		* xr, assuming caller has chosen values for variables x1,...,xr-1 the following expression can be used to find the set of lower boundaries for xr:
		*
		* [x1,x2,...,xr-1, 0] * b(xr).Matrix + b(xr).vector and then the maximum of these (r-1) possible boundaries is chosen. std::vector B has the same
		* meaning for the upper boundaries of every variable.
		*
		* @retval Value TRUE if the system has a REAL solution, FALSE otherwise.
		*
		* @throw std::logic_error if A.n_rows == 0 OR A.n_cols == 0 OR A.n_cols != c.n_cols
		* This solver can be used to verify if system of n inequalities of form 0 <= cj is trivialy satisfied (system without variables). Such a system
		* can be verified by setting A to a zero matrix with one row and as many columns as there are inequalities. Vector c should be set as usually to
		* represent the right-hand side of the system. Solver will report TRUE if system is trivially satisfied otherwise FALSE. Output variables b and B
		* will be empty vectors as their values would otherwise be meaningless.
		*
		* @note Fourier Elimination method is used to check if system of inequalities has REAL solution even if both matrix A and vector c are integers! If
		* real solution exists nothing is said about the integer solution. In this case, caller must search through the solution set for the integer vector
		* that satisfies all the inequalities. Even when real solution exists, integer solution might not exist.
		*
		* @see Loop Transformations for Restructuring Compilers, The Foundations (Algorithm 3.2)
		*/

		/**
		* Simple type alias that is used to represent bound of single variable xi. Fourier elimination produces set of these bounds for every variable in
		* system.
		*/
	typedef std::pair<MatrixExt<Rational<long long>>, RowVector<Rational<long long>>> Bound;

	template<typename T>
	bool PerformFourierElimination(const MatrixExt<T>& A,
		const RowVector<T>& c,
		std::vector<Bound>& b,
		std::vector<Bound>& B);


	/**
		* Helper method that aims to enumerate all integer solutions to the system of inequalities solved using PerformFourierElimination() method. The
		* method starts with the variable x1 finding all possible integer values. Each of the integer values for x1 are taken in turn to evaluate bounds
		* for variable x2 and integer values for this variable are calculated. This process is continued until all integer solutions are enumerated. If
		* at the end of algorithm the solution set is empty, FALSE is returned. Otherwise, TRUE is returned to indicate there are integer solutions.
		*
		* @param Two input arguments of type const std::vector<std::pair<MatrixExt<double>, RowVector<double>>>&. The first one represents the set of lower
		* bounds for every variable, while the second one represents the set of upper bounds for each variable. These are returned by the already mentioned
		* PerformFourierElimination() method.
		*
		* There is one output parameter of type std::vector<RowVector<int>>& that will contain all possible integer solutions. Note that this way of
		* representing the solution set is not the most efficient one. The natural way to represent the solution set is via N-ary tree as for each value
		* of variable xi (i=1,...,m-1) we can have k values of variable xi+1 leading to a tree-like structure with as many root nodes as there are integer
		* values of variable x1 (we would actually have one root node connecting all these nodes corresponding to all integer values of variable x1).
		* However, this representation is not most convenient for the caller, as he is mostly interested in set of flat integer solution vectors. To make
		*  the function more user-friendly the solution set is modelled as a std::vector of RowVectors storing each integer solution vector separatelly.
		*
		* @retval TRUE if integer solution(s) exists, FALSE otherwise.
		*
		* @throw std::logic_error if the solution set is infinite, that is if lower or upper or both bounds of at least one variable is not finite. The
		* same exception is thrown if b.size() != B.size().
		*/
	bool EnumerateIntegerSolutions(const std::vector<Bound>& b,
		const std::vector<Bound>& B,
		std::vector<std::vector<RowVector<long long>>>& solution_sets);

} /// namespace util
} /// namespace gap

#endif /// GAP_UTIL_FOURIER_ELIMINATION_H
