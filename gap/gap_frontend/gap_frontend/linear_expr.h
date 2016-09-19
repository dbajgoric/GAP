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

#ifndef GAP_FRONTEND_LINEAR_EXPR_H
#define GAP_FRONTEND_LINEAR_EXPR_H

#include <clang/AST/Stmt.h>
#include <llvm/ADT/APSInt.h>
#include <map>
#include <vector>
#include <memory>

namespace gap
{

	/// Represents a linear combination of form: a1*x1 + a2*x2 + ... + an*xn + a0.
	/// Used to represent lower and upper bounds of loops as well as array indices
	/// in array subscript expressions
	class LinearExpr
	{
		typedef std::map<const clang::VarDecl*, llvm::APSInt> MapType;
		typedef std::vector<const clang::VarDecl*> VectorType;

	public:

		LinearExpr() = default;
		LinearExpr(const MapType& vars_coeffs_map, const llvm::APSInt& constant);
		LinearExpr(MapType&& vars_coeffs_map, const llvm::APSInt& constant);

		LinearExpr(const LinearExpr& other);
		LinearExpr(LinearExpr&& other);
		LinearExpr& operator=(const LinearExpr& other);
		LinearExpr& operator=(LinearExpr&& other);

		/// Modifiers
		bool InsertVar(const clang::VarDecl* var, const llvm::APSInt& coeff_val);
		bool AddToVarCoeff(const clang::VarDecl* var, const llvm::APSInt& coeff_val);
		void InsertOrAddIfKnownVar(const clang::VarDecl* var, const llvm::APSInt& coeff_val);
		void SetConstant(const llvm::APSInt& constant);
		void AddToConstant(const llvm::APSInt& constant);

		/// Accessors
		std::size_t GetVarsCount() const;
		const VectorType& GetVars() const;
		std::pair<llvm::APSInt, bool> GetVarCoeff(const clang::VarDecl* var) const;
		llvm::APSInt GetConstant() const;

		/// Dump
		void Dump() const;

	private:

		MapType m_vars_coeffs_map;
		VectorType m_variables;
		std::unique_ptr<llvm::APSInt> m_constant;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_LINEAR_EXPR_H