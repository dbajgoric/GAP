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

#ifndef GAP_TRANSFORM_UNIMODULAR_TRANSFORMATION_H
#define GAP_TRANSFORM_UNIMODULAR_TRANSFORMATION_H

#include <gap_util/row_vector.h>
#include <gap_util/fourier_elimination.h>

namespace gap
{
namespace tran
{

	/**
	 * @brief Inner loop parallelization
	 *
	 * The goal of this method is to find unimodular matrix U that will transform loop nest L to loop nest LU, in such
	 * a way that transformation is valid (for every distance vector d > 0 in L, distance vector dU > 0 in LU) and that
	 * all distance vectors dU in LU have level 1. This means that in new loop nest LU, all inner loops LU2,...,LUm can
	 * be run in parallel. Only iterations of loop LU1 must be run sequentially.
	 *
	 * For this purpose, Hyperplane Method will be used. This method finds an integer vector (um, um-1,...,u1) that is
	 * actually a first column of matrix U, such that first component of every distance vector dU in LU is greater or
	 * equal to 1. Additionally, first non-zero component of vector (um, um-1,...,u1) vector will always be equal to 1
	 * so that GCD(um, um-1,...,u1) = 1. Each uk is chosen to be smallest nonnegative integer value that satisfies the
	 * corresponding constraint (so each uk >= 1).
	 *
	 * Matrix U is then formed by enlarging the Im-1 identity matrix in the following way: prepend the (um, um-1,...,u1)'
	 * as first column of U. Let's assume that k is the row that corresponds to uk that is first non-zero component of
	 * the mentioned vector (and uk = 1). Fill the rest of that row with zeros. This will produce matrix U such that if
	 * first column and k-th row are removed, what remains will be Im-1 identity matrix. Because of this, we know that
	 * matrix U generated this way will have det(U) = +- 1, so this is indeed unimodular matrix.
	 *
	 * @param The method expects single input argument:
	 *        distance_vectors - an array of distance vectors for loop nest L, calculated by dependence analyser. Note that
	 *        only positive distance vectors will be taken into consideration. In case of dependencies that are not loop
	 *        carried, d = 0. However, zero distances are ignored as it is impossible to find vector (um, um-1,...,u1) such
	 *        that du > 0.
	 *
	 *        It outputs mxm unimodular matrix U that defines valid L -> LU transformation.
	 *
	 * @retval Number of innermost loop that can run in parallel (always nest depth - 1).
	 * @see For much more information consult Loop Transformations for Restructuring Compilers, Loop Parallelization:
	 *      - Algorithm 3.1 (Hyperplane Method)
	 *      - Theorem 3.7
	 */
	unsigned int FindInnerLoopTransformMatrix(
		const std::vector<gap::util::RowVector<long long>>& distance_vectors
		, util::MatrixExt<long long>& U);


	/**
	 * @brief Outer loop parallelization
	 *
	 * @param
	 *        -> const std::vector<RowVector<int>>& - vector of dependence distance vectors of given loop nest
	 *        -> MatrixExt<int>& - unimodular matrix U that will transform L -> LU
	 *
	 * @reval Number of outer loops in LU that has no data dependencies and therefore can run in parallel (0 if there are
	 *        no such loops, for example when rank(D) = m).
	 * @see For much more information consult Loop Transformations for Restructuring Compilers, Loop Parallelization:
	 *      - Algorithm 3.2
	 *      - Theorem 3.8
	 */
	unsigned int FindOuterLoopTransformMatrix(
		const std::vector<gap::util::RowVector<long long>>& distance_vectors
		, util::MatrixExt<long long>& U);


	/**
	 * @brief Calculate transformed loop nest LU limits
	 *
	 * This method uses Fourier Elimination method to calculate the limits of the new loop nest LU produces by unimodular matrix
	 * U. It is assumed that matrix U gives valid transformation L -> LU (i.e. all dependencies are preserved). Loop index K of
	 * the loop nest LU is represented via U like this:
	 *
	 * K = I*U, so we have that I = K*U^-1 (U is unimodular matrix so this is always possible). On the other hand, loop limits of
	 * the nest L can be represented via lower bound vector p0, lower bound matrix P, upper bound vector q and upper bound matrix
	 * Q like this:
	 *
	 * p0 <= IP
	 *       IQ <= q0
	 *
	 * Using the fact that I = K*U^-1 we get:
	 *
	 * p0 <= K*U^-1*P
	 *       K*U^-1*Q <= q0
	 *
	 * Next, this has to be represented in the form kA <= c, where A will be mx2m matrix and c will be 1x2m row vector. After this,
	 * PerformFourierElimination() (from fourier_elimination.h) is used to calculate lower and upper bounds for K1,K2,...,Km.
	 *
	 * @param
	 *        -> const MatrixExt<int>& U - unimodular matrix that defines transformation L -> LU
	 *        -> const RowVector<int>& p0, q0 - lower and upper 1xm bound vectors for loop nest L
	 *        -> const MatrixExt<int>& P, Q - lower and upper mxm bound matrices for loop nest L
	 *        -> std::vector<Bound>& b, B - lower and upper bounds for K1,K2,...,Km of loop nest LU
	 *
	 * @note Once we choose the index variables of loop nest LU, let's assume they are K1,K2,K3, and set that index vector I of L is
	 * (I1,I2,I3) = (K1,K2,K3) * U^-1 it's critical that we set K1 to be index of loop LU1, K2 index of loop LU2 and K3 of LU3. If we
	 * instead choose that K3 will be index of LU1, K1 of LU2 and K2 of LU3 we must change the way that we defined I via K to reflect
	 * this: (I1,I2,I3) = (K3,K1,K2) * U^-1. If we continue to use previous definition of mapping I -> K, then it'll be like applying
	 * additional transformation to loop LU that will yield loop LU' - and this mustn't happen!
	 *
	 * @see Loop Transformations for Restructuring Compilers, Loop Parallelization (section 3.6) and fourier_elimination.h
	 */
	void CalculateTransformedNestLimits(
		const gap::util::MatrixExt<long long>& U,
		const gap::util::RowVector<long long>& p0,
		const gap::util::MatrixExt<long long>& P,
		const gap::util::RowVector<long long>& q0,
		const gap::util::MatrixExt<long long>& Q,
		std::vector<gap::util::Bound>& b,
		std::vector<gap::util::Bound>& B);

} /// namespace tran
} /// namespace gap

#endif /// GAP_TRANSFORM_UNIMODULAR_TRANSFORMATION_H
