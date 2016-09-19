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

#ifndef GAP_FRONTEND_NEST_TRANSFORMER_H
#define GAP_FRONTEND_NEST_TRANSFORMER_H

#include "mat_vec_pair.h"
#include "transformation.h"
#include <gap_transform/unimodular_transform.h>
#include <gap_util/types.h>
#include <vector>

namespace gap
{

	/// Forward declaration
	class DependenceMdl;

	/// Drives the data dependence and loop transformation process. It
	/// encapsulates the logic of best-fit transformation selection and
	/// produces a unimodular matrix representing the selected transform
	/// as well as updated lower and upper nest bounds. Note that lower
	/// and upper bounds are represented by rational matrix - row vector
	/// pair. Also note that some loops may have a multiple lower and
	/// upper bounds, in which case lower/upper bound is formed applying
	/// max/min function on these (in general case this cannot be done
	/// at compile time as inner loops' bounds can be a function of index
	/// variables of outer loops)
	///
	/// The transformer uses the following logic when choosing the loop
	/// transformation:
	/// 1) If outer loop parallelization is possible and transformed loop
	/// nest has 2 or more dependence free loops then this transformation
	/// is choosen
	/// 2) If outer loop parallelization is not possible or it produces only
	/// one dependence free loop then inner loop parallelization is selected
	///
	/// An error will be reported in case dependence is found in a single
	/// loop nest as it is impossible to parallelize such a nest by currenty
	/// supported transformations (transformations such as loop distribution
	/// can deal with this scenario)
	class NestTransformer
	{
	public:

		explicit NestTransformer(const DependenceMdl& dep_mdl);

		/// Accessors
		const util::IntMatrixType& GetTransformMat() const;
		const std::vector<util::Bound>& GetLowerBnd() const;
		const std::vector<util::Bound>& GetUpperBnd() const;
		const Transformation& GetTransformation() const;

		/// Dump
		void Dump() const;

	private:

		std::vector<util::IntRowVecType> m_distance_vecs;
		util::IntMatrixType m_transform_mat;
		std::vector<util::Bound> m_lower_bnd;
		std::vector<util::Bound> m_upper_bnd;
		Transformation m_transform;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_NEST_TRANSFORMER_H
