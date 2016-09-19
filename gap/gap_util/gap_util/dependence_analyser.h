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

#ifndef GAP_DEPENDENCE_ANALYSER_H
#define GAP_DEPENDENCE_ANALYSER_H

#include <vector>
#include "dependence_info.h"

namespace gap
{
namespace dep
{

/**
 * The following method performs the general linear dependence analysis on the pair of variables of two assignment statements in
 * the perfect loop nest that has the following form:
 *
 * DO I1 = p1, q1
 *   DO I2 = p2(I1), q2(I1)
 *     DO I3 = p3(I1, I2), q3(I1, I2)
 *            -
 *            -
 *            -
 *            DO Im = pm(I1,...,Im-1), qm(I1,...,Im-1)
 *              H(I1,I2,...,Im)
 *			  ENDDO
 *            -
 *            -
 *            -
 *     ENDDO
 *   ENDDO
 * ENDDO
 *
 * The body H(I) consists of finite number of assignment statements and it usually looks similar to the following code sample:
 *
 * S:    X(a11*I1 + ... + am1*Im + a10, ..., a1n*I1 + ... + amn*Im + an0) = ...
 * T:                                                                 ... = X(b11*I1 + ... + bm1*Im + b10, ..., b1n*I1 + ... + bmn*Im + bn0)
 *
 * Statement can have single output variable (for example element of an array) and many input variables (for example expression combining
 * several elements of an array like X(2I) - X(I+1) + X(2I - 4)). This algorithm works on the level of 2 variables belonging to two not
 * necessary distinct assignment statements. It is up to the caller to choose proper variables and feed them to the analyser. Caller also
 * has to specify if variables belong to distinct statements or they are part of the same statement.
 *
 * IMPORTANT NOTE: method assumes that S < T or S = T, or in other words statement S comes before statement T in the program or they represent
 * the same statement (which has to be indicated by the caller). The caller can always invoke the method in such way that this assumption is
 * satisfied.
 *
 * 1) Two variables can be written in the matrix form as X(IA + a0) and X(IB + b0) where I=(I1,...,Im), A=(aij)mn, a0=(a10,...,an0), B=(bij)mn
 *    b0=(b10,...,bn0). These two variables will represent the same memory location IFF iA + a0 = jB + b0 => iA - jB = b0 - a0 where
 *    variables i and j are instances of loop index vector I (so i and j are vectors). This can be written in the following way:
 *
 *            _  _
 *           |  A |
 *    (i;j)* |    | = b0 - a0
 *           | -B |
 *           --  --
 *
 * 2) This is a system of diophantine equations that will be solved by calling SolveDiophantineSystem() method declared in diophantine.h
 *    header.
 *
 *    2.1) SolveDiophantineSystem() returns FALSE - there is no integer solution to the system which means that two variables can never
 *         represent the same memory location and therefore CANNOT cause dependence between statements S and T.
 *    2.2) SolveDiophantineSystem() returns TRUE - there is integer solution to the system which memans that it is POSSIBLE that two
 *         variables represent the same memory location and cause dependence between S and T. This is required condition for existance
 *         of dependence but not sufficient one as we have to check the loop nest boundaries as well.
 *
 * 3) SolveDiophantineSystem() method produced solution vector t (2m cols) that has rank_s determined components and 2m x 2m unimodular matrix
 *    U. These two together are solution of the system as (i;j) = tU. However, tU will produce definite solution only if every component of t is
 *    determined or if rank_s == t.n_cols which is generally not true. Note that first m cols of matrix U determine vector i and following m cols
 *    determine vector j. Form matrices U1 and U2 in the way U = (U1 | U2).
 *
 *    Separate solution vectors can be calculated in the following way:
 *
 *    i = tU1
 *    j = tU2
 *
 * 4) If rank_s < t.n_cols go to 5. Here we handle special case where every component of t is determined. The only thing left to check is
 *    if solution (i;j) satisfies the system of inequalities imposed by loop nest limits:
 *
 *    p0 <= IP
 *          IQ <= q0 (P and Q are mxm matrices and p0 and q0 are 1xm vectors)
 *
 *    SIDE NOTE: it is assumed that each loop index Ik will have single linear function as lower boundary and single linear function as upper
 *    boundary. Therefore, boundaries such as MAX(kI + k0, lI + l0,...) with multiple linear functions are currently not supported. These
 *    can easily be allowed for by extending the matrices P and Q so that they will have m or more columns corresponding to these additional
 *    bound functions.
 *
 *    As vectors i and j are just instances of loop index vector I, they both have to satisfy the above system. Form the system of inequalities
 *    for both of these vectors (so called dependence constraints):
 *
 *    p0 <= (tU1)P
 *          (tU1)Q <= q0
 *    p0 <= (tU2)P
 *          (tU2)Q <= q0
 *
 *    Transform this to a system IA <= c as required by the PerformFourierElimination() method declared in fourier_elimination.h header. As this
 *    is the special case where we have no variables in the system of inequalities, A will have one row filled with as many zeros as there are
 *    trivial inequalities to verify.
 *
 *    If PerformFourierElimination() returns FALSE then these two variables cannot represent the same memory location within the boundaries of
 *    the loop nest! This means that they cannot cause dependence between statements S and T. Return FALSE.
 *
 *    If PerformFourierElimination() returned TRUE go to step 6.
 *
 *    5) Here we handle the case where rank_s < t.n_cols, which means that we have t.n_cols - rank_s undetermined components of t. Like in step
 *       4, first half of matrix U (vertically looking) determines unknown vector i while the other half determines unknown vector j. We must
 *       form a system of inequalities featuring mentioned undetermined components of t.
 *
 *		 p0 <= (tU1)P
 *             (tU1)Q <= q0
 *       p0 <= (tU2)P
 *             (tU2)Q <= q0
 *
 *       Calculate the following (2m x m) matrices:
 *
 *       I_P = U1 * P
 *       I_Q = U1 * Q
 *       J_P = U2 * P
 *       J_Q = U2 * Q
 *
 *       Form the system of 4m inequalities (2m for i and 2m for j). Left side of this system is formed by taking elements of previously calculated
 *       matrices that correspond to undetermined components of t. In the end system will be in IA <= c form required by the PerformFourierElimination()
 *       method.
 *
 *       5.1) If PerformFourierElimination() returns FALSE it is impossible to find vectors i and j such that variables X(iA + a0) and X(jB + b0) represent
 *            the same memory location within loop nest limits (this is why GCD test is required but not sufficient condition for the existance of dependence
 *            between two variables. If loop nest would cover entire Z^m space then GCD would be sufficient to determined dependence existance). Return FALSE
 *            and terminate the algorithm.
 *
 *       5.2) When PerformFourierElimination() returns TRUE this only means there is a REAL solution to the given system of inequalities. Nothing is said
 *            about the integer solution and in order to check if integer solution exists one has to search manually through the solution set. In order to
 *            facilitate this process fourier_elimination.h header provides EnumerateIntegerSolutions() method that can be used to find all integer solutions
 *            of the system based on the set of lower and upper boundaries returned by PerformFourierElimination() method.
 *
 *            Note that because loop nest always has finite lower and upper boundary, in case when there is a solution to the system of inequalities the
 *            number of solutions will always be finite as well. This is important as PerformFourierElimination() throws an exception when at least one of
 *            provided lower or upper bounds is not finite (impossible to enumerate all solutions as there is infinite number of them).
 *
 *            If EnumerateIntegerSolutions() returns FALSE it means there is no integer solution, so it is still impossible to find (i,j) such that variables
 *            represent the same memory location. Return FALSE and terminate the algorithm.
 *
 *            Otherwise, at least one integer solution (i,j) exists (fully determined without any unknown parts). Proceed to step 6.
 *
 *    6) Steps 4 and 5 already generated all pairs (i,j) for which variables X(iA + a0) and X(iB + b0) represent the same memory location at certain iterations
 *       of the loop nest. In this step we have to assign each of these pairs to one of two sets: set that represents the dependence of T on S, and the set that
 *       represents the dependence of S on T.
 *
 *       Assign to the set that identifies dependence of T on S the following pairs:
 *		 {(i,j) : i < j}
 *       Assign to the set that identifies dependence of S on T the follwing pairs:
 *       {(j,i) : j < i}
 *
 *       NOTE: this < and > are lexicographical comparison operators and not 'normal' smaller and greater operators! By definition of dependence, in order for
 *       statement instance T(j) to be dependent on S(i), instance S(i) has to come before instance T(j) in the original sequential program.
 *
 *       6.1) If S < T (S and T are distinct statements), additionally assign to the set that represents dependence of T on S the following pairs:
 *            {(i,j) : i = j}
 *
 *            If variable X(iA + a0) of S and variable X(jB + b0) of T represent the same memory location when i = j (basically in the same iteration all the
 *            way up to the inner-most loop), T is dependent on S as statement S comes before statement T in the program. This is not loop carried dependence,
 *            and it can be easily identified by zero distance vector and level equal to m + 1 (where m is number of loops in the nest).
 *
 *       6.2) S = T. Nothing to be done here as statement instance S(i) cannot depend on itself by the definition of dependence (note 'instance' in the previous
 *            sentence. Statement can very well depend on itself when it represents the different instances).
 *
 *    7) Calculate dependence distance vectors, direction vectors and levels.
 *
 *       7.1) For every pair (i,j) in the set that represents dependence of T on S calculate the distance, direction vector and level as follows:
 *            d = j - i (distance vector must always be lexicographically non-negative)
 *            sigma = sig(d)
 *            l = lev(d)
 *
 *      7.2) Do the same for every pair (i,j) in the set that represents dependence of S on T:
 *            d = i - j
 *            sigma = sig(d)
 *            l = lev(d)
 *
 *      Return TRUE if dependece of S on T, or dependence of T on S or both or not empty. Otherwise return FALSE.
 *
 * @param The method accepts following input arguments:
 *        A, a0 - coefficient matrix A and vector of constants a0 from the variable X(iA + a0) of S
 *        B, b0 - coefficient matrix B and vector of constants b0 from the variable X(jB + b0) of T
 *        P, p0 - lower boundary matrix P and lower boundary vector of constants p0
 *        Q, q0 - upper boundary matrix Q and upper boundary vector of constants q0
 *
 *        It sets the following output parameters:
 *        T_on_S - object containing vector i, j, d = j - i, sig = signum(d), l = lev(d)
 *        S_on_T - object containing vector i, j, d = i - j, sig = signum(d), l = lev(d)
 *
 * @retval TRUE to indicate that variables X(iA + a0) and X(jB + b0) cause dependence between S and T, FALSE otherwise.
 *
 * @note This method performs so-called general linear dependence testing. It doesn't discover certain special cases that can simplify the
 * dependence analysis such as regular and rectangular loop nests, or uniform dependence distances when A = B. This makes this method much
 * slower than the other available dependence analysis procedures and it should be used as last resort when none of the special-purpose
 * methods can be applied.
 *
 * @see Loop Transformations for Restructuring Compilers, The Foundations (Algorithm 5.1)
 */

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
	bool are_distinct = true);


/**
 * This is specialized algorithm for dependence testing that can be used for regular and rectangular loop nests, when matrix A = B which
 * leads to uniform dependence distances! Currently, for all other cases general linear dependence algorithm has to be deployed. Loop nest
 * is regular when P = Q, and rectangular when P = Q = Identity matrix (in rectangular loops nest lower/upper boundaries for every loop variable
 * Ik are constants. Obviously, every rectangular matrix is regular as well but not the other way around.
 *
 * Like in the general algorithm, caller has to specify if statements S and T are distinct. It is assumed that either S < T or S = T. Variables
 * to be analysed are X(iA + a0) of S and X(jA + b0) of T. Steps of the algorithm are as follows:
 *
 * 1) Mentioned variables will cause dependence between S and T IFF:
 *
 *    iA + a0 = jA + b0                 =>
 *    jA - iA = a0 - b0                 =>
 *    (j - i)A = c0 where c0 = a0 - b0  =>
 *    kA = c0.
 *
 *    So instead of solving this diophantine system for two unknowns i and j, we will solve it for uknown k = j - i. Please note the following
 *    crucial fact:
 *
 *    NOTE: if we find out that statement T depends on statement S, i <= j will be satisfied and in this case value k = j - i >= 0 (these are
 *    lexicographical comparison operations). Therefore, k is actually the distance vector of dependence of T on S. If we find out that statement
 *    S is dependent on statement T, then i > j (as S < T or S = T, when i = j then we can only have dependence of T on S) and calculated vector
 *    k < 0! So, in this case distance vector of dependece of S on T is d = -1 * k!
 *
 *    Run the SolveDiophantineSystem() solver on kA = c0 system. If this method returns FALSE this means that there is NO integer vector k such
 *    that kA = c0 or (j - i)A = c0. In other words, it is impossible to find two vectors i and j such that system (j - i)A = c0 will be satisfied
 *    therefore variables X(iA + a0) and X(jA + b0) can never represent the same memory location. These variables cannot cause dependence between
 *    statements S and T. Return FALSE.
 *
 * 2) System kA = c0 has at least one integer solution k = tU where t is (1 x m) vector and U is (m x m) unimodular matrix. This means that GCD
 *    test is positive, which is required but not sufficient condition for existence of dependence between two variables. We have to verify if
 *    integer solution k exists within the loop nest boundaries. As P = Q we have the following system of inequalities:
 *
 *    p0 <= iP <= q0
 *    p0 <= jP <= q0
 *
 *    Using the fact that k = j - i => j = i + k we get:
 *
 *    p0 <= iP <= q0
 *    p0 <= iP + kP <= q0
 *    -------------------
 *    p0 <= iP <= q0
 *    p0 - kP <= iP <= q0 - kP
 *    
 *    Applying the Fourier Elimination method we get that:
 *
 *    max(p0, p0 - kP) <= iP <= min(q0, q0 - kP)				(*)
 *
 *    p0 <= q0 (true by definition of p0 and q0 vectors)
 *    p0 <= q0 - kP -> kP <= q0 - p0
 *    p0 - kP <= q0 -> p0 - q0 <= kP
 *    p0 - kP <= q0 - kP -> p0 <= q0 (true)
 *    -------------------------------------
 *    p0 - q0 <= kP <= q0 - p0									(**)
 *
 *    2.1) Echelon matrix S rank rank_s is equal to m, which means that every component of vector t is determined. In this case
 *         k = tU is (1 x m) vector of constants. Using PerformFourierElimination() check if system (from (**))
 *
 *         0 <= tUP + q0 - p0
 *         0 <= -tUP + q0 - p0
 *
 *         is trivially satisfied. If yes, goto step 3. If no, then there is no integer vector k = j - i such that both j and i are
 *         within loop bounds. Return FALSE and terminate the algorithm.
 *
 *    2.2) rank_s < m which means that there are m - rank_s undetermined components of vector t. Starting from (**) we form a system
 *         of 2*m inequalities with m - rank_s variables and solve this with PerformFourierElimination. If this system has no REAL
 *         solution then there is REAL vector k such that both i and j are within loop bounds. Return FALSE and terminate
 *         the algorithm.
 *
 *         Otherwise, use EnumerateIntegerSolutions() method to generate all possible integer solutions of vector k, for which kA = c0.
 *         If this method returns false, it means that there are no integer solutions to this system of inequalities. This means that
 *         within loop nest bounds, we cannot find vector k such that variables X(iA + a0) and X(jA + b0) represent the same memory
 *         location. Return FALSE and terminate the algorithm.
 *
 * 3) Calculate the dependence information for statement S and T.
 *    3.1) m == rank_s -> means every component of vector t is determined so there is only one solution vector k = tU. We have
 *         following three cases:
 *
 *         -> k > 0 THEN j > i which means T depends on S with distance vector d = k (remember that k = j - i)
 *         -> k < 0 THEN i > j which means S depends on T with distance vector d = -1 * k
 *         -> k = 0 THEN i = j. If S < T then T depends on S with d = 0. If S = T this case doesn't lead to dependence as statement
 *            instance S(i) cannot depend on itself.
 *
 *         Calculate direction vector sig(d) and level of dependence lev(d).
 *
 *    3.2) rank_s < m -> means there are m - rank_s undetermined components of vector t. Step 2.2 produced all possible integer vectors
 *         of form (tk, tk+1,..., tm) where k = rank_s + 1 (m - (rank_s + 1) + 1 = m - rank_s). For every such vector, form fully determined
 *         vector t by replacing m - rank_s undetermined components with the mentioned (1 x m - rank_s) vector and calculate vector k = tU!
 *
 *         Then follow exact same logic as in step 3.1 to decide if k identify dependence of T on S, or S on T. This step is repeated for every
 *         vector t formed in the described way. For every distance vector d calculate direction vector sig(d) and level of dependence lev(d).
 *
 *    Return TRUE if dependence T on S, or dependence of S on T (or both) is not empty. Otherwise return FALSE.
 *
 *    NOTE: unlike general linear dependence test, this algorithm doesn't calculate loop index instances (i,j) for each dependence. The reason
 *    for this is that here, dependence distances are uniform! This means that for distance vector d every statement T(i + d) depends on statement
 *    S(i) as long as both i and i + d = j are both within loop bounds! (or if S depends on T, for vector d every statement S(j + d) depends on
 *    statement T(j) as long as both j and j + d = i are within loop bounds). Having uniform distance vector generally provides all the neccessary
 *    information the caller needs to decide type of loop transformation that will be applied.
 *
 *
 * @param The method accepts following input arguments:
 *        A, a0 - coefficient matrix A and vector of constants a0 from the variable X(iA + a0) of S
 *        b0 - vector of constants b0 from the variable X(jA + b0) of T
 *        P, p0 - lower and upper (P = Q) boundary matrix P and lower boundary vector of constants p0
 *        q0 - upper boundary vector of constants q0
 *
 *        It sets the following output parameters:
 *        T_on_S - object containing vector d, sig = signum(d), l = lev(d) (both i and j are 0x0 vectors)
 *        S_on_T - object containing vector d, sig = signum(d), l = lev(d) (both i and j are 0x0 vectors)
 *
 * @retval TRUE to indicate that variables X(iA + a0) and X(jA + b0) cause dependence between S and T, FALSE otherwise.
 */
bool UniformLinearDependenceTest(
	const util::MatrixExt<long long>& A,
	const util::RowVector<long long>& a0,
	const util::RowVector<long long>& b0,
	const util::MatrixExt<long long>& P,
	const util::RowVector<long long>& p0,
	const util::RowVector<long long>& q0,
	std::vector<DependenceInfo>& T_on_S,
	std::vector<DependenceInfo>& S_on_T,
	bool are_distinct = true);

} /// namespace dep
} /// namespace gap

#endif /// GAP_DEPENDENCE_ANALYSER_H
