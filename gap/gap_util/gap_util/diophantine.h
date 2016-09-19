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

#ifndef GAP_UTIL_DIOPHANTINE_H
#define GAP_UTIL_DIOPHANTINE_H

#include "row_vector.h"

namespace gap
{
namespace util
{

	/**
	 * @brief Solves single Diophantine equation with m variables using echelon reduction algorithm.
	 * Equation is given in the form: a1x1 + a2x2 + ... + amxm = c, or in matrix form: xA = c (where
	 * x is m-row vector and c is a constant). The steps of the algorithm are as follows:
	 *
	 * 1) Reduce coefficients column vector A=[a1,...,am] to echelon form [s11,0,...,0]. As Theorem 3.4
	 *    (Loop Transformations for Restructuring Compilers, The Foundations) states, modul of s11 is
	 *    actually equal to the GCD of the coefficients: GCD(a1,a2,...,am)=|s11|.
	 * 2) Step 1 has produced unimodular matrix U and echelon column vector S such that UA=S. Starting from the
	 *    matrix form of diophantine equation we get:
	 *    c = xA =>
	 *    c = x(U^-1)UA => set t=x(U^-1) =>
	 *    c = tUA =>
	 *    c = tS =>
	 *    c = [t1,t2,...,tm] * [s11,0,...,0]'
	 *    c = t1*s11
	 *    t1 = c/s11
	 *
	 *    If s11 equally divides (without remainder) constant c (right-hand side of the equation), diophantine
	 *    equation has a solution. This solution is described by vector t=[c/s11, t2, t3,...,tm] where integers
	 *    t2,...,tm can be arbitrarily chosen and unimodular matrix U. As long as t1=c/s11 (and of course c % s11 == 0)
	 *    equality will be satisfied.
	 * 3) Vector x can be derived easily from the vector t (this step is done by the caller!):
	 *    t = x(U^-1) /<-- U
	 *    x = tU.
	 *
	 * Method has two input arguments, a ColVector object representing coefficient vector A and an integer c that is
	 * the right-hand side of the diophantine equation. There aret two output arguments: unimodular matrix U and
	 * an integer that will hold the value of t1. This method only calculated the general solution, and it is up
	 * to the caller to choose the concrete set of solutions.
	 *
	 * In case given diophantine equation has solution method will return TRUE and parameters U and t1 are set to
	 * proper values. In case there is no solution, method will return FALSE in which case parameters U and t1 must
	 * be ignored.
	 *
	 * @throw std::logic_error if A.n_rows == 0.
	 * @see Loop Transformations for Restructuring Compilers, The Foundations (Theorem 3.5)
	 */
	bool SolveDiophantineEquation(const ColVector<long long>& A,
		const long long c,
		MatrixExt<long long>& U,
		long long& t1);

	/**
	 * Solves the system of n Diophantine equations with m variables using echelon reduction algorithm. System has the
	 * following form:
	 *
	 * a11x1 + a21x2 + ... + am1xm = c1
	 * a12x1 + a22x2 + ... + am2xm = c2
	 * -
	 * -
	 * -
	 * a1nx1 + a2nx2 + ... + amnxm = c
	 *
	 * or in matrix form: xA = c, where A is (m x n) coefficients integer matrix and c is (1 x n) integer row vector
	 * representing right-hand side of the system. The steps of the algorithm are as follows:
	 *
	 * 1) Reduce coefficient matrix A to echelon form. This step produces unimodular matrix U and echelon matrix S
	 *    such that UA = S.
	 * 2) Starting from the matrix form of diophantine equation we get:
	 *    c = xA =>
	 *    c = x(U^-1)UA => set t=x(U^-1) =>
	 *    c = tUA =>
	 *    c = tS => where t is a (1 x m) vector and S is an (m x n) echelon matrix calculated in step 1.
	 *
	 *    We reduced complex system xA = c to much simpler form tS = c. This system is easy to solve because S is an
	 *    echelon matrix. In order to check if entire problem has a solution, we need to solve the system tS = c and
	 *    find determined component of the vector t. Let's assume that matrix S has the following form:
	 *
	 *         ___       ___
	 *        | s11 s12 s13 |
	 *        |             |
	 *    S = | 0   s22 s23 |
	 *        |             |
	 *        | 0   0   0   |
	 *        ---         ---
	 *
	 *    c = tU = [s11t1, s12t1 + s22t2, s13t1 + s23t2], and this translates into following system:
	 *
	 *    s11t1         = c1
	 *    s12t1 + s22t2 = c2
	 *    s13t1 + s23t2 = c3
	 *
	 *    Such a system is trivial to solve: firstly, calculate t1 and reuse it to calculate t2 from the second equation. If
	 *    both variables are integers, replace them into third equation and check if it holds. If it does, the system has a
	 *    solution! Solution is represented by vector t=[c1 / s11, (s11c2 - s12c1) / (s11s22), t3) where t3 is an arbitrary
	 *    integer and unimodular matrix U.
	 * 3) Vector x can be derived easily from the vector t (this step is done by the caller!):
	 *    t = x(U^-1) /<-- U
	 *    x = tU.
	 *
	 * @param Method has two input arguments, a (m x n) coefficient matrix A and an integer (1 x n) vector c represents
	 * the right-hand side of the system.
 
	 * There are three output arguments: (m x m) unimodular matrix U and (1 x m) row vector t that holds rank(S) determined
	 * components of solution vector (rank(S) is number of non-zero rows of S, as S is echelon matrix). The third output
	 * argument is an integer rank_s that will represent rank of matrix S (and also rank of A).
	 *
	 * NOTE: only first rank(S) component of out vector t should be used by the caller!!! Remaining t.n_cols - rank(S)
	 * component are not determined and can have any integer value (and contain zeros by default).
	 *
	 * @retval Boolean value true for SUCCESS (solution was found), and false for FAILURE (there is no solution)
	 *
	 * @throw std::logic_error if A.n_rows == 0 OR A.n_cols == 0 OR A.n_cols != c.n_cols
	 * @see Loop Transformations for Restructuring Compilers, The Foundations (Theorem 3.6)
	 */
	bool SolveDiophantineSystem(const MatrixExt<long long>& A,
		const RowVector<long long>& c,
		MatrixExt<long long>& U,
		RowVector<long long>& t,
		uword& rank_s);

} /// namespace util
} /// namespace gap

#endif /// GAP_UTIL_DIOPHANTINE_H
