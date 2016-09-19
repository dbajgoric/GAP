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

#include <iostream>
#include <algorithm>
#include "rational_number_test.h"
#include "matrix_test.h"
#include "row_col_vector_test.h"
#include "diophantine_test.h"
#include "fourier_elimination_test.h"
#include "dependence_analyser_test.h"
#include "unimodular_transform_test.h"

int main()
{
	/*RUN_RATIONAL_NUMBER_TESTS();
	RUN_MATRIX_EXT_TESTS();
	RUN_ROW_COL_VECTOR_TESTS();
	RUN_DIOPHANTINE_TESTS();
	RUN_FOURIER_ELIMINATION_TESTS();
	//RUN_DEPENDENCE_ANALYSER_TESTS();
	//RUN_UNIMODULAR_TRANSFORMATION_TESTS();

	std::cout << "================== All tests passed ==================\n";*/

	/*************************** ONLY FOR THESIS TESTS *****************************/
	//ThesisProgram1();
	//ThesisMatrixMult();
	ThesisMRI_Q_Computation();

    return 0;
}
