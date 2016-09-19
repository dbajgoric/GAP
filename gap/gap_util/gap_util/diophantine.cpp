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

#include "diophantine.h"

namespace gap
{
namespace util
{

    bool SolveDiophantineEquation(const ColVector<long long>& A,
		const long long c,
		MatrixExt<long long>& U,
		long long& t1)
    {
        if (A.n_rows == 0)
            throw std::logic_error("SolveDiophantineEquation(): coefficient vector A must have at least one row");

        /** Reduce coefficient vector A to echelon form. */
        ColVector<long long> S(A.n_rows);
        A.ReduceToEchelon(U, S);

        /** If c % S(0) !=0 (absolute value |s11|=S(0) is GCD(a1,...,am)), equation has no solution. */
        if (c % S(0) != 0)
            return false;

        /** Solution exists and it is identified by integer c / S(0) and matrix U. */
        t1 = c / S(0);
        return true;
    }

    bool SolveDiophantineSystem(const MatrixExt<long long>& A, const RowVector<long long>& c, MatrixExt<long long>& U, RowVector<long long>& t, uword& rank_s)
    {
        if (A.n_rows == 0 || A.n_cols == 0)
            throw std::logic_error("SolveDiophantineSystem(): coefficient matrix A must have at least one row and one column");

        if (A.n_cols != c.n_cols)
            throw std::logic_error("SolveDiophantineSystem(): number of cols of matrix A and vector c must be equal");

        /** Reduce coefficient matrix A to echelon form. */
        MatrixExt<long long> S(A.n_rows, A.n_cols);
        rank_s = A.ReduceToEchelon(U, S);

        /**
         * Check if solution exists by solving simplified system tS = c. First S.rank components of t are determined, while other can
         * be chosen arbitrarily (by the caller). As S is echelon matrix, it is easy to calculate t1 and then move to the right and
         * calculate other determined component using previously computed ones.
         */
        t = RowVector<long long>(S.n_rows);
        uword t_component(0);

        for (auto i = 0U; i < S.n_cols; ++i)
        {
            long long sum(0);
            for (auto j = 0U; j < t_component; ++j)
                sum += S(j,i) * t(j);

            if (t_component >= rank_s || S(t_component,i) == 0)
            {
                /** One of the equations doesn't hold when components of t calculated previously are put into it. */
                if (sum != c(i))
                    return false;

                continue;
            }

            long long difference(c(i) - sum);
            /** Current component of vector t has no integer solution. */
            if (difference % S(t_component,i) != 0)
                return false;

            /** Current component of t has integer solution, compute it. */
            t(t_component) = difference / S(t_component,i);
            ++t_component;
        }

        return true;
    }

} /// namespace util
} /// namespace gap
