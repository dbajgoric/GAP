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

#include "linear_expr.h"
#include "ast_helpers.h"
#include <clang/AST/Decl.h>
#include <iostream>
#include <exception>
#include <algorithm>


namespace gap
{
	namespace
	{

		std::string SignToStr(bool is_negative)
		{
			return is_negative ? "-" : "+";
		}

	} /// Anonymous namespace


	LinearExpr::LinearExpr(const MapType& vars_coeffs_map, const llvm::APSInt& constant)
		: m_vars_coeffs_map(vars_coeffs_map)
		, m_constant(std::make_unique<llvm::APSInt>(constant))
	{
		if (m_vars_coeffs_map.find(nullptr) != m_vars_coeffs_map.end())
			throw std::runtime_error("variable cannot be null in vars_coeffs_map");

		m_variables.reserve(m_vars_coeffs_map.size());
		for (auto var_int_pair : m_vars_coeffs_map)
			m_variables.push_back(var_int_pair.first);
	}

	LinearExpr::LinearExpr(MapType&& vars_coeffs_map, const llvm::APSInt& constant)
		: m_vars_coeffs_map(std::move(vars_coeffs_map))
		, m_constant(std::make_unique<llvm::APSInt>(constant))
	{
		if (m_vars_coeffs_map.find(nullptr) != m_vars_coeffs_map.end())
			throw std::runtime_error("variable cannot be null in vars_coeffs_map");

		m_variables.reserve(m_vars_coeffs_map.size());
		for (auto var_int_pair : m_vars_coeffs_map)
			m_variables.push_back(var_int_pair.first);
	}

	LinearExpr::LinearExpr(const LinearExpr& other)
		: m_vars_coeffs_map(other.m_vars_coeffs_map)
		, m_variables(other.m_variables)
		, m_constant(
			other.m_constant
			? new llvm::APSInt(*other.m_constant)
			: nullptr)
	{
	}

	LinearExpr::LinearExpr(LinearExpr&& other)
		: m_vars_coeffs_map(std::move(other.m_vars_coeffs_map))
		, m_variables(std::move(other.m_variables))
		, m_constant(std::move(other.m_constant))
	{
	}

	LinearExpr& LinearExpr::operator=(const LinearExpr& other)
	{
		if (this != &other)
		{
			m_vars_coeffs_map = other.m_vars_coeffs_map;
			m_variables = other.m_variables;
			m_constant =
				other.m_constant
				? std::make_unique<llvm::APSInt>(*other.m_constant)
				: nullptr;
		}
		return *this;
	}

	LinearExpr& LinearExpr::operator=(LinearExpr&& other)
	{
		if (this != &other)
		{
			m_vars_coeffs_map = std::move(other.m_vars_coeffs_map);
			m_variables = std::move(other.m_variables);
			m_constant = std::move(other.m_constant);
		}
		return *this;
	}

	bool LinearExpr::InsertVar(const clang::VarDecl* var, const llvm::APSInt& coeff_val)
	{
		auto result = m_vars_coeffs_map.insert(std::make_pair(var, coeff_val));
		if (result.second)
			m_variables.push_back(var);

		return result.second;
	}

	bool LinearExpr::AddToVarCoeff(const clang::VarDecl* var, const llvm::APSInt& value)
	{
		auto iter = m_vars_coeffs_map.find(var);
		if (iter == m_vars_coeffs_map.end())
			return false;

		AddAssign(iter->second, value);
		return true;
	}

	void LinearExpr::InsertOrAddIfKnownVar(const clang::VarDecl* var, const llvm::APSInt& coeff_val)
	{
		if (!InsertVar(var, coeff_val))
			AddToVarCoeff(var, coeff_val);
	}

	void LinearExpr::SetConstant(const llvm::APSInt& constant)
	{
		m_constant = std::make_unique<llvm::APSInt>(constant);
	}

	void LinearExpr::AddToConstant(const llvm::APSInt& constant)
	{
		if (m_constant)
			AddAssign(*m_constant, constant);
		else
			SetConstant(constant);
	}

	std::size_t LinearExpr::GetVarsCount() const
	{
		return m_variables.size();
	}

	const LinearExpr::VectorType& LinearExpr::GetVars() const
	{
		return m_variables;
	}

	std::pair<llvm::APSInt, bool> LinearExpr::GetVarCoeff(const clang::VarDecl* var) const
	{
		auto iter = m_vars_coeffs_map.find(var);
		if (iter == m_vars_coeffs_map.end())
			return{ llvm::APSInt(), false };

		return{ iter->second, true };
	}

	llvm::APSInt LinearExpr::GetConstant() const
	{
		return
			m_constant
			? *m_constant
			: llvm::APSInt::get(0);
	}

	void LinearExpr::Dump() const
	{
		for (auto iter = m_vars_coeffs_map.begin(); iter != m_vars_coeffs_map.end(); ++iter)
		{
			if (iter != m_vars_coeffs_map.begin())
				std::cout << SignToStr(iter->second.isNegative()) << " ";
			else if (iter->second.isNegative())
				std::cout << "-";

			std::cout
				<< iter->second.abs().getZExtValue()
				<< "*"
				<< iter->first->getName().str()
				<< " ";
		}

		llvm::APSInt constant = GetConstant();
		if (GetVarsCount() > 0)
		{
			std::cout
				<< SignToStr(constant.isNegative())
				<< " "
				<< constant.abs().getZExtValue();
		}
		else
		{
			if (constant.isNegative())
				std::cout << "-";

			std::cout << constant.abs().getZExtValue();
		}
	}

} /// namespace gap