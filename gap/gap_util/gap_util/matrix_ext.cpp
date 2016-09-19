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
#include "matrix_ext.h"
#include "rational_number.hpp"
#include "util.h"
#include <iostream>
#include <cmath>

namespace gap
{
namespace util
{

    template<typename T>
    MatrixExt<T>::MatrixExt(const uword num_rows, const uword num_cols): Mat<T>(num_rows, num_cols, fill::zeros)
    {
    }

    template<typename T>
    MatrixExt<T>::MatrixExt(const Mat<T>& X): Mat<T>(X)
    {
    }

    template<typename T>
    MatrixExt<T>::MatrixExt(const std::initializer_list<std::initializer_list<T> >& X): Mat<T>(X)
    {
    }

    template<typename T>
    MatrixExt<T>::MatrixExt(Mat<T>&& X): Mat<T>(std::move(X))
    {
    }

    template<typename T>
    const MatrixExt<T>& MatrixExt<T>::operator=(const Mat<T>& X)
    {
        Mat<T>::operator=(X);
        return *this;
    }

    template<typename T>
    const MatrixExt<T>& MatrixExt<T>::operator=(const std::initializer_list<std::initializer_list<T> >& X)
    {
        Mat<T>::operator=(X);
        return *this;
    }

    template<typename T>
    MatrixExt<T> operator+(const MatrixExt<T>& X, const MatrixExt<T>& Y)
    {
        return MatrixExt<T>(dynamic_cast<const Mat<T>& >(X) + dynamic_cast<const Mat<T>& >(Y));
    }

    template<typename T>
    MatrixExt<T> operator-(const MatrixExt<T>& X, const MatrixExt<T>& Y)
    {
        return MatrixExt<T>(dynamic_cast<const Mat<T>& >(X) - dynamic_cast<const Mat<T>& >(Y));
    }

    template<typename T>
    MatrixExt<T> operator*(const MatrixExt<T>& X, const MatrixExt<T>& Y)
    {
        return MatrixExt<T>(dynamic_cast<const Mat<T>& >(X) * dynamic_cast<const Mat<T>& >(Y));
    }

    template<typename T>
    MatrixExt<T> operator*(const MatrixExt<T>& X, const T& scalar)
    {
        return MatrixExt<T>(dynamic_cast<const Mat<T>& >(X) * scalar);
    }

    template<typename T>
    MatrixExt<T> operator*(const T& scalar, const MatrixExt<T>& X)
    {
        return MatrixExt<T>(scalar * dynamic_cast<const Mat<T>& >(X));
    }

    template<typename T>
    MatrixExt<T> operator/(const MatrixExt<T>& X, const MatrixExt<T>& Y)
    {
        return MatrixExt<T>(dynamic_cast<const Mat<T>& >(X) / dynamic_cast<const Mat<T>& >(Y));
    }

    template<typename T>
    MatrixExt<T> operator/(const MatrixExt<T>& X, const T& scalar)
    {
        return MatrixExt<T>(dynamic_cast<const Mat<T>& >(X) / scalar);
    }

    template<typename T>
    MatrixExt<T> operator/(const T& scalar, const MatrixExt<T>& X)
    {
        return MatrixExt<T>(scalar / dynamic_cast<const Mat<T>& >(X));
    }

    template<typename T>
    umat operator==(const MatrixExt<T>& X, const MatrixExt<T>& Y)
    {
        return dynamic_cast<const Mat<T>& >(X) == dynamic_cast<const Mat<T>& >(Y);
    }

    template<typename T>
    umat operator!=(const MatrixExt<T>& X, const MatrixExt<T>& Y)
    {
        return dynamic_cast<const Mat<T>& >(X) != dynamic_cast<const Mat<T>& >(Y);
    }

    template<typename T>
    umat operator>=(const MatrixExt<T>& X, const MatrixExt<T>& Y)
    {
        return dynamic_cast<const Mat<T>& >(X) >= dynamic_cast<const Mat<T>& >(Y);
    }

    template<typename T>
    umat operator<=(const MatrixExt<T>& X, const MatrixExt<T>& Y)
    {
        return dynamic_cast<const Mat<T>& >(X) <= dynamic_cast<const Mat<T>& >(Y);
    }

    template<typename T>
    umat operator>(const MatrixExt<T>& X, const MatrixExt<T>& Y)
    {
        return dynamic_cast<const Mat<T>& >(X) > dynamic_cast<const Mat<T>& >(Y);
    }

    template<typename T>
    umat operator<(const MatrixExt<T>& X, const MatrixExt<T>& Y)
    {
        return dynamic_cast<const Mat<T>& >(X) < dynamic_cast<const Mat<T>& >(Y);
    }

    template<typename T>
    uword MatrixExt<T>::GetRowLevel(const uword row)
    {
        if (row < 0 || row >= this->n_rows)
            throw std::range_error("MatrixExt::FindLeadElement(): argument 'row' not within allowed range");

        for (auto i = 0U; i < this->n_cols; ++i)
        {
            if ((*this)(row, i) != static_cast<T>(0))
                return i;
        }

        /** Row is a zero vector. */
        return this->n_cols;
    }

    template<typename T>
    uword MatrixExt<T>::ComputeRank() const
    {
        MatrixExt<T> U(this->n_rows, this->n_rows), S(*this);
        if (!S.IsEchelon())
            ReduceToEchelon(U, S);

        uword rank_s(0);
        for (auto i = 0U; i < this->n_rows; ++i)
        {
            if (!any(S(i, span(0, S.n_cols - 1))))
                break;

            ++rank_s;
        }
        return rank_s;
    }

    template<typename T>
    bool MatrixExt<T>::IsEchelon()
    {
        int last_row_lead_el(-1);
        for (auto i = 0U; i < this->n_rows; ++i)
        {
            unsigned int curr_row_lead_el(GetRowLevel(i));
            if (last_row_lead_el == this->n_cols && curr_row_lead_el < this->n_cols)
                return false;
            else if (last_row_lead_el >= static_cast<int>(curr_row_lead_el) && curr_row_lead_el < this->n_cols)
                return false;

            last_row_lead_el = curr_row_lead_el;
        }

        return true;
    }

    template<typename T>
    bool MatrixExt<T>::IsDiagonal()
    {
        for (auto i = 0U; i < this->n_rows; ++i)
        {
            for (auto j = 0U; j < this->n_cols; ++j)
            {
                if ((*this)(i,j) != static_cast<T>(0) && i != j)
                    return false;
            }
        }

        return true;
    }

    template<typename T>
    MatrixExt<T> MatrixExt<T>::GenReversalMatrix(const uword n_rows, const uword n_to_invert)
    {
        if (n_to_invert >= n_rows)
            throw std::range_error("MatrixExt::GenReversalMatrix(): n_to_invert belongs to segment [0, n_rows-1]");

        MatrixExt<T> out(Mat<T>(n_rows, n_rows, fill::eye));
        out(n_to_invert, n_to_invert) = -1;
        return out;
    }

    template<typename T>
    MatrixExt<T> MatrixExt<T>::GenInterchangeMatrix(const uword n_rows, const uword x, const uword y)
    {
        if (x >= n_rows || y >= n_rows)
            throw std::range_error("MatrixExt::GenInterchangeMatrix(): x and y arguments must be smaller than n_rows");

        MatrixExt<T> out(Mat<T>(n_rows, n_rows, fill::eye));
        out(x,x) = 0;
        out(x,y) = 1;
        out(y,y) = 0;
        out(y,x) = 1;
        return out;
    }

    template<typename T>
    MatrixExt<T> MatrixExt<T>::GenSkewingMatrix(const uword n_rows, const uword source, const uword target, const T scalar, bool row_skew)
    {
        if (source >= n_rows || target >= n_rows)
            throw std::range_error("MatrixExt::GenSkewingMatrix(): source/target row/col must be smaller than n_rows");

        if (source == target)
            throw std::logic_error("MatrixExt::GenSkewingMatrix(): source row/col must be different than target row/col");

        MatrixExt<T> out(Mat<T>(n_rows, n_rows, fill::eye));
        if (row_skew)
            out(target, source) = scalar;
        else
            out(source, target) = scalar;

        return out;
    }

    template<typename T>
    MatrixExt<T>& MatrixExt<T>::DoReversalOperation(const uword n_to_invert, bool row_reversal)
    {
        if (row_reversal)
        {
            if (n_to_invert >= this->n_rows)
                throw std::range_error("MatrixExt::DoReversalOperation(): n_to_invert must be smaller than n_rows");

            this->row(n_to_invert) *= -1;
        }
        else
        {
            if (n_to_invert >= this->n_cols)
                throw std::range_error("MatrixExt::DoReversalOperation(): n_to_invert must be smaller than n_cols");

            this->col(n_to_invert) *= -1;
        }
        return *this;
    }

    template<typename T>
    MatrixExt<T>& MatrixExt<T>::DoInterchangeOperation(const uword x, const uword y, bool row_interchange)
    {
        if (row_interchange)
        {
            if (x >= this->n_rows || y >= this->n_rows)
                throw std::range_error("MatrixExt::DoInterchangeOperation(): x and y must be smaller than n_rows");

            Row<T> r(this->row(x));
            this->row(x) = this->row(y);
            this->row(y) = r;
        }
        else
        {
            if (x >= this->n_cols || y >= this->n_cols)
                throw std::range_error("MatrixExt::DoInterchangeOperation(): x and y must be smaller than n_cols");

            Col<T> c(this->col(x));
            this->col(x) = this->col(y);
            this->col(y) = c;
        }
        return *this;
    }

    template<typename T>
    MatrixExt<T>& MatrixExt<T>::DoSkewingOperation(const uword source, const uword target, const T scalar, bool row_skew)
    {
        if (source == target)
            throw std::logic_error("MatrixExt::DoSkewingOperation(): source row/col must be different than target row/col");

        if (row_skew)
        {
            if (source >= this->n_rows || target >= this->n_rows)
                throw std::range_error("MatrixExt::DoSkewingOperation(): source/target row must be smaller than n_rows");

            this->row(target) += MatrixExt<T>(this->row(source)) * scalar;
        }
        else
        {
            if (source >= this->n_cols || target >= this->n_cols)
                throw std::range_error("MatrixExt::DoSkewingOperation(): source/target row must be smaller than n_cols");

            this->col(target) += this->col(source) * scalar;
        }
        return *this;
    }

    template<typename T>
    uword MatrixExt<T>::ReduceToEchelon(MatrixExt<T>& U, MatrixExt<T>& S) const
    {
        U.eye(this->n_rows, this->n_rows);
        S = *this;
        if (S.IsEchelon())
            return S.ComputeRank();

        uword i0 = -1;
        for (uword j = 0; j < this->n_cols; ++j)
        {
            if (i0 < this->n_rows - 1 && !any(S(span(i0 + 1, this->n_rows - 1), j)))
                continue;

            if (++i0 >= this->n_rows - 1)
                break;

            for (uword i = this->n_rows - 1; i >= i0 + 1; --i)
            {
                while(S(i,j) != T(0))
                {
					T multiplier = T(-1) * static_cast<T>(util::signum(S(i - 1, j) * S(i, j)) * std::floor(std::abs(S(i - 1, j)) / std::abs(S(i, j))));
                    U.DoSkewingOperation(i, i-1, multiplier, true).DoInterchangeOperation(i, i-1, true);
                    S.DoSkewingOperation(i, i-1, multiplier, true).DoInterchangeOperation(i, i-1, true);
                }
            }
        }

        /** i0 + 1 is the rank of the echelon matrix S. */
        return i0 + 1;
    }

    template<typename T>
    uword MatrixExt<T>::ReduceToEchelon2(MatrixExt<T>& V, MatrixExt<T>& S) const
    {
        V.eye(this->n_rows, this->n_rows);
        S = *this;
        if (S.IsEchelon())
            return S.ComputeRank();

        uword i0 = -1;
        for (uword j = 0; j < this->n_cols; ++j)
        {
            if (i0 < this->n_rows - 1 && !any(S(span(i0 + 1, this->n_rows - 1), j)))
                continue;

            if (++i0 >= this->n_rows - 1)
                break;

            for (uword i = this->n_rows - 1; i >= i0 + 1; --i)
            {
                while(S(i,j) != T(0))
                {
					T multiplier = static_cast<T>(util::signum(S(i - 1, j) * S(i, j)) * std::floor(std::abs(S(i - 1, j)) / std::abs(S(i, j))));
                    S.DoSkewingOperation(i, i-1, T(-1) * multiplier, true).DoInterchangeOperation(i, i-1, true);
                    V.DoSkewingOperation(i-1, i, multiplier, false).DoInterchangeOperation(i, i-1, false);
                }
            }
        }

        /** i0 + 1 is the rank of the echelon matrix S. */
        return i0 + 1;
    }

    template<typename T>
    void MatrixExt<T>::ReduceToDiagonal(MatrixExt<T>& U, MatrixExt<T>& V, MatrixExt<T>& D) const
    {
        U.eye(this->n_rows, this->n_rows);
        V.eye(this->n_cols, this->n_cols);
        D = *this;
        if (D.IsDiagonal())
            return;

        uword k (0), smaller_dim(this->n_rows <= this->n_cols ? this->n_rows : this->n_cols);
        T abs_min(0);
        uword p(0), q(0);
        do
        {
            /** Find absolute minimum value for k <= i <= m and k <= j <= n. */
            abs_min = std::abs(D(k,k));
            p = q = k;
            T tmp;
            for (auto i = k + 1; i < this->n_rows; ++i)
            {
                tmp = std::abs(D(i,k));
                if (tmp > T(0) && tmp < abs_min)
                {
                    abs_min = tmp;
                    p = i;
                    q = k;
                }
            }

            for (auto j = k + 1; j < this->n_cols; ++j)
            {
                tmp = std::abs(D(k,j));
                if (tmp > T(0) && tmp < abs_min)
                {
                    abs_min = tmp;
                    p = k;
                    q = j;
                }
            }

            /**
             * Interchange rows k and p (if k != p) in D and U, and cols k and q (if k != q) in D and V
             * to bring absolute minimum to (k,k) position.
             */
            if (p > k)
            {
                D.DoInterchangeOperation(k, p, true);
                U.DoInterchangeOperation(k, p, true);
            }
            else if (q > k)
            {
                D.DoInterchangeOperation(k, q, false);
                V.DoInterchangeOperation(k, q, false);
            }

            /**
             * Subtract appropriate multiple of row k from rows k <= i <= m in D and U. Subtract appropriate
             * multiple of col k from cols k <= j <= n in D and V. This step aims to decrease the values
             * outside the main diagonal as much as possible (eventually to 0);
             */
            bool all_nullified(true);
            T multiplier(0);
            for (auto i = k + 1; i < this->n_rows; ++i)
            {
				multiplier = T(-1) * static_cast<T>(util::signum(D(i, k) * D(k, k)) * std::floor(std::abs(D(i, k)) / std::abs(D(k, k))));
                D.DoSkewingOperation(k, i, multiplier, true);
                U.DoSkewingOperation(k, i, multiplier, true);
                if (D(i,k) != T(0))
                    all_nullified = false;
            }

            for (auto j = k + 1; j < this->n_cols; ++j)
            {
				multiplier = T(-1) * static_cast<T>(util::signum(D(k, j) * D(k, k)) * std::floor(std::abs(D(k, j)) / std::abs(D(k, k))));
                D.DoSkewingOperation(k, j, multiplier, false);
                V.DoSkewingOperation(k, j, multiplier, false);
                if (D(k,j) != T(0))
                    all_nullified = false;
            }

            /**
             * If there is at least one element D(i,k) k <= i <= m or D(k,j) k <= j <= n not equal to zero, we
             * have to repeat another iteration with the same value of k.
             */
            if (!all_nullified)
                continue;

            /**
             * Otherwise increment k and continue with the submatrix of D obtained by crossing out the topmost
             * k rows and the leftmost k columns.
             */
            ++k;

        } while(k < smaller_dim);
    }


    template class MatrixExt<int>;
    template MatrixExt<int> operator+<int>(const MatrixExt<int>& X, const MatrixExt<int>& Y);
    template MatrixExt<int> operator-<int>(const MatrixExt<int>& X, const MatrixExt<int>& Y);
    template MatrixExt<int> operator*<int>(const MatrixExt<int>& X, const MatrixExt<int>& Y);
    template MatrixExt<int> operator*<int>(const MatrixExt<int>& X, const int& scalar);
    template MatrixExt<int> operator*<int>(const int& scalar, const MatrixExt<int>& X);
    template MatrixExt<int> operator/<int>(const MatrixExt<int>& X, const MatrixExt<int>& Y);
    template MatrixExt<int> operator/<int>(const MatrixExt<int>& X, const int& scalar);
    template MatrixExt<int> operator/<int>(const int& scalar, const MatrixExt<int>& X);
    template umat operator==<int>(const MatrixExt<int>& X, const MatrixExt<int>& Y);
    template umat operator!=<int>(const MatrixExt<int>& X, const MatrixExt<int>& Y);
    template umat operator>=<int>(const MatrixExt<int>& X, const MatrixExt<int>& Y);
    template umat operator<=<int>(const MatrixExt<int>& X, const MatrixExt<int>& Y);
    template umat operator><int>(const MatrixExt<int>& X, const MatrixExt<int>& Y);
    template umat operator< <int>(const MatrixExt<int>& X, const MatrixExt<int>& Y);

	template class MatrixExt<long long>;
	template MatrixExt<long long> operator+<long long>(const MatrixExt<long long>& X, const MatrixExt<long long>& Y);
	template MatrixExt<long long> operator-<long long>(const MatrixExt<long long>& X, const MatrixExt<long long>& Y);
	template MatrixExt<long long> operator*<long long>(const MatrixExt<long long>& X, const MatrixExt<long long>& Y);
	template MatrixExt<long long> operator*<long long>(const MatrixExt<long long>& X, const long long& scalar);
	template MatrixExt<long long> operator*<long long>(const long long& scalar, const MatrixExt<long long>& X);
	template MatrixExt<long long> operator/<long long>(const MatrixExt<long long>& X, const MatrixExt<long long>& Y);
	template MatrixExt<long long> operator/<long long>(const MatrixExt<long long>& X, const long long& scalar);
	template MatrixExt<long long> operator/<long long>(const long long& scalar, const MatrixExt<long long>& X);
	template umat operator==<long long>(const MatrixExt<long long>& X, const MatrixExt<long long>& Y);
	template umat operator!=<long long>(const MatrixExt<long long>& X, const MatrixExt<long long>& Y);
	template umat operator>=<long long>(const MatrixExt<long long>& X, const MatrixExt<long long>& Y);
	template umat operator<=<long long>(const MatrixExt<long long>& X, const MatrixExt<long long>& Y);
	template umat operator><long long>(const MatrixExt<long long>& X, const MatrixExt<long long>& Y);
	template umat operator< <long long>(const MatrixExt<long long>& X, const MatrixExt<long long>& Y);

    template class MatrixExt<double>;
    template MatrixExt<double> operator+<double>(const MatrixExt<double>& X, const MatrixExt<double>& Y);
    template MatrixExt<double> operator-<double>(const MatrixExt<double>& X, const MatrixExt<double>& Y);
    template MatrixExt<double> operator*<double>(const MatrixExt<double>& X, const MatrixExt<double>& Y);
    template MatrixExt<double> operator*<double>(const MatrixExt<double>& X, const double& scalar);
    template MatrixExt<double> operator*<double>(const double& scalar, const MatrixExt<double>& X);
    template MatrixExt<double> operator/<double>(const MatrixExt<double>& X, const MatrixExt<double>& Y);
    template MatrixExt<double> operator/<double>(const MatrixExt<double>& X, const double& scalar);
    template MatrixExt<double> operator/<double>(const double& scalar, const MatrixExt<double>& X);
    template umat operator==<double>(const MatrixExt<double>& X, const MatrixExt<double>& Y);
    template umat operator!=<double>(const MatrixExt<double>& X, const MatrixExt<double>& Y);
    template umat operator>=<double>(const MatrixExt<double>& X, const MatrixExt<double>& Y);
    template umat operator<=<double>(const MatrixExt<double>& X, const MatrixExt<double>& Y);
    template umat operator><double>(const MatrixExt<double>& X, const MatrixExt<double>& Y);
    template umat operator< <double>(const MatrixExt<double>& X, const MatrixExt<double>& Y);

	template class MatrixExt<Rational<long long>>;
	template MatrixExt<Rational<long long>> operator+<Rational<long long>>(const MatrixExt<Rational<long long>>& X, const MatrixExt<Rational<long long>>& Y);
	template MatrixExt<Rational<long long>> operator-<Rational<long long>>(const MatrixExt<Rational<long long>>& X, const MatrixExt<Rational<long long>>& Y);
	template MatrixExt<Rational<long long>> operator*<Rational<long long>>(const MatrixExt<Rational<long long>>& X, const MatrixExt<Rational<long long>>& Y);
	template MatrixExt<Rational<long long>> operator*<Rational<long long>>(const MatrixExt<Rational<long long>>& X, const Rational<long long>& scalar);
	template MatrixExt<Rational<long long>> operator*<Rational<long long>>(const Rational<long long>& scalar, const MatrixExt<Rational<long long>>& X);
	template MatrixExt<Rational<long long>> operator/<Rational<long long>>(const MatrixExt<Rational<long long>>& X, const MatrixExt<Rational<long long>>& Y);
	template MatrixExt<Rational<long long>> operator/<Rational<long long>>(const MatrixExt<Rational<long long>>& X, const Rational<long long>& scalar);
	template MatrixExt<Rational<long long>> operator/<Rational<long long>>(const Rational<long long>& scalar, const MatrixExt<Rational<long long>>& X);
	template umat operator==<Rational<long long>>(const MatrixExt<Rational<long long>>& X, const MatrixExt<Rational<long long>>& Y);
	template umat operator!=<Rational<long long>>(const MatrixExt<Rational<long long>>& X, const MatrixExt<Rational<long long>>& Y);
	template umat operator>=<Rational<long long>>(const MatrixExt<Rational<long long>>& X, const MatrixExt<Rational<long long>>& Y);
	template umat operator<=<Rational<long long>>(const MatrixExt<Rational<long long>>& X, const MatrixExt<Rational<long long>>& Y);
	template umat operator><Rational<long long>>(const MatrixExt<Rational<long long>>& X, const MatrixExt<Rational<long long>>& Y);
	template umat operator< <Rational<long long>>(const MatrixExt<Rational<long long>>& X, const MatrixExt<Rational<long long>>& Y);

} /// namespace util
} /// namespace gap
