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

#ifndef GAP_UTIL_MATRIX_EXT_H
#define GAP_UTIL_MATRIX_EXT_H

#include <initializer_list>
#include <armadillo/armadillo>
#include <type_traits>

namespace gap
{
namespace util
{

    using namespace arma;

    template<typename T>
    class MatrixExt;

    template<typename T>
    MatrixExt<T> operator+(const MatrixExt<T>& X, const MatrixExt<T>& Y);

    template<typename T>
    MatrixExt<T> operator-(const MatrixExt<T>& X, const MatrixExt<T>& Y);

    template<typename T>
    MatrixExt<T> operator*(const MatrixExt<T>& X, const MatrixExt<T>& Y);

    template<typename T>
    MatrixExt<T> operator*(const MatrixExt<T>& X, const T& scalar);

    template<typename T>
    MatrixExt<T> operator*(const T& scalar, const MatrixExt<T>& X);

    template<typename T>
    MatrixExt<T> operator/(const MatrixExt<T>& X, const MatrixExt<T>& Y);

    template<typename T>
    MatrixExt<T> operator/(const MatrixExt<T>& X, const T& scalar);

    template<typename T>
    MatrixExt<T> operator/(const T& scalar, const MatrixExt<T>& X);

    template<typename T>
    umat operator==(const MatrixExt<T>& X, const MatrixExt<T>& Y);

    template<typename T>
    umat operator!=(const MatrixExt<T>& X, const MatrixExt<T>& Y);

    template<typename T>
    umat operator>=(const MatrixExt<T>& X, const MatrixExt<T>& Y);

    template<typename T>
    umat operator<=(const MatrixExt<T>& X, const MatrixExt<T>& Y);

    template<typename T>
    umat operator>(const MatrixExt<T>& X, const MatrixExt<T>& Y);

    template<typename T>
    umat operator<(const MatrixExt<T>& X, const MatrixExt<T>& Y);

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, bool>::type Equal(const MatrixExt<T>& X, const MatrixExt<T>& Y);

	template<typename T>
	typename std::enable_if<std::is_floating_point<T>::value, bool>::type Equal(const MatrixExt<T>& X, const MatrixExt<T>& Y);

    template<typename T>
    class MatrixExt : public Mat<T>
    {

    public:
        MatrixExt(const uword num_rows, const uword num_cols);

		template<typename fill_type>
		MatrixExt(const uword num_rows, const uword num_cols, const arma::fill::fill_class<fill_type>& f) : Mat<T>(num_rows, num_cols, f)
		{
		}

        MatrixExt(const Mat<T>& X);
        MatrixExt(const std::initializer_list<std::initializer_list<T> >& X);
        MatrixExt(Mat<T>&& X);
        const MatrixExt& operator=(const Mat<T>& X);
        const MatrixExt& operator=(const std::initializer_list<std::initializer_list<T> >& X);
        virtual ~MatrixExt()
        {
        }

		template<typename T2>
		operator MatrixExt<T2>() const
		{
			MatrixExt<T2> out(this->n_rows, this->n_cols);
			auto j = this->begin();
			for (auto i = out.begin(); i != out.end(); ++i, ++j)
				*i = T2(*j);
			return out;
		}

        friend MatrixExt operator+<T>(const MatrixExt& X, const MatrixExt& Y);
        friend MatrixExt operator-<T>(const MatrixExt& X, const MatrixExt& Y);
        friend MatrixExt operator*<T>(const MatrixExt& X, const MatrixExt& Y);
        friend MatrixExt operator*<T>(const MatrixExt& X, const T& scalar);
        friend MatrixExt operator*<T>(const T& scalar, const MatrixExt& X);
        friend MatrixExt operator/<T>(const MatrixExt& X, const MatrixExt& Y);
        friend MatrixExt operator/<T>(const MatrixExt& X, const T& scalar);
        friend MatrixExt operator/<T>(const T& scalar, const MatrixExt& X);
        friend umat operator==<T>(const MatrixExt& X, const MatrixExt& Y);
        friend umat operator!=<T>(const MatrixExt& X, const MatrixExt& Y);
        friend umat operator>=<T>(const MatrixExt& X, const MatrixExt& Y);
        friend umat operator<=<T>(const MatrixExt& X, const MatrixExt& Y);
        friend umat operator><T>(const MatrixExt& X, const MatrixExt& Y);
        friend umat operator< <T>(const MatrixExt& X, const MatrixExt& Y);
		friend bool Equal<T>(const MatrixExt& X, const MatrixExt& Y);

        /**
         * @brief Returns column index of the first non-zero element of the given row,
         * also known as a 'level'. If row is zero vector return value is n_cols.
         *
         * @throw std::range_error if row < 0 or row > n_rows - 1
         */
        uword GetRowLevel(const uword row);

        /**
         * @brief Computes rank of this instance by reducing it to echelon form counting the number of non-zero
         * rows (these are linearly independent rows in case of echelon matrix).
         *
         * @param None
         * @retval uword value representing the rank
         */
        uword ComputeRank() const;

        /**
         * @brief Checks if this matrix is echelon matrix.
         */
        virtual bool IsEchelon();

        /**
         * @brief Checks if this matrix is diagonal matrix.
         */
        virtual bool IsDiagonal();

        /**
         * @brief Generates reversal matrix for row/column n. Premultiplying this matrix with
         * given matrix A will result matrix A' same as A but with nth row multiplied with -1.
         * Postmultiplying this matrix with some matrix B will result in matrix B' same as B
         * but with nth column multiplied with -1.
         *
         * Reversal matrices are square, so caller has to specify only one dimension. Transponse
         * of a reversal matrix is the same matrix, so the same reversal matrix can be used to
         * perform both row and column operations.
         *
         * @throw std::range_error if n_to_invert > n_rows - 1
         */
        static MatrixExt GenReversalMatrix(const uword n_rows, const uword n_to_invert);

        /**
         * @brief Generates interchange matrix of rows/columns x and y. Premultiplying this matrix
         * with given matrix A will result matrix A' same as A but with rows x and y interchanged.
         * Postmultiplying this matrix with some matrix B will result in matrix B' same as B
         * but with columns x and y interchanged.
         *
         * Interchange matrices are square, so caller has to specify only one dimension. Transponse
         * of a interchange matrix is the same matrix, so the same interchange matrix can be used to
         * perform both row and column operations.
         *
         * @throw std::range_error if x > n_rows - 1 || y > n_rows - 1
         */
        static MatrixExt GenInterchangeMatrix(const uword n_rows, const uword x, const uword y);

        /**
         * @brief Generates skewing matrix that adds integer multiple of one row/column to another
         * row/column. Skewing matrices are square, so caller has to specify only one dimension. As
         * skewing matrix is formed differently for row and column operations, caller must specify
         * what type of skewing matrix should be created.
         *
         * @throw std::range_error if target > n_rows - 1 || source > n_rows - 1
         * @throw std::logic_error if source == target
         */
        static MatrixExt GenSkewingMatrix(const uword n_rows, const uword source, const uword target, const T scalar, bool row_skew);

        /**
         * @brief Similar like GenReversalMatrix except that it actually performs row/column reversal
         * operation on this instance. Method also returns the reference to this instance to allow chaining.
         *
         * @throw std::range_error if (n_to_invert > n_rows - 1 && row_reversal) || (n_to_invert > n_cols - 1 && !row_reversal)
         */
        MatrixExt& DoReversalOperation(const uword n_to_invert, bool row_reversal);

        /**
         * @brief Similar like GenInterchangeMatrix except that it actually performs row/column interchange
         * operation on this instance. Method also returns the reference to this instance to allow chaining.
         *
         * @throw std::range_error if [ (x > n_rows - 1 || y > n_rows - 1) && row_interchange ] ||
         *                            [ (x > n_cols - 1 || y > n_cols - 1) && !row_interchange ]
         */
        MatrixExt& DoInterchangeOperation(const uword x, const uword y, bool row_interchange);

        /**
         * @brief Similar like GenSkewingMatrix expect that it actually performs row/colum skewing operation
         * on this instance. Method also returns the reference to this instance to allow chaining.
         *
         * @throw std::logic_error if source == target
         * @throw std::range_error if [ (source > n_rows - 1 || target > n_rows - 1) && row_skew ] ||
                                      [ (source > n_cols - 1 || target > n_cols - 1) && !row_skew ]
         */
        MatrixExt& DoSkewingOperation(const uword source, const uword target, const T scalar, bool row_skew);

        /**
         * @brief Performs Echelon Reduction on this instance. The product of this algorithm is unimodular matrix U
         * and echelon matrix S such that: UA = S where A is this instance.
         *
         * @param Unimodular matrix U and echelon matrix S that will be the result of echelonization
         * @retval Rank of matrix S (number of non-zero rows in case of echelon matrices)
         *
         * @see Loop Transformations for Restructuring Compilers, The Foundations (Algorithm 2.1)
         */
        uword ReduceToEchelon(MatrixExt& U, MatrixExt& S) const;

        /**
         * @brief Perform Modified Echelon Reduction on this instance. The product of this algorithm is unimodular
         * matrix V and echelon matrix S such that: A = VS where A is this instance.
         *
         * @param Unimodular matrix U and echelon matrix S that will be the result of echelonization
         * @retval Rank of matrix S (number of non-zero rows in case of echelon matrices)
         *
         * @see Loop Transformations for Restructuring Compilers, The Foundations (Algorithm 2.2)
         */
        uword ReduceToEchelon2(MatrixExt& V, MatrixExt& S) const;

        /**
         * @brief Performs Diagonalization on this instance. The product of this algorithm are unimodular matrices
         * U and V and diagonal matrix D such that: UAV = D where A is this instance.
         *
         * @see Loop Transformations for Restructuring Compilers, The Foundations (Algorithm 2.3)
         */
        void ReduceToDiagonal(MatrixExt& U, MatrixExt& V, MatrixExt& D) const;
    };


	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, bool>::type Equal(const MatrixExt<T>& X, const MatrixExt<T>& Y)
	{
		if (X.n_rows != Y.n_rows || X.n_cols != Y.n_cols)
			throw std::range_error("Dimensions not equal");

		for (auto i = X.begin(), j = Y.begin(); i != X.end() && j != Y.end(); ++i, ++j)
			if (*i != *j)
				return false;

		return true;
	}

	template<typename T>
	typename std::enable_if<std::is_floating_point<T>::value, bool>::type Equal(const MatrixExt<T>& X, const MatrixExt<T>& Y)
	{
		if (X.n_rows != Y.n_rows || X.n_cols != Y.n_cols)
			throw std::range_error("Dimensions not equal");

		for (auto i = X.begin(), j = Y.begin(); i != X.end() && j != Y.end(); ++i, ++j)
			if (not_equal(*i, *j))
				return false;

		return true;
	}

} /// namespace util
} /// namespace gap

#endif // GAP_UTIL_MATRIX_EXT_H
