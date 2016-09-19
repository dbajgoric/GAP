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

#ifndef GAP_UTIL_COLUMN_VECTOR_H
#define GAP_UTIL_COLUMN_VECTOR_H

#include "matrix_ext.h"

namespace gap
{
namespace util
{

    template<typename T>
    class ColVector;

    template<typename T>
    ColVector<T> operator+(const ColVector<T>& x, const ColVector<T>& y);

    template<typename T>
    ColVector<T> operator-(const ColVector<T>& x, const ColVector<T>& y);

    template<typename T>
    ColVector<T> operator*(const MatrixExt<T>& X, const ColVector<T>& y);

    template<typename T>
    ColVector<T> operator*(const ColVector<T>& x, const T& scalar);

    template<typename T>
    ColVector<T> operator*(const T& scalar, const ColVector<T>& x);

    template<typename T>
    ColVector<T> operator/(const ColVector<T>& x, const ColVector<T>& y);

    template<typename T>
    ColVector<T> operator/(const ColVector<T>& x, const T& scalar);

    template<typename T>
    ColVector<T> operator/(const T& scalar, const ColVector<T>& x);

    template<typename T>
    class ColVector : public MatrixExt<T>
    {
    public:
        ColVector(const uword n_rows);
        ColVector(const std::vector<T>& x);
        ColVector(const std::initializer_list<T>& x);
        ColVector(const MatrixExt<T>& X);
        ColVector(MatrixExt<T>&& X);
        const ColVector& operator=(const MatrixExt<T>& X);
        const ColVector& operator=(const std::vector<T>& x);
        const ColVector& operator=(const std::initializer_list<T>& x);
        virtual ~ColVector()
        {
        }

        friend ColVector operator+<T>(const ColVector& x, const ColVector& y);
        friend ColVector operator-<T>(const ColVector& x, const ColVector& y);
        friend ColVector operator*<T>(const MatrixExt<T>& X, const ColVector<T>& y);
        friend ColVector operator*<T>(const ColVector& x, const T& scalar);
        friend ColVector operator*<T>(const T& scalar, const ColVector& x);
        friend ColVector operator/<T>(const ColVector& x, const ColVector& y);
        friend ColVector operator/<T>(const ColVector& x, const T& scalar);
        friend ColVector operator/<T>(const T& scalar, const ColVector& x);
    };


	/**
	 * @brief Calculates GCD of a list of integrals.
	 */
	template<typename T>
	inline T gcd(const ColVector<T>& numbers)
	{
		static_assert(std::is_integral<T>::value, "gcd(): only integral ColVector<T> is allowed");

		MatrixExt<T> U(numbers.n_rows, numbers.n_rows);
		ColVector<T> S(numbers.n_rows);

		numbers.ReduceToEchelon(U, S);
		return S(0) > 0 ? S(0) : -1 * S(0);
	}

} /// namespace util
} /// namespace gap

#endif /// GAP_UTIL_COLUMN_VECTOR_H
