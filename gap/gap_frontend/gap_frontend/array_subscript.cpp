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

#include "array_subscript.h"
#include <clang/AST/Decl.h>
#include <iostream>
#include <exception>
#include <cassert>


namespace gap
{

	ArraySubscript::ArraySubscript(
		clang::VarDecl* array_decl
		, clang::ArraySubscriptExpr* arr_sub_expr)

		: m_array_decl(array_decl)
		, m_arr_sub_expr(arr_sub_expr)
	{
		assert(m_array_decl != nullptr && "array_decl cannot be a nullptr");
	}

	ArraySubscript::ArraySubscript(
		clang::VarDecl* array_decl
		, const std::vector<LinearExpr>& lin_exprs
		, clang::ArraySubscriptExpr* arr_sub_expr)

		: m_array_decl(array_decl)
		, m_array_subscript(lin_exprs)
		, m_arr_sub_expr(arr_sub_expr)
	{
		assert(m_array_decl != nullptr && "array_decl cannot be a nullptr");
	}

	ArraySubscript::ArraySubscript(
		clang::VarDecl* array_decl
		, std::vector<LinearExpr>&& lin_exprs
		, clang::ArraySubscriptExpr* arr_sub_expr)

		: m_array_decl(array_decl)
		, m_array_subscript(std::move(lin_exprs))
		, m_arr_sub_expr(arr_sub_expr)
	{
		assert(m_array_decl != nullptr && "array_decl cannot be a nullptr");
	}

	void ArraySubscript::PushLinearExpr(const LinearExpr& lin_expr)
	{
		m_array_subscript.push_back(lin_expr);
	}

	void ArraySubscript::PushLinearExpr(LinearExpr&& lin_expr)
	{
		m_array_subscript.push_back(std::move(lin_expr));
	}

	void ArraySubscript::SetLinearExpr(const LinearExpr& lin_expr, std::size_t dim)
	{
		if (dim > m_array_subscript.size())
			throw std::runtime_error("dim exceeds the dimensionality of this array subscript");

		m_array_subscript[dim] = lin_expr;
	}

	void ArraySubscript::SetLinearExpr(LinearExpr&& lin_expr, std::size_t dim)
	{
		if (dim > m_array_subscript.size())
			throw std::runtime_error("dim exceeds the dimensionality of this array subscript");

		m_array_subscript[dim] = std::move(lin_expr);
	}

	const clang::VarDecl* ArraySubscript::GetArrDecl() const
	{
		return m_array_decl;
	}

	const LinearExpr& ArraySubscript::GetLinearExpr(std::size_t dim) const
	{
		if (dim > m_array_subscript.size())
			throw std::runtime_error("dim exceeds the dimensionality of this array subscript");

		return m_array_subscript[dim];
	}

	clang::VarDecl* ArraySubscript::GetArrDecl()
	{
		return m_array_decl;
	}

	LinearExpr& ArraySubscript::GetLinearExpr(std::size_t dim)
	{
		if (dim > m_array_subscript.size())
			throw std::runtime_error("dim exceeds the dimensionality of this array subscript");

		return m_array_subscript[dim];
	}

	std::size_t ArraySubscript::GetDimensionality() const
	{
		return m_array_subscript.size();
	}

	clang::ArraySubscriptExpr* ArraySubscript::GetArrSubExpr()
	{
		return m_arr_sub_expr;
	}

	void ArraySubscript::Dump() const
	{
		std::cout << m_array_decl->getName().str();
		for (auto & arr_idx : m_array_subscript)
		{
			std::cout << "[";
			arr_idx.Dump();
			std::cout << "]";
		}
	}

} /// namespace gap