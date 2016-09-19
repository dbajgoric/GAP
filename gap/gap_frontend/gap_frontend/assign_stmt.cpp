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

#include "assign_stmt.h"
#include "array_subscript_serializer.h"
#include <clang/AST/Expr.h>
#include <iostream>
#include <exception>


namespace gap
{
	namespace
	{

		void ExprToArrSubs(
			clang::ASTContext& ast_ctx
			, AssignStmt& assign_stmt
			, clang::Expr* expr)
		{
			if (auto bin_op = clang::dyn_cast<clang::BinaryOperator>(expr))
			{
				ExprToArrSubs(ast_ctx, assign_stmt, bin_op->getLHS()->IgnoreCasts());
				ExprToArrSubs(ast_ctx, assign_stmt, bin_op->getRHS()->IgnoreCasts());
				return;
			}
			if (auto arr_sub_ptr = ArrSubscriptSerializer::Deserialize(ast_ctx, expr->IgnoreCasts()))
				assign_stmt.PushToRhs(*arr_sub_ptr.release());
		}

	} /// Anonymous namespace


	AssignStmt::AssignStmt(
		clang::ASTContext& ast_ctx
		, clang::BinaryOperator& bin_assign)

		: m_stmt(&bin_assign)
	{
		if (m_stmt->getOpcode() != clang::BO_Assign)
			throw std::runtime_error("stmt must be an assignment statement");

		m_lhs = ArrSubscriptSerializer::Deserialize(ast_ctx, bin_assign.getLHS());
		ExprToArrSubs(ast_ctx, *this, bin_assign.getRHS());
	}

	void AssignStmt::SetLhs(ArrSubscriptPtr lhs)
	{
		m_lhs = std::move(lhs);
	}

	void AssignStmt::PushToRhs(const ArraySubscript& arr_subscript)
	{
		m_rhs.push_back(arr_subscript);
	}

	void AssignStmt::PushToRhs(ArraySubscript&& arr_subscript)
	{
		m_rhs.push_back(std::move(arr_subscript));
	}

	clang::BinaryOperator* AssignStmt::GetStmt()
	{
		return m_stmt;
	}

	const clang::BinaryOperator* AssignStmt::GetStmt() const
	{
		return m_stmt;
	}

	const ArraySubscript* AssignStmt::GetLhs() const
	{
		return m_lhs.get();
	}

	const std::vector<ArraySubscript>& AssignStmt::GetRhs() const
	{
		return m_rhs;
	}

	ArraySubscript* AssignStmt::GetLhs()
	{
		return m_lhs.get();
	}

	std::vector<ArraySubscript>& AssignStmt::GetRhs()
	{
		return m_rhs;
	}

	void AssignStmt::Dump() const
	{
		if (m_lhs)
			m_lhs->Dump();
		else
			std::cout << "null";

		std::cout << " = ";
		if (m_rhs.empty())
		{
			std::cout << "null";
			return;
		}
		for (auto iter = m_rhs.begin(); iter != m_rhs.end(); ++iter)
		{
			iter->Dump();
			if (iter != std::prev(m_rhs.end()))
				std::cout << " op ";
		}
	}

} /// namespace gap