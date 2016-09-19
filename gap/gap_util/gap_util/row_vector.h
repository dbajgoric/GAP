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

#ifndef GAP_UTIL_ROW_VECTOR_H
#define GAP_UTIL_ROW_VECTOR_H

#include "matrix_ext.h"
#include "column_vector.h"
#include <vector>
#include <initializer_list>

namespace gap
{
namespace util
{

    template<typename T>
    class RowVector;

    template<typename T>
    RowVector<T> operator+(const RowVector<T>& x, const RowVector<T>& y);

    template<typename T>
    RowVector<T> operator-(const RowVector<T>& x, const RowVector<T>& y);

    template<typename T>
    RowVector<T> operator*(const RowVector<T>& x, const MatrixExt<T>& Y);

    template<typename T>
    T operator*(const RowVector<T>& x, const ColVector<T>& Y);

    template<typename T>
    MatrixExt<T> operator*(const ColVector<T>& x, const RowVector<T>& Y);

    template<typename T>
    RowVector<T> operator*(const RowVector<T>& x, const T& scalar);

    template<typename T>
    RowVector<T> operator*(const T& scalar, const RowVector<T>& x);

    template<typename T>
    RowVector<T> operator/(const RowVector<T>& x, const RowVector<T>& y);

    template<typename T>
    RowVector<T> operator/(const RowVector<T>& x, const T& scalar);

    template<typename T>
    RowVector<T> operator/(const T& scalar, const RowVector<T>& x);


    template<typename T>
    class RowVector : public MatrixExt<T>
    {
    private:
        using MatrixExt<T>::GetRowLevel;

    public:
        enum ComparisonResult
        {
            LeftGreater = 0
            , RightGreater
            , Equal
        };

        RowVector(const uword n_cols);
        RowVector(const std::vector<T>& x);
        RowVector(const std::initializer_list<T>& x);
        RowVector(const MatrixExt<T>& X);
        RowVector(MatrixExt<T>&& X);
        const RowVector& operator=(const MatrixExt<T>& X);
        const RowVector& operator=(const std::vector<T>& x);
        const RowVector& operator=(const std::initializer_list<T>& x);
        virtual ~RowVector()
        {
        }

        friend RowVector operator+<T>(const RowVector& x, const RowVector& y);
        friend RowVector operator-<T>(const RowVector& x, const RowVector& y);
        friend RowVector operator*<T>(const RowVector& x, const MatrixExt<T>& Y);
        friend T operator*<T>(const RowVector<T>& x, const ColVector<T>& y);
        friend MatrixExt<T> operator*<T>(const ColVector<T>& x, const RowVector<T>& y);
        friend RowVector operator*<T>(const RowVector& x, const T& scalar);
        friend RowVector operator*<T>(const T& scalar, const RowVector& x);
        friend RowVector operator/<T>(const RowVector& x, const RowVector& y);
        friend RowVector operator/<T>(const RowVector& x, const T& scalar);
        friend RowVector operator/<T>(const T& scalar, const RowVector& x);

        /**
         * @brief The following method performs 'lexicographical' comparison of row vectors. Vector a is lexicographically
         * smaller than vector b IFF lead_element(a) < lead_element(b). Vector a is lexicographically positive if its
         * leading element is positive. Vector a is lexicographically negative if its leading element is negative.
         * Vector a is lexicographically greater than vector b IFF lead_element(a) > lead_element(b).
         *
         * This method returns a ComparisonResult value telling which vector is greater (left, right or equal). Additionally,
         * it returns the level at which one vector is greater than the other. Following cases are possible:
         *
         * 1) If vectors x and y are equal, n_cols is returned through output variable and Equal as a return value.
         * 2) If vector x is lexicographically greater than y, level at which x is greater than y is returned through
         *    the output variable and LeftGreater as a return value. For instance, vector x=[0,3,-6] is greater than
         *    vector y=[0,1,5] at level 1.
         * 3) If vector x is lexicographically smaller than y, level at which y is greater than x is returned through
         *    output variable and RightGreater as a return value. For instance, for vectors x=[0,-1,10] and y=[0,0,2],
         *    level of -1 will be returned.
         *
         * @throw std::logic_error when number of cols is not the same in both vectors
         */
         static ComparisonResult CompareLexicographically(const RowVector& x, const RowVector& y, uword& level);
		 static ComparisonResult CompareLexicographically(const RowVector& x, const RowVector& y)
		 {
			 uword tmp_lev(0);
			 return CompareLexicographically(x, y, tmp_lev);
		 }

         /**
         * @brief Finds level of this row vector that is an index of the first non-zero (leading) element. The
         * level is unsigned integer that belongs to the range [0, n_cols]. If this vector is zero vector,
         * level is equal to n_cols (as by definition).
         */
        uword GetLevel() const;
    };

} /// namespace util
} /// namespace gap

#endif // GAP_UTIL_ROW_VECTOR_H

