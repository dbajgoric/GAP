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

#include "identifier.h"
#include <algorithm>
#include <exception>
#include <cassert>

namespace gap
{

	Identifier::Identifier(const std::string& name, const clang::QualType& type)
		: m_name(name)
		, m_type(type)
	{
	}

	std::string Identifier::GetName() const
	{
		return m_name;
	}

	clang::QualType Identifier::GetType() const
	{
		return m_type;
	}

	bool Identifier::IsArrayLikeIdentifier() const
	{
		return false;
	}

	ArrayLikeIdentifier::ArrayLikeIdentifier(const std::string& name
		, const clang::QualType& type
		, const clang::QualType& base_elem_type
		, unsigned int dimensionality)
		: Identifier(name, type)
		, m_dimensionality(dimensionality)
		, m_size_vec(dimensionality)
		, m_base_elem_type(base_elem_type)
	{
		if (!type->isArrayType() && !type->isPointerType())
			throw std::runtime_error(__FUNCTION__ "(): type must be an array or a pointer type");
		if (dimensionality == 0)
			throw std::runtime_error(__FUNCTION__ "(): array must have at least one dimension");

		m_size_vec.assign(m_size_vec.size(), nullptr);
	}

	ArrayLikeIdentifier::ArrayLikeIdentifier(const std::string& name
		, const clang::QualType& type
		, const clang::QualType& base_elem_type
		, const std::vector<clang::Expr*>& exprs)

		: ArrayLikeIdentifier(name, type, base_elem_type, exprs.size())
	{
		m_size_vec = exprs;
	}

	ArrayLikeIdentifier::ArrayLikeIdentifier(const std::string& name
		, const clang::QualType& type
		, const clang::QualType& base_elem_type
		, std::vector<clang::Expr*>&& exprs)

		: ArrayLikeIdentifier(name, type, base_elem_type, exprs.size())
	{
		m_size_vec = std::move(exprs);
	}

	bool ArrayLikeIdentifier::IsArrayLikeIdentifier() const
	{
		return true;
	}

	bool ArrayLikeIdentifier::HasSizeForEachDim() const
	{
		return std::find(m_size_vec.begin(), m_size_vec.end(), nullptr) == m_size_vec.end();
	}

	void ArrayLikeIdentifier::SetSize(unsigned int dim, clang::Expr* expr)
	{
		if (dim >= m_dimensionality)
			throw std::runtime_error(__FUNCTION__ "(): dim greater-or-equal to array dimensionality");

		m_size_vec[dim] = expr;
	}

	void ArrayLikeIdentifier::SetSizeForEachDim(const std::vector<clang::Expr*>& exprs)
	{
		if (exprs.size() != m_dimensionality)
			throw std::runtime_error("exprs.size() must be equal to m_dimensionality");

		m_size_vec = exprs;
	}

	void ArrayLikeIdentifier::SetSizeForEachDim(std::vector<clang::Expr*>&& exprs)
	{
		if (exprs.size() != m_dimensionality)
			throw std::runtime_error("exprs.size() must be equal to m_dimensionality");

		m_size_vec = std::move(exprs);
	}

	const clang::Expr* ArrayLikeIdentifier::GetSize(unsigned int dim) const
	{
		if (dim >= m_dimensionality)
			throw std::runtime_error(__FUNCTION__ "(): dim greater-or-equal than array dimensionality");

		return m_size_vec[dim];
	}

	clang::Expr* ArrayLikeIdentifier::GetSize(unsigned int dim)
	{
		if (dim >= m_dimensionality)
			throw std::runtime_error(__FUNCTION__ "(): dim greater-or-equal than array dimensionality");

		return m_size_vec[dim];
	}

	unsigned int ArrayLikeIdentifier::GetDimensionality() const
	{
		return m_dimensionality;
	}

	void ArrayLikeIdentifier::ResetSize(unsigned int dim_start)
	{
		if (dim_start >= m_dimensionality)
			throw std::runtime_error("dim greater-or-equal than array dimensionality");

		for (int i = dim_start; i < m_dimensionality; ++i)
			m_size_vec[i] = nullptr;
	}

	clang::QualType ArrayLikeIdentifier::GetBaseElemType() const
	{
		return m_base_elem_type;
	}

	ArrayLikeIdentifier* CastAsArrayId(Identifier* identifier)
	{
		return
			identifier->IsArrayLikeIdentifier()
			? static_cast<ArrayLikeIdentifier*>(identifier)
			: nullptr;
	}

	const ArrayLikeIdentifier* CastAsArrayId(const Identifier* identifier)
	{
		return
			identifier->IsArrayLikeIdentifier()
			? static_cast<const ArrayLikeIdentifier*>(identifier)
			: nullptr;
	}

} /// namespace gap