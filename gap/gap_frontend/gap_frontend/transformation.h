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

#ifndef GAP_FRONTEND_TRANSFORMATION_H
#define GAP_FRONTEND_TRANSFORMATION_H

#include <cstddef>

namespace gap
{

	/// Records the relevant info regarding the nest transformation such as the
	/// transformation type (inner or outer loop parallelization) and number of
	/// dependence free loops in the nest - which loops are these is clear from
	/// the context (i.e. if k is the number of loops without the loop carried
	/// dependences, these are k outer loops if outer loop parallelization was
	/// applied, k inner loops if inner parallelization was applied and entire
	/// nest of depth k if no transformation was applied)
	///
	/// FIXME: transformation matrix should also be part of this class
	class Transformation
	{
	public:

		enum TransformType
		{
			TRANSFORM_NONE
			, TRANSFORM_INNER_PAR
			, TRANSFORM_OUTER_PAR
		};

		Transformation()
			: m_transform_type(TRANSFORM_NONE)
			, m_dep_free_loops_cnt(0)
		{
		}

		Transformation(TransformType transform_type, std::size_t dep_free_loops_cnt)
			: m_transform_type(transform_type)
			, m_dep_free_loops_cnt(dep_free_loops_cnt)
		{
		}

		void SetTransformType(TransformType type)
		{
			m_transform_type = type;
		}

		TransformType GetTransformType() const
		{
			return m_transform_type;
		}

		std::size_t GetDepFreeLoopsCnt() const
		{
			return m_dep_free_loops_cnt;
		}

	private:

		TransformType m_transform_type;
		std::size_t m_dep_free_loops_cnt;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_TRANSFORMATION_H
