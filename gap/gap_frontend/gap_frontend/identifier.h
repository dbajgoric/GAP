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

#ifndef GAP_FRONTEND_IDENTIFIER_H
#define GAP_FRONTEND_IDENTIFIER_H

#include <string>
#include <vector>
#include <clang/AST/Type.h>
#include <clang/AST/Expr.h>

namespace gap
{

	/// Base identifier class that stores the attributes owned by
	/// every identifier
	class Identifier
	{
	public:

		Identifier(const std::string& name, const clang::QualType& type);
		virtual ~Identifier() { }

		std::string GetName() const;
		clang::QualType GetType() const;
		virtual bool IsArrayLikeIdentifier() const;

	protected:

		std::string m_name;
		clang::QualType m_type;
	};

	/// Represents an array-like identifier (an array or a pointer)
	/// that may have the size attribute set
	class ArrayLikeIdentifier : public Identifier
	{
	public:

		ArrayLikeIdentifier(const std::string& name
			, const clang::QualType& type
			, const clang::QualType& base_elem_type
			, unsigned int dimensionality);

		ArrayLikeIdentifier(const std::string& name
			, const clang::QualType& type
			, const clang::QualType& base_elem_type
			, const std::vector<clang::Expr*>& exprs);

		ArrayLikeIdentifier(const std::string& name
			, const clang::QualType& type
			, const clang::QualType& base_elem_type
			, std::vector<clang::Expr*>&& exprs);

		virtual bool IsArrayLikeIdentifier() const;
		bool HasSizeForEachDim() const;
		void SetSize(unsigned int dim, clang::Expr* expr);
		void SetSizeForEachDim(const std::vector<clang::Expr*>& exprs);
		void SetSizeForEachDim(std::vector<clang::Expr*>&& exprs);
		const clang::Expr* GetSize(unsigned int dim) const;
		clang::Expr* GetSize(unsigned int dim);
		unsigned int GetDimensionality() const;
		void ResetSize(unsigned int dim_start);
		clang::QualType GetBaseElemType() const;

	private:

		/// The number of dimensions of this array-like structure
		const unsigned int m_dimensionality;

		/// The vector that will hold size expr for each array dimension
		std::vector<clang::Expr*> m_size_vec;
		clang::QualType m_base_elem_type;
	};

	ArrayLikeIdentifier* CastAsArrayId(Identifier* identifier);
	const ArrayLikeIdentifier* CastAsArrayId(const Identifier* identifier);

} /// namespace gap

#endif /// GAP_FRONTEND_IDENTIFIER_H
