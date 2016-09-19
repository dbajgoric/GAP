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

#include <utility>
#include <stdexcept>
#include "row_vector.h"
#include "rational_number.hpp"

namespace gap
{
namespace util
{

    template<typename T>
    RowVector<T>::RowVector(const uword n_cols): MatrixExt<T>(1, n_cols)
    {
    }

    template<typename T>
    RowVector<T>::RowVector(const std::vector<T>& x): MatrixExt<T>(1, x.size())
    {
        for (auto i = 0U; i < this->n_cols; ++i)
            (*this)(i) = x[i];
    }

    template<typename T>
    RowVector<T>::RowVector(const std::initializer_list<T>& x): MatrixExt<T>(1, x.size())
    {
        auto list_iter = x.begin();
        for (auto i = 0U; i < this->n_cols; ++i, ++list_iter)
            (*this)(i) = *list_iter;
    }

    template<typename T>
    RowVector<T>::RowVector(const MatrixExt<T>& X): MatrixExt<T>(X)
    {
        if (X.n_rows != 1)
            throw std::logic_error("RowVector(const MatrixExt<T>& X): X.n_rows must be equal to 1");
    }

    template<typename T>
    RowVector<T>::RowVector(MatrixExt<T>&& X): MatrixExt<T>(std::move(X))
    {
        if (X.n_rows != 1)
            throw std::logic_error("RowVector(const MatrixExt<T>& X): X.n_rows must be equal to 1");
    }

    template<typename T>
    const RowVector<T>& RowVector<T>::operator=(const MatrixExt<T>& X)
    {
        if (X.n_rows != 1)
            throw std::logic_error("RowVector(const MatrixExt<T>& X): X.n_rows must be equal to 1");

        MatrixExt<T>::operator=(X);
        return *this;
    }

    template<typename T>
    const RowVector<T>& RowVector<T>::operator=(const std::vector<T>& x)
    {
		if (this->n_cols != x.size())
		{
			*this = RowVector<T>(x);
		}
		else
		{
			auto list_iter = x.begin();
			for (auto i = 0U; i < this->n_cols; ++i, ++list_iter)
				(*this)(i) = *list_iter;
		}

		return *this;
    }

    template<typename T>
    const RowVector<T>& RowVector<T>::operator=(const std::initializer_list<T>& x)
    {
		if (this->n_cols != x.size())
		{
			*this = RowVector<T>(x);
		}
		else
		{
			auto list_iter = x.begin();
			for (auto i = 0U; i < this->n_cols; ++i, ++list_iter)
				(*this)(i) = *list_iter;
		}

        return *this;
    }

    template<typename T>
    RowVector<T> operator+(const RowVector<T>& x, const RowVector<T>& y)
    {
        return RowVector<T>(dynamic_cast<const MatrixExt<T>& >(x) + dynamic_cast<const MatrixExt<T>& >(y));
    }

    template<typename T>
    RowVector<T> operator-(const RowVector<T>& x, const RowVector<T>& y)
    {
        return RowVector<T>(dynamic_cast<const MatrixExt<T>& >(x) - dynamic_cast<const MatrixExt<T>& >(y));
    }

    template<typename T>
    RowVector<T> operator*(const RowVector<T>& x, const MatrixExt<T>& Y)
    {
        return RowVector<T>(dynamic_cast<const MatrixExt<T>& >(x) * Y);
    }

    template<typename T>
    T operator*(const RowVector<T>& x, const ColVector<T>& y)
    {
        return RowVector<T>(dynamic_cast<const MatrixExt<T>& >(x) * dynamic_cast<const MatrixExt<T>& >(y))(0);
    }

    template<typename T>
    MatrixExt<T> operator*(const ColVector<T>& x, const RowVector<T>& y)
    {
        return RowVector<T>(dynamic_cast<const MatrixExt<T>& >(x) * dynamic_cast<const MatrixExt<T>& >(y));
    }

    template<typename T>
    RowVector<T> operator*(const RowVector<T>& x, const T& scalar)
    {
        return RowVector<T>(dynamic_cast<const MatrixExt<T>& >(x) * scalar);
    }

    template<typename T>
    RowVector<T> operator*(const T& scalar, const RowVector<T>& x)
    {
        return RowVector<T>(scalar * dynamic_cast<const MatrixExt<T>& >(x));
    }

    template<typename T>
    RowVector<T> operator/(const RowVector<T>& x, const RowVector<T>& y)
    {
        return RowVector<T>(dynamic_cast<const MatrixExt<T>& >(x) / dynamic_cast<const MatrixExt<T>& >(y));
    }

    template<typename T>
    RowVector<T> operator/(const RowVector<T>& x, const T& scalar)
    {
        return RowVector<T>(dynamic_cast<const MatrixExt<T>& >(x) / scalar);
    }

    template<typename T>
    RowVector<T> operator/(const T& scalar, const RowVector<T>& x)
    {
        return RowVector<T>(scalar / dynamic_cast<const MatrixExt<T>& >(x));
    }

    template<typename T>
    typename RowVector<T>::ComparisonResult RowVector<T>::CompareLexicographically(const RowVector& x, const RowVector& y, uword& level)
    {
        if (x.n_cols != y.n_cols)
            throw std::logic_error("RowVector::CompareLexicographically(): vectors must have the same number of cols");

        RowVector<T> d(x - y);
        level = d.GetLevel();
        if (level == x.n_cols)
        {
            /** Vector d = y - x is zero vector, so x == y. */
            return Equal;
        }

        return d(level) > T(0) ? LeftGreater : RightGreater;
    }

    template<typename T>
    uword RowVector<T>::GetLevel() const
    {
        for (auto i = 0U; i < this->n_cols; ++i)
        {
            if ((*this)(i) != static_cast<T>(0))
                return i;
        }

        /** This vector is a zero vector. */
        return this->n_cols;
    }


    template class RowVector<int>;
    template RowVector<int> operator+<int>(const RowVector<int>& x, const RowVector<int>& y);
    template RowVector<int> operator-<int>(const RowVector<int>& x, const RowVector<int>& y);
    template RowVector<int> operator*<int>(const RowVector<int>& x, const MatrixExt<int>& Y);
    template int operator*<int>(const RowVector<int>& x, const ColVector<int>& y);
    template MatrixExt<int> operator*<int>(const ColVector<int>& x, const RowVector<int>& y);
    template RowVector<int> operator*<int>(const RowVector<int>& x, const int& scalar);
    template RowVector<int> operator*<int>(const int& scalar, const RowVector<int>& x);
    template RowVector<int> operator/<int>(const RowVector<int>& x, const RowVector<int>& y);
    template RowVector<int> operator/<int>(const RowVector<int>& x, const int& scalar);
    template RowVector<int> operator/<int>(const int& scalar, const RowVector<int>& x);

	template class RowVector<long long>;
	template RowVector<long long> operator+<long long>(const RowVector<long long>& x, const RowVector<long long>& y);
	template RowVector<long long> operator-<long long>(const RowVector<long long>& x, const RowVector<long long>& y);
	template RowVector<long long> operator*<long long>(const RowVector<long long>& x, const MatrixExt<long long>& Y);
	template long long operator*<long long>(const RowVector<long long>& x, const ColVector<long long>& y);
	template MatrixExt<long long> operator*<long long>(const ColVector<long long>& x, const RowVector<long long>& y);
	template RowVector<long long> operator*<long long>(const RowVector<long long>& x, const long long& scalar);
	template RowVector<long long> operator*<long long>(const long long& scalar, const RowVector<long long>& x);
	template RowVector<long long> operator/<long long>(const RowVector<long long>& x, const RowVector<long long>& y);
	template RowVector<long long> operator/<long long>(const RowVector<long long>& x, const long long& scalar);
	template RowVector<long long> operator/<long long>(const long long& scalar, const RowVector<long long>& x);

    /*template class RowVector<double>;
    template RowVector<double> operator+<double>(const RowVector<double>& x, const RowVector<double>& y);
    template RowVector<double> operator-<double>(const RowVector<double>& x, const RowVector<double>& y);
    template RowVector<double> operator*<double>(const RowVector<double>& x, const MatrixExt<double>& Y);
    template double operator*<double>(const RowVector<double>& x, const ColVector<double>& y);
    template MatrixExt<double> operator*<double>(const ColVector<double>& x, const RowVector<double>& y);
    template RowVector<double> operator*<double>(const RowVector<double>& x, const double& scalar);
    template RowVector<double> operator*<double>(const double& scalar, const RowVector<double>& x);
    template RowVector<double> operator/<double>(const RowVector<double>& x, const RowVector<double>& y);
    template RowVector<double> operator/<double>(const RowVector<double>& x, const double& scalar);
    template RowVector<double> operator/<double>(const double& scalar, const RowVector<double>& x);*/

	template class RowVector<Rational<long long>>;
	template RowVector<Rational<long long>> operator+<Rational<long long>>(const RowVector<Rational<long long>>& x, const RowVector<Rational<long long>>& y);
	template RowVector<Rational<long long>> operator-<Rational<long long>>(const RowVector<Rational<long long>>& x, const RowVector<Rational<long long>>& y);
	template RowVector<Rational<long long>> operator*<Rational<long long>>(const RowVector<Rational<long long>>& x, const MatrixExt<Rational<long long>>& Y);
	template Rational<long long> operator*<Rational<long long>>(const RowVector<Rational<long long>>& x, const ColVector<Rational<long long>>& y);
	template MatrixExt<Rational<long long>> operator*<Rational<long long>>(const ColVector<Rational<long long>>& x, const RowVector<Rational<long long>>& y);
	template RowVector<Rational<long long>> operator*<Rational<long long>>(const RowVector<Rational<long long>>& x, const Rational<long long>& scalar);
	template RowVector<Rational<long long>> operator*<Rational<long long>>(const Rational<long long>& scalar, const RowVector<Rational<long long>>& x);
	template RowVector<Rational<long long>> operator/<Rational<long long>>(const RowVector<Rational<long long>>& x, const RowVector<Rational<long long>>& y);
	template RowVector<Rational<long long>> operator/<Rational<long long>>(const RowVector<Rational<long long>>& x, const Rational<long long>& scalar);
	template RowVector<Rational<long long>> operator/<Rational<long long>>(const Rational<long long>& scalar, const RowVector<Rational<long long>>& x);

} /// namespace util
} /// namespace gap
