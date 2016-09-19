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

#include "row_col_vector_test.h"
#include "test_utils.h"
#include <gap_util/row_vector.h>
#include <gap_util/column_vector.h>
#include <cassert>

using namespace arma;
using namespace gap::util;

void TestRowVectorArithmeticOps()
{
    RowVector<int> a(5), b(5), c(5);
    MatrixExt<int> A(5,5), B(3,1), P(3,5), P_expected(3,5);

    a << 3 << -3 << 2  << -4 << 9 << endr;
    b << 0 << 1  << -9 << 34 << 7 << endr;
    c << 1 << -1 << 1  << 2  << 3 << endr;

    A << -1 << 0 << 0  << 0 << 0  << endr
      << 0  << 1 << 0  << 0 << 0  << endr
      << 0  << 0 << -1 << 0 << 0  << endr
      << 0  << 0 << 0  << 1 << 0  << endr
      << 0  << 0 << 0  << 0 << -1 << endr;

    B << 1 << endr << -2 << endr << 10 << endr;
    P_expected = { { 0, 1, -9, 34, 7 }, { 0, -2, 18, -68, -14 }, { 0, 10, -90, 340, 70 } };

    RowVector<int> a_prim(a);
    a_prim(0) *= -1; a_prim(2) *= -1; a_prim(4) *= -1;
    P = B * b;

    assert(CompareMatrices(a * A,a_prim));
    assert(CompareMatrices(a - c, { { 2, -2, 1, -6, 6} }));
    assert(CompareMatrices(a * 3, { { 9, -9, 6, -12, 27 } }));
    assert(CompareMatrices(a / 3, { { 1, -1, 0, -1, 3 } }));
    assert(!CompareMatrices(5 / a, { { 1, -1, 2, -1, 1} } ));
    assert(P.n_rows == B.n_rows && P.n_cols == b.n_cols && CompareMatrices(P, P_expected));
}

void TestGetLevel()
{
    RowVector<int> a(0), b(4), c(3), d(5), e(6), e_prim(e);
    b = { 1, 0, 0, 0 };
    c = { 0, 0, 1 };
    d = {  0, 0, 0, 0, 0 };
    e = { 0, -5, 0, -3, 0, 0 };
    e_prim(1) = 6; e_prim(2) = -9;

    assert(a.GetLevel() == a.n_cols);
    assert(b.GetLevel() == 0);
    assert(c.GetLevel() == 2);
    assert(d.GetLevel() == d.n_cols);
    assert(e.GetLevel() == e_prim.GetLevel() && e_prim.GetLevel() == 1);
}

void TestLexicographicalComparison()
{
    RowVector<int> a(1), b(3), c(5), d(2), e(1), f(3), g(2), h(5), i(4), j(4);
    a = { -3 };
    b = { 0, -2, 5 };
    c = { 0, -4, 0, 0, 1 };
    d = { 0, 0 };
    e = { -3 };
    f = { 3, 0, -5 };
    g = { 0, -3 };
    h = { 0, -4, 0, 0, -1 };
    i = { 0, 0, -3, 4 };
    j = { 0, 0, -3, 4 };

    uword level(0);
    RowVector<int>::ComparisonResult result;

    result = RowVector<int>::CompareLexicographically(a, e, level);
    assert(level == a.n_cols && result == RowVector<int>::Equal);

    result = RowVector<int>::CompareLexicographically(b, f, level);
    assert(level == 0 && result == RowVector<int>::RightGreater);

    result = RowVector<int>::CompareLexicographically(c, h, level);
    assert(level == 4 && result == RowVector<int>::LeftGreater);

    result = RowVector<int>::CompareLexicographically(d, g, level);
    assert(level == 1 && result == RowVector<int>::LeftGreater);

    result = RowVector<int>::CompareLexicographically(i, j, level);
    assert(level == i.n_cols && result == RowVector<int>::Equal);


    try
    {
        RowVector<int>::CompareLexicographically(a, g, level);
        assert(!"std::logic_error exception was expected as vectors have different number of columns");
    }
    catch(const std::logic_error&)
    {
    }
    catch(...)
    {
        assert(!"Wrong exception type - std::logic_error was expected");
    }
}

void TestColVectorArithmeticOps()
{
    ColVector<int> a(5), b(5), c(5), p(3), p_expected{ -29, 115, 33 };
    MatrixExt<int> A(5,5), B(3,5);
    RowVector<int> r(5);

    a = { 3, -3, 2, -4, 9 };
    b = { 0, 1, -9, 34, 7 };
    c = { 1, -1, 1, 2, 3 };
    r = { -1, 4, -2, 0, -2 };

    A << -1 << 0 << 0  << 0 << 0  << endr
      << 0  << 1 << 0  << 0 << 0  << endr
      << 0  << 0 << -1 << 0 << 0  << endr
      << 0  << 0 << 0  << 1 << 0  << endr
      << 0  << 0 << 0  << 0 << -1 << endr;

    B << 1  << -2 << 3  << 0 << 0  << endr
      << -2 << 0  << 0  << 4 << -3 << endr
      << 10 << -1 << -3 << 0 << 1  << endr;

    ColVector<int> c_prim(c);
    c_prim(0) *= -1; c_prim(2) *= -1; c_prim(4) *= -1;
    p = B * b;

    assert(r * a == -37);
    assert(CompareMatrices(A * c, c_prim));
    assert(CompareMatrices(a - c, { { 2 }, { -2 }, { 1 }, { -6 }, { 6 } } ));
    assert(CompareMatrices(a * 3, { { 9 }, { -9 }, { 6 }, { -12 }, { 27 } } ));
    assert(CompareMatrices(a / 3, { { 1 }, { -1 }, { 0 }, { -1 }, { 3 } }));
    assert(!CompareMatrices(5 / a, { { 1 }, { -1 }, { 2 }, { -1 }, { 1 } } ));
    assert(p.n_rows == B.n_rows && p.n_cols == b.n_cols && CompareMatrices(p, p_expected));
}

void TestColVectorEchelonReduction()
{
    ColVector<int> a{4, 32, 18}, b{9, -3, 2, 21}, c{5, 35, -20, 25}, d{-4, 52, 0, 0, 16}, S(3);
    MatrixExt<int> U(3,3);

    a.ReduceToEchelon(U, S);
    assert(CompareMatrices(U*a, S));

    b.ReduceToEchelon(U,S);
    assert(CompareMatrices(U*b, S));

    c.ReduceToEchelon(U,S);
    assert(CompareMatrices(U*c, S));

    d.ReduceToEchelon(U,S);
    assert(CompareMatrices(U*d, S));
}

void TestColVectorEchelonReduction2()
{
    ColVector<int> a{4, 32, 18}, b{9, -3, 2, 21}, c{5, 35, -20, 25}, d{-4, 52, 0, 0, 16}, S(3);
    MatrixExt<int> V(3,3);

    a.ReduceToEchelon2(V, S);
    assert(CompareMatrices(a, V*S));

    b.ReduceToEchelon2(V,S);
    assert(CompareMatrices(b, V*S));

    c.ReduceToEchelon2(V,S);
    assert(CompareMatrices(c, V*S));

    d.ReduceToEchelon2(V,S);
    assert(CompareMatrices(d, V*S));
}

void TestColVectorDiagonalization()
{
    ColVector<int> a{4, 32, 18}, b{9, -3, 2, 21}, c{5, 35, -20, 25}, e{-4, 52, 0, 0, 16}, D(3);
    MatrixExt<int> U(3,3), V(1,1);

    a.ReduceToDiagonal(U, V, D);
    assert(CompareMatrices(U*a*V, D));

    b.ReduceToDiagonal(U, V, D);
    assert(CompareMatrices(U*b*V, D));

    c.ReduceToDiagonal(U, V, D);
    assert(CompareMatrices(U*c*V, D));

    e.ReduceToDiagonal(U, V, D);
    assert(CompareMatrices(U*e*V, D));
}
