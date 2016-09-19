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

#ifndef GAP_FRONTEND_MAT_VEC_PAIR_H
#define GAP_FRONTEND_MAT_VEC_PAIR_H

#include <gap_util/row_vector.h>
#include <gap_util/rational_number.hpp>
#include <cassert>

namespace gap
{

	/// Convenience struct holding the matrix and row vector pair. Used to
	/// model loop nest bounds and array subscripts as required by the dep.
	/// analyser. If matrix dim is (m x n) then row vector's dim must be
	/// (1 x n)
	template<class ElemType>
	struct MatVecPair
	{
		typedef util::MatrixExt<ElemType> MatrixType;
		typedef util::RowVector<ElemType> RowVecType;

		MatVecPair()
			: m_mat(0, 0)
			, m_vec(0)
		{
		}

		MatVecPair(std::size_t n_rows, std::size_t n_cols)
			: m_mat(n_rows, n_cols)
			, m_vec(n_cols)
		{
		}

		MatVecPair(const MatrixType& mat, const RowVecType& vec)
			: m_mat(mat)
			, m_vec(vec)
		{
			assert(
				m_mat.n_cols == m_vec.n_cols
				&& "matrix and vector must have equal number of columns");
		}

		MatVecPair(MatrixType&& mat, RowVecType&& vec)
			: m_mat(std::move(mat))
			, m_vec(std::move(vec))
		{
			assert(
				m_mat.n_cols == m_vec.n_cols
				&& "matrix and vector must have equal number of columns");
		}

		MatrixType m_mat;
		RowVecType m_vec;
	};


	typedef MatVecPair<long long> IntMatVecPair;
	typedef MatVecPair<util::Rational<long long>> RationalMatVecPair;

} /// namespace gap

#endif /// GAP_FRONTEND_MAT_VEC_PAIR_H
