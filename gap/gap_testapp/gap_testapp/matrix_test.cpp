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

#include "matrix_test.h"
#include <gap_util/matrix_ext.h>
#include <gap_util/rational_number.hpp>
#include <cassert>
#include <stdexcept>
#include "test_utils.h"

using namespace arma;
using namespace gap::util;

void TestRationalMatrixArithmetic()
{
	/*c2cuda::MatrixExt<c2cuda::Rational<int>> A(3, 5), B(5, 5, arma::fill::eye), C(4, 3, arma::fill::ones), D(4, 3, arma::fill::ones);
	A	<< 4 << c2cuda::Rational<int>(3, 2) << c2cuda::Rational<int>(9, 10) << 1 << 0 << endr
		<< c2cuda::Rational<int>(10, 50) << 89 << 9 << c2cuda::Rational<long>(45, 7) << 0 << endr
		<< 0 << 0 << 0 << 0	<< c2cuda::Rational<int>(1, 19) << endr;

	assert(CompareMatrices(A, A * B));
	assert(CompareMatrices(D - C, c2cuda::MatrixExt<c2cuda::Rational<int>>(D.n_rows, D.n_cols)));*/
}

void TestIsEchelon()
{
    MatrixExt<int> A(4,4), B(4,3), C(4,3), D(4,3), E(4,3), F(0,0), G(zeros<Mat<int> >(4,5)),
        H(1, 5), I(4,1), J(4,1), K(4,1), L(3,5);

    A << 1 << 0 << 0 << 0  << endr
      << 0 << 2 << 0 << 0  << endr
      << 0 << 0 << 6 << 0  << endr
      << 0 << 0 << 0 << -1 << endr;

    B << 5 << 2  << 3 << endr
      << 0 << -1 << 0 << endr
      << 0 << 0  << 3 << endr
      << 0 << 0  << 0 << endr;

    C << 5 << 2  << 3 << endr
      << 0 << -1 << 0 << endr
      << 0 << 0  << 0 << endr
      << 0 << 5  << 0 << endr;

    D << 5 << 2  << 3 << endr
      << 0 << -1 << 0 << endr
      << 0 << 0  << 3 << endr
      << 0 << 0  << 7 << endr;

    E << 5  << 2  << 3 << endr
      << 0  << -1 << 0 << endr
      << 0  << 0  << 3 << endr
      << -3 << 0  << 0 << endr;

    H << 0 << -2 << 4 << 0 << 1 << endr;

    I << 0 << endr
      << 0 << endr
      << 0 << endr
      << 2 << endr;

    J << 5 << endr
      << 0 << endr
      << 0 << endr
      << 2 << endr;

    K << 5 << endr
      << 0 << endr
      << 0 << endr
      << 0 << endr;

    L << 0 << 0 << 0 << 0 << 0 << endr
      << 0 << 0 << 0 << 0 << 0 << endr
      << 0 << 0 << 0 << 0 << 0 << endr;

    assert(A.IsEchelon());
    assert(B.IsEchelon());
    assert(!C.IsEchelon());
    assert(!D.IsEchelon());
    assert(!E.IsEchelon());
    assert(F.IsEchelon());
    assert(G.IsEchelon());
    assert(H.IsEchelon());
    assert(!I.IsEchelon());
    assert(!J.IsEchelon());
    assert(K.IsEchelon());
    assert(L.IsEchelon());
}

void TestIsDiagonal()
{
    MatrixExt<int> A(4,4), B(4,3), C(4,3), D(4,3), E(5,3), F(0,0), G(zeros<Mat<int> >(4,5)),
        H(1, 5), I(4,1), J(4,1), K(4,1);

    A << 1 << 0 << 0 << 0  << endr
      << 0 << 2 << 0 << 0  << endr
      << 0 << 0 << 6 << 0  << endr
      << 0 << 0 << 0 << -1 << endr;

    B << 5 << 2  << 3 << endr
      << 0 << -1 << 0 << endr
      << 0 << 0  << 3 << endr
      << 0 << 0  << 0 << endr;

    C << 5 << 0  << 0 << endr
      << 0 << -1 << 0 << endr
      << 0 << 0  << 0 << endr
      << 0 << 0  << 0 << endr;

    D << 5 << 0  << 0 << endr
      << 0 << -1 << 0 << endr
      << 0 << 0  << 3 << endr
      << 0 << 0  << 7 << endr;

    E << 5  << 0  << 0 << 0 << 0 << endr
      << 0  << -1 << 0 << 0 << 0 << endr
      << 0  << 0  << 3 << 0 << 0 << endr
      << 0  << 0  << 0 << 0 << 0<< endr;

    H << 0 << 0 << 0 << 0 << 1 << endr;

    I << 1 << endr
      << 0 << endr
      << 0 << endr
      << 0 << endr;

    J << 5 << endr
      << 0 << endr
      << 0 << endr
      << 2 << endr;

    K << 0 << endr
      << 0 << endr
      << 0 << endr
      << 0 << endr;

    assert(A.IsDiagonal());
    assert(!B.IsDiagonal());
    assert(C.IsDiagonal());
    assert(!D.IsDiagonal());
    assert(E.IsDiagonal());
    assert(F.IsDiagonal());
    assert(G.IsDiagonal());
    assert(!H.IsDiagonal());
    assert(I.IsDiagonal());
    assert(!J.IsDiagonal());
    assert(K.IsDiagonal());
}

void TestComputeRank()
{
    MatrixExt<int> A(4,4), B(4,3), C(3,5), D(2,4), E(3,2), F(0,0), G(4,5),
        H(1, 5), I(4,1), J(3,7);

    A << 1 << -1  << 2  << 0  << endr
      << 0 << 2   << 4  << -5 << endr
      << 0 << -10 << 6  << 0  << endr
      << 0 << 12  << -4 << -1 << endr;

    B << 5  << 2  << 3 << endr
      << 0  << -1 << 4 << endr
      << -3 << 1  << 3 << endr
      << 2  << -4 << 0 << endr;

    C << 5 << 2  << 3 << 0 << -2 << endr
      << 0 << -1 << 0 << -1 << 3 << endr
      << 7 << 0  << -2 << 0 << 0 << endr;

    D << 5 << 0 << 3  << 0 << endr
      << 4 << 0 << -7 << 0 << endr;

    E << 5  << 2  << endr
      << 0  << -1 << endr
      << 10 << -4 << endr;

    H << 0 << -2 << 4 << 0 << 1 << endr;

    I << 5   << endr
      << -35 << endr
      << 7   << endr
      << -24 << endr;

    J << 0 << -5  << 0 << 9   << 0 << -3 << 12 << endr
      << 0 << 0   << 0 << -34 << 0 << -4 << 0  << endr
      << 0 << -45 << 0 << 9   << 0 << 67 << 0  << endr;

    assert(A.ComputeRank() == 4);
    assert(B.ComputeRank() == 3);
    assert(C.ComputeRank() == 3);
    assert(D.ComputeRank() == 2);
    assert(E.ComputeRank() == 2);
    assert(F.ComputeRank() == 0);
    assert(G.ComputeRank() == 0);
    assert(H.ComputeRank() == 1);
    assert(I.ComputeRank() == 1);
    assert(J.ComputeRank() == 3);
}

void TestGenReversalMatrix()
{
    MatrixExt<int> A(3,4), B(5,2), C(4,4), D(1,1);
    A << -1 << 4 << 5 << 8 << endr
      << 3  << 0 << 1 << 0 << endr
      << 0  << 0 << 0 << 0 << endr;

    B << 2  << -3 << endr
      << 0  << 4  << endr
      << -4 << 0  << endr
      << 7  << 0  << endr
      << -4 << -4 << endr;

    C << 3 << 0 << 2 << -1 << endr
      << 0 << 0 << 0 << -2 << endr
      << 1 << 1 << 1 << 1  << endr
      << 0 << 0 << 0 << 2  << endr;

    D << 5 << endr;

    MatrixExt<int>
		AR1 = MatrixExt<int>::GenReversalMatrix(A.n_rows, 0),
        AR2 = MatrixExt<int>::GenReversalMatrix(A.n_cols, 0),
        BR1 = MatrixExt<int>::GenReversalMatrix(B.n_rows, 4),
        BR2 = MatrixExt<int>::GenReversalMatrix(B.n_cols, 1),
        CR1 = MatrixExt<int>::GenReversalMatrix(C.n_rows, 2),
        CR2 = MatrixExt<int>::GenReversalMatrix(C.n_cols, 3),
        DR1 = MatrixExt<int>::GenReversalMatrix(D.n_rows, 0),
        DR2 = MatrixExt<int>::GenReversalMatrix(D.n_cols, 0);

    MatrixExt<int>
		A_1(A),
        A_2(A),
        B_1(B),
        B_2(B),
        C_1(C),
        C_2(C),
        D_1(D),
        D_2(D);

    A_1.row(0) *= -1;
    A_2.col(0) *= -1;
    B_1.row(4) *= -1;
    B_2.col(1) *= -1;
    C_1.row(2) *= -1;
    C_2.col(3) *= -1;
    D_1.row(0) *= -1;
    D_2.col(0) *= -1;

    assert(CompareMatrices(AR1*A, A_1) && CompareMatrices(A*AR2, A_2));
    assert(CompareMatrices(BR1*B, B_1) && CompareMatrices(B*BR2, B_2));
    assert(CompareMatrices(CR1*C, C_1) && CompareMatrices(C*CR2, C_2));
    assert(CompareMatrices(DR1*D, D_1) && CompareMatrices(D*DR2, D_2));
}

void TestGenInterchangeMatrix()
{
    MatrixExt<int> A(3,4), B(5,2), C(4,4), D(1,1);
    A << -1 << 4 << 5 << 8 << endr
      << 3  << 0 << 1 << 0 << endr
      << 0  << 0 << 0 << 0 << endr;

    B << 2  << -3 << endr
      << 0  << 4  << endr
      << -4 << 0  << endr
      << 7  << 0  << endr
      << -4 << -4 << endr;

    C << 3 << 0 << 2 << -1 << endr
      << 0 << 0 << 0 << -2 << endr
      << 1 << 1 << 1 << 1  << endr
      << 0 << 0 << 0 << 2  << endr;

    D << 5 << endr;

    MatrixExt<int>
		AR1 = MatrixExt<int>::GenInterchangeMatrix(A.n_rows, 0, 2),
        AR2 = MatrixExt<int>::GenInterchangeMatrix(A.n_cols, 1, 3),
        BR1 = MatrixExt<int>::GenInterchangeMatrix(B.n_rows, 0, 4),
        BR2 = MatrixExt<int>::GenInterchangeMatrix(B.n_cols, 1, 1),
        CR1 = MatrixExt<int>::GenInterchangeMatrix(C.n_rows, 0, 3),
        CR2 = MatrixExt<int>::GenInterchangeMatrix(C.n_cols, 1, 2),
        DR1 = MatrixExt<int>::GenInterchangeMatrix(D.n_rows, 0, 0),
        DR2 = MatrixExt<int>::GenInterchangeMatrix(D.n_cols, 0, 0);

    MatrixExt<int>
		A_1(A),
		A_2(A),
		B_1(B),
		B_2(B),
		C_1(C),
		C_2(C),
		D_1(D),
		D_2(D);

    Row<int> r1(A.row(0));
    A_1.row(0) = A_1.row(2);
    A_1.row(2) = r1;

    Col<int> c1(A.col(1));
    A_2.col(1) = A_2.col(3);
    A_2.col(3) = c1;

    Row<int> r2(B.row(0));
    B_1.row(0) = B_1.row(4);
    B_1.row(4) = r2;

    Row<int> r3(C.row(0));
    C_1.row(0) = C_1.row(3);
    C_1.row(3) = r3;

    Col<int> c3(C.col(1));
    C_2.col(1) = C_2.col(2);
    C_2.col(2) = c3;

    assert(CompareMatrices(AR1*A, A_1) && CompareMatrices(A*AR2, A_2));
    assert(CompareMatrices(BR1*B, B_1) && CompareMatrices(B*BR2, B_2));
    assert(CompareMatrices(CR1*C, C_1) && CompareMatrices(C*CR2, C_2));
    assert(CompareMatrices(DR1*D, D_1) && CompareMatrices(D*DR2, D_2));
}

void TestGenSkewingMatrix()
{
    MatrixExt<int> A(3,4), B(5,2), C(4,4), D(1,1);
    A << -1 << 4 << 5 << 8 << endr
      << 3  << 0 << 1 << 0 << endr
      << 0  << 0 << 0 << 0 << endr;

    B << 2  << -3 << endr
      << 0  << 4  << endr
      << -4 << 0  << endr
      << 7  << 0  << endr
      << -4 << -4 << endr;

    C << 3 << 0 << 2 << -1 << endr
      << 0 << 0 << 0 << -2 << endr
      << 1 << 1 << 1 << 1  << endr
      << 0 << 0 << 0 << 2  << endr;

    D << 5 << endr;

    MatrixExt<int>
		AR1 = MatrixExt<int>::GenSkewingMatrix(A.n_rows, 0, 2, 2, true),
        AR2 = MatrixExt<int>::GenSkewingMatrix(A.n_cols, 2, 1, -1, false),
        BR1 = MatrixExt<int>::GenSkewingMatrix(B.n_rows, 4, 1, -3, true),
        BR2 = MatrixExt<int>::GenSkewingMatrix(B.n_cols, 0, 1, 10, false),
        CR1 = MatrixExt<int>::GenSkewingMatrix(C.n_rows, 2, 3, -2, true),
        CR2 = MatrixExt<int>::GenSkewingMatrix(C.n_cols, 3, 2, -1, false);


    MatrixExt<int>
		A_1(A),
		A_2(A),
		B_1(B),
		B_2(B),
		C_1(C),
		C_2(C);

    A_1.row(2) += A.row(0) * 2;
    A_2.col(1) += A.col(2) * (-1);
    B_1.row(1) += B.row(4) * (-3);
    B_2.col(1) += B.col(0) * 10;
    C_1.row(3) += C.row(2) * (-2);
    C_2.col(2) += C.col(3) * (-1);

    assert(CompareMatrices(AR1*A, A_1) && CompareMatrices(A*AR2, A_2));
    assert(CompareMatrices(BR1*B, B_1) && CompareMatrices(B*BR2, B_2));
    assert(CompareMatrices(CR1*C, C_1) && CompareMatrices(C*CR2, C_2));

    try
    {
        MatrixExt<int> DR1 = MatrixExt<int>::GenSkewingMatrix(D.n_rows, 0, 0, 4, true);
        assert(!"::GenSkewingMatrix operates on two distinct rows");
    }
    catch(std::logic_error&)
    {
    }
    catch(std::exception&)
    {
        assert(!"::GenSkewingMatrix should have thrown std::range_error");
    }

    try
    {
        MatrixExt<int> DR2 = MatrixExt<int>::GenSkewingMatrix(D.n_cols, 0, 0, -2, false);
        assert(!"::GenSkewingMatrix operates on two distinct rows");
    }
    catch(std::logic_error&)
    {
    }
    catch(std::exception&)
    {
        assert(!"::GenSkewingMatrix should have thrown std::range_error");
    }
}

void TestDoReversalOperation()
{
    MatrixExt<int> A(3,4), B(5,2), C(4,4), D(1,1);
    A << -1 << 4 << 5 << 8 << endr
      << 3  << 0 << 1 << 0 << endr
      << 0  << 0 << 0 << 0 << endr;

    B << 2  << -3 << endr
      << 0  << 4  << endr
      << -4 << 0  << endr
      << 7  << 0  << endr
      << -4 << -4 << endr;

    C << 3 << 0 << 2 << -1 << endr
      << 0 << 0 << 0 << -2 << endr
      << 1 << 1 << 1 << 1  << endr
      << 0 << 0 << 0 << 2  << endr;

    D << 5 << endr;

    MatrixExt<int> Aprim(A), Bprim(B), Cprim(C), Dprim(D);
    MatrixExt<int>
		A_1(A),
		A_2(A),
		B_1(B),
		B_2(B),
		C_1(C),
		C_2(C),
		D_1(D),
		D_2(D);

    A.DoReversalOperation(0, true);
    Aprim.DoReversalOperation(0, false);
    B.DoReversalOperation(4, true);
    Bprim.DoReversalOperation(1, false);
    C.DoReversalOperation(2, true);
    Cprim.DoReversalOperation(3, false);
    D.DoReversalOperation(0, true);
    Dprim.DoReversalOperation(0, false);

    A_1.row(0) *= -1;
    A_2.col(0) *= -1;
    B_1.row(4) *= -1;
    B_2.col(1) *= -1;
    C_1.row(2) *= -1;
    C_2.col(3) *= -1;
    D_1.row(0) *= -1;
    D_2.col(0) *= -1;

    assert(CompareMatrices(A, A_1) && CompareMatrices(Aprim, A_2));
    assert(CompareMatrices(B, B_1) && CompareMatrices(Bprim, B_2));
    assert(CompareMatrices(C, C_1) && CompareMatrices(Cprim, C_2));
    assert(CompareMatrices(D, D_1) && CompareMatrices(Dprim, D_2));
}

void TestDoInterchangeOperation()
{
    MatrixExt<int> A(3,4), B(5,2), C(4,4), D(1,1);
    A << -1 << 4 << 5 << 8 << endr
      << 3  << 0 << 1 << 0 << endr
      << 0  << 0 << 0 << 0 << endr;

    B << 2  << -3 << endr
      << 0  << 4  << endr
      << -4 << 0  << endr
      << 7  << 0  << endr
      << -4 << -4 << endr;

    C << 3 << 0 << 2 << -1 << endr
      << 0 << 0 << 0 << -2 << endr
      << 1 << 1 << 1 << 1  << endr
      << 0 << 0 << 0 << 2  << endr;

    D << 5 << endr;

    MatrixExt<int> Aprim(A), Bprim(B), Cprim(C), Dprim(D);
    MatrixExt<int>
		A_1(A),
        A_2(A),
        B_1(B),
        B_2(B),
        C_1(C),
        C_2(C),
        D_1(D),
        D_2(D);

    A.DoInterchangeOperation(0, 2, true);
    Aprim.DoInterchangeOperation(1, 3, false);
    B.DoInterchangeOperation(0, 4, true);
    Bprim.DoInterchangeOperation(1, 1, false);
    C.DoInterchangeOperation(0, 3, true);
    Cprim.DoInterchangeOperation(1, 2, false);
    D.DoInterchangeOperation(0, 0, true);
    Dprim.DoInterchangeOperation(0, 0, false);

    Row<int> r1(A_1.row(0));
    A_1.row(0) = A_1.row(2);
    A_1.row(2) = r1;

    Col<int> c1(A_2.col(1));
    A_2.col(1) = A_2.col(3);
    A_2.col(3) = c1;

    Row<int> r2(B_1.row(0));
    B_1.row(0) = B_1.row(4);
    B_1.row(4) = r2;

    Row<int> r3(C_1.row(0));
    C_1.row(0) = C_1.row(3);
    C_1.row(3) = r3;

    Col<int> c3(C_2.col(1));
    C_2.col(1) = C_2.col(2);
    C_2.col(2) = c3;

    assert(CompareMatrices(A, A_1) && CompareMatrices(Aprim, A_2));
    assert(CompareMatrices(B, B_1) && CompareMatrices(Bprim, B_2));
    assert(CompareMatrices(C, C_1) && CompareMatrices(Cprim, C_2));
    assert(CompareMatrices(D, D_1) && CompareMatrices(Dprim, D_2));
}

void TestDoSkewingOperation()
{
    MatrixExt<int> A(3,4), B(5,2), C(4,4), D(1,1);
    A << -1 << 4 << 5 << 8 << endr
      << 3  << 0 << 1 << 0 << endr
      << 0  << 0 << 0 << 0 << endr;

    B << 2  << -3 << endr
      << 0  << 4  << endr
      << -4 << 0  << endr
      << 7  << 0  << endr
      << -4 << -4 << endr;

    C << 3 << 0 << 2 << -1 << endr
      << 0 << 0 << 0 << -2 << endr
      << 1 << 1 << 1 << 1  << endr
      << 0 << 0 << 0 << 2  << endr;

    D << 5 << endr;

    MatrixExt<int> Aprim(A), Bprim(B), Cprim(C), Dprim(D), bla(A);
    MatrixExt<int>
		A_1(A),
        A_2(A),
        B_1(B),
        B_2(B),
        C_1(C),
        C_2(C);

    A.DoSkewingOperation(0, 2, 2, true);
    Aprim.DoSkewingOperation(2, 1, -1, false);
    B.DoSkewingOperation(4, 1, -3, true);
    Bprim.DoSkewingOperation(0, 1, 10, false);
    C.DoSkewingOperation(2, 3, -2, true);
    Cprim.DoSkewingOperation(3, 2, -1, false);

    A_1.row(2) += A_1.row(0) * 2;
    A_2.col(1) += A_2.col(2) * (-1);
    B_1.row(1) += B_1.row(4) * (-3);
    B_2.col(1) += B_2.col(0) * 10;
    C_1.row(3) += C_1.row(2) * (-2);
    C_2.col(2) += C_2.col(3) * (-1);

    assert(CompareMatrices(A, A_1) && CompareMatrices(Aprim, A_2));
    assert(CompareMatrices(B, B_1) && CompareMatrices(Bprim, B_2));
    assert(CompareMatrices(C, C_1) && CompareMatrices(Cprim, C_2));

    try
    {
        D.DoSkewingOperation(0, 0, 4, true);
        assert(!"::GenSkewingMatrix operates on two distinct rows");
    }
    catch(std::logic_error&)
    {
    }
    catch(std::exception&)
    {
        assert(!"::GenSkewingMatrix should have thrown std::range_error");
    }

    try
    {
        Dprim.DoSkewingOperation(0, 0, -2, false);
        assert(!"::GenSkewingMatrix operates on two distinct rows");
    }
    catch(std::logic_error&)
    {
    }
    catch(std::exception&)
    {
        assert(!"::GenSkewingMatrix should have thrown std::range_error");
    }
}

void TestMatrixEchelonReduction()
{
    MatrixExt<int> A(3,3), B(3,3), C(3,4), D(3,4), E(6,2), F(4,3), G(2,4), U(3,3), S(3,3);
    A << 4 << 4 << 1 << endr
      << 6 << 0 << 1 << endr
      << 4 << 3 << 2 << endr;

    B << 1 << 0  << 0  << endr
      << 2 << 2  << -1 << endr
      << 1 << -1 << 1  << endr;

    C << 3 << 1 << -2 << 4 << endr
      << 1 << 0 << 2  << 3 << endr
      << 2 << 1 << -1 << 1 << endr;

    D << 1 << -2 << 3 << -1 << endr
      << 2 << -1 << 2 << 2  << endr
      << 3 << 1  << 2 << 3  << endr;

    E << 1  << -4 << endr
      << -5 << 0  << endr
      << 1  << -6 << endr
      << 0  << 0  << endr
      << 8  << -1 << endr
      << -3 << 3  << endr;

    F << -1 << 3  << 0 << endr
      << 0  << -4 << 1 << endr
      << 0  << 0  << 9 << endr
      << 0  << 0  << 0 << endr;

    G << -1 << 4 << endr
      << 0  << 3 << endr
      << 0  << 0 << endr
      << 0  << 0 << endr;

    A.ReduceToEchelon(U, S);
    assert(CompareMatrices(U*A, S));

    B.ReduceToEchelon(U,S);
    assert(CompareMatrices(U*B, S));

    C.ReduceToEchelon(U,S);
    assert(CompareMatrices(U*C, S));

    D.ReduceToEchelon(U,S);
    assert(CompareMatrices(U*D, S));

    E.ReduceToEchelon(U,S);
    assert(CompareMatrices(U*E, S));

    uword rank_s = F.ReduceToEchelon(U,S);
    assert(CompareMatrices(U*F, S) && rank_s == 3);

    rank_s = G.ReduceToEchelon(U,S);
    assert(CompareMatrices(U*G, S) && rank_s == 2);
}

void TestMatrixEchelonReduction2()
{
    MatrixExt<int> A(3,3), B(3,3), C(3,4), D(3,4), V(3,3), S(3,3);
    A << 4 << 4 << 1 << endr
      << 6 << 0 << 1 << endr
      << 4 << 3 << 2 << endr;

    B << 1 << 0  << 0  << endr
      << 2 << 2  << -1 << endr
      << 1 << -1 << 1  << endr;

    C << 3 << 1 << -2 << 4 << endr
      << 1 << 0 << 2  << 3 << endr
      << 2 << 1 << -1 << 1 << endr;

    D << 1 << -2 << 3 << -1 << endr
      << 2 << -1 << 2 << 2  << endr
      << 3 << 1  << 2 << 3  << endr;

    A.ReduceToEchelon2(V, S);
    assert(CompareMatrices(A, V*S));

    B.ReduceToEchelon2(V,S);
    assert(CompareMatrices(B, V*S));

    C.ReduceToEchelon2(V,S);
    assert(CompareMatrices(C, V*S));

    D.ReduceToEchelon2(V,S);
    assert(CompareMatrices(D, V*S));
}

void TestMatrixDiagonalization()
{
    MatrixExt<int> A(3,2), B(3,3), C(3,4), E(3,4), U(3,3), V(2,2), D(3,3);
    A << 6  << 5  << endr
      << 4  << 2  << endr
      << 10 << -3 << endr;

    B << 1 << 0  << 0  << endr
      << 2 << 2  << -1 << endr
      << 1 << -1 << 1  << endr;

    C << 3 << 1 << -2 << 4 << endr
      << 1 << 0 << 2  << 3 << endr
      << 2 << 1 << -1 << 1 << endr;

    E << 1 << -2 << 3 << -1 << endr
      << 2 << -1 << 2 << 2  << endr
      << 3 << 1  << 2 << 3  << endr;

    A.ReduceToDiagonal(U, V, D);
    assert(CompareMatrices(U*A*V, D));

    B.ReduceToDiagonal(U, V, D);
    assert(CompareMatrices(U*B*V, D));

    C.ReduceToDiagonal(U, V, D);
    assert(CompareMatrices(U*C*V, D));

    E.ReduceToDiagonal(U, V, D);
    assert(CompareMatrices(U*E*V, D));
}
