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

#include "column_vector.h"
#include <utility>
#include <stdexcept>
#include "rational_number.hpp"

namespace gap
{
namespace util
{

	template<typename T>
	ColVector<T>::ColVector(const uword n_rows) : MatrixExt<T>(n_rows, 1)
	{
	}

	template<typename T>
	ColVector<T>::ColVector(const std::vector<T>& x) : MatrixExt<T>(x.size(), 1)
	{
		for (auto i = 0U; i < this->n_rows; ++i)
			(*this)(i) = x[i];
	}

	template<typename T>
	ColVector<T>::ColVector(const std::initializer_list<T>& x) : MatrixExt<T>(x.size(), 1)
	{
		auto list_iter = x.begin();
		for (auto i = 0U; i < this->n_rows; ++i, ++list_iter)
			(*this)(i) = *list_iter;
	}

	template<typename T>
	ColVector<T>::ColVector(const MatrixExt<T>& X) : MatrixExt<T>(X.n_rows, 1)
	{
		if (X.n_cols != 1)
			throw std::logic_error("ColVector(const MatrixExt<T>& X): X.n_cols must be equal to 1");

		uhword bla = this->vec_state;
	}

	template<typename T>
	ColVector<T>::ColVector(MatrixExt<T>&& X) : MatrixExt<T>(std::move(X))
	{
		if (X.n_cols != 1)
			throw std::logic_error("ColVector(const MatrixExt<T>& X): X.n_cols must be equal to 1");
	}

	template<typename T>
	const ColVector<T>& ColVector<T>::operator=(const MatrixExt<T>& X)
	{
		if (X.n_cols != 1)
			throw std::logic_error("ColVector(const MatrixExt<T>& X): X.n_cols must be equal to 1");

		MatrixExt<T>::operator=(X);
		return *this;
	}

	template<typename T>
	const ColVector<T>& ColVector<T>::operator=(const std::vector<T>& x)
	{
		if (this->n_rows != x.size())
			throw std::logic_error("ColVector::operator=(): size of the argument std::vector different than number of rows of this instance");

		for (auto i = 0U; i < this->n_rows; ++i)
			(*this)(i) = x[i];

		return *this;
	}

	template<typename T>
	const ColVector<T>& ColVector<T>::operator=(const std::initializer_list<T>& x)
	{
		if (this->n_rows != x.size())
			throw std::logic_error("ColVector::operator=(): size of the argument std::vector different than number of rows of this instance");

		auto list_iter = x.begin();
		for (auto i = 0U; i < this->n_rows; ++i, ++list_iter)
			(*this)(i) = *list_iter;

		return *this;
	}

	template<typename T>
	ColVector<T> operator+(const ColVector<T>& x, const ColVector<T>& y)
	{
		return ColVector<T>(dynamic_cast<const MatrixExt<T>&>(x) + dynamic_cast<const MatrixExt<T>&>(y));
	}

	template<typename T>
	ColVector<T> operator-(const ColVector<T>& x, const ColVector<T>& y)
	{
		return ColVector<T>(dynamic_cast<const MatrixExt<T>&>(x) - dynamic_cast<const MatrixExt<T>&>(y));
	}

	template<typename T>
	ColVector<T> operator*(const MatrixExt<T>& X, const ColVector<T>& y)
	{
		return ColVector<T>(X * dynamic_cast<const MatrixExt<T>&>(y));
	}

	template<typename T>
	ColVector<T> operator*(const ColVector<T>& x, const T& scalar)
	{
		return ColVector<T>(dynamic_cast<const MatrixExt<T>&>(x) * scalar);
	}

	template<typename T>
	ColVector<T> operator*(const T& scalar, const ColVector<T>& x)
	{
		return ColVector<T>(scalar * dynamic_cast<const MatrixExt<T>&>(x));
	}

	template<typename T>
	ColVector<T> operator/(const ColVector<T>& x, const ColVector<T>& y)
	{
		return ColVector<T>(dynamic_cast<const MatrixExt<T>&>(x) / dynamic_cast<const MatrixExt<T>&>(y));
	}

	template<typename T>
	ColVector<T> operator/(const ColVector<T>& x, const T& scalar)
	{
		return ColVector<T>(dynamic_cast<const MatrixExt<T>&>(x) / scalar);
	}

	template<typename T>
	ColVector<T> operator/(const T& scalar, const ColVector<T>& x)
	{
		return ColVector<T>(scalar / dynamic_cast<const MatrixExt<T>&>(x));
	}


	template class ColVector<int>;
	template ColVector<int> operator+<int>(const ColVector<int>& x, const ColVector<int>& y);
	template ColVector<int> operator-<int>(const ColVector<int>& x, const ColVector<int>& y);
	template ColVector<int> operator*<int>(const MatrixExt<int>& X, const ColVector<int>& y);
	template ColVector<int> operator*<int>(const ColVector<int>& x, const int& scalar);
	template ColVector<int> operator*<int>(const int& scalar, const ColVector<int>& x);
	template ColVector<int> operator/<int>(const ColVector<int>& x, const ColVector<int>& y);
	template ColVector<int> operator/<int>(const ColVector<int>& x, const int& scalar);
	template ColVector<int> operator/<int>(const int& scalar, const ColVector<int>& x);

	template class ColVector<long long>;
	template ColVector<long long> operator+<long long>(const ColVector<long long>& x, const ColVector<long long>& y);
	template ColVector<long long> operator-<long long>(const ColVector<long long>& x, const ColVector<long long>& y);
	template ColVector<long long> operator*<long long>(const MatrixExt<long long>& X, const ColVector<long long>& y);
	template ColVector<long long> operator*<long long>(const ColVector<long long>& x, const long long& scalar);
	template ColVector<long long> operator*<long long>(const long long& scalar, const ColVector<long long>& x);
	template ColVector<long long> operator/<long long>(const ColVector<long long>& x, const ColVector<long long>& y);
	template ColVector<long long> operator/<long long>(const ColVector<long long>& x, const long long& scalar);
	template ColVector<long long> operator/<long long>(const long long& scalar, const ColVector<long long>& x);

	/*template class ColVector<double>;
	template ColVector<double> operator+<double>(const ColVector<double>& x, const ColVector<double>& y);
	template ColVector<double> operator-<double>(const ColVector<double>& x, const ColVector<double>& y);
	template ColVector<double> operator*<double>(const MatrixExt<double>& X, const ColVector<double>& y);
	template ColVector<double> operator*<double>(const ColVector<double>& x, const double& scalar);
	template ColVector<double> operator*<double>(const double& scalar, const ColVector<double>& x);
	template ColVector<double> operator/<double>(const ColVector<double>& x, const ColVector<double>& y);
	template ColVector<double> operator/<double>(const ColVector<double>& x, const double& scalar);
	template ColVector<double> operator/<double>(const double& scalar, const ColVector<double>& x);*/

	template class ColVector<Rational<long long>>;
	template ColVector<Rational<long long>> operator+<Rational<long long>>(const ColVector<Rational<long long>>& x, const ColVector<Rational<long long>>& y);
	template ColVector<Rational<long long>> operator-<Rational<long long>>(const ColVector<Rational<long long>>& x, const ColVector<Rational<long long>>& y);
	template ColVector<Rational<long long>> operator*<Rational<long long>>(const MatrixExt<Rational<long long>>& X, const ColVector<Rational<long long>>& y);
	template ColVector<Rational<long long>> operator*<Rational<long long>>(const ColVector<Rational<long long>>& x, const Rational<long long>& scalar);
	template ColVector<Rational<long long>> operator*<Rational<long long>>(const Rational<long long>& scalar, const ColVector<Rational<long long>>& x);
	template ColVector<Rational<long long>> operator/<Rational<long long>>(const ColVector<Rational<long long>>& x, const ColVector<Rational<long long>>& y);
	template ColVector<Rational<long long>> operator/<Rational<long long>>(const ColVector<Rational<long long>>& x, const Rational<long long>& scalar);
	template ColVector<Rational<long long>> operator/<Rational<long long>>(const Rational<long long>& scalar, const ColVector<Rational<long long>>& x);

} /// namespace util
} /// namespace gap
