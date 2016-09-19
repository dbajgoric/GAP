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

#ifndef GAP_FRONTEND_ARRAY_DECL_INFO_H
#define GAP_FRONTEND_ARRAY_DECL_INFO_H

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include "identifier.h"

namespace gap
{

	/// Helper class that stores information regarding the particular nest
	/// array declaration, that is used throught code generation phase (for
	/// example, the corresponding device array decl and array identifier)
	class ArrayDeclInfo
	{
	public:

		ArrayDeclInfo(
			ArrayLikeIdentifier& array_id
			, clang::VarDecl& host_var
			, clang::VarDecl& device_var
			, clang::VarDecl* tmp_host_var
			, clang::Expr& flat_dim_expr)

			: m_array_id(array_id)
			, m_host_var(host_var)
			, m_device_var(device_var)
			, m_tmp_host_var(tmp_host_var)
			, m_flat_dim_expr(flat_dim_expr)
		{
		}

		ArrayLikeIdentifier& ArrayId() { return m_array_id; }
		clang::VarDecl& HostVar() { return m_host_var; }
		clang::VarDecl& DeviceVar() { return m_device_var; }
		clang::VarDecl* TmpHostVar() { return m_tmp_host_var; }
		clang::Expr& FlatDimExpr() { return m_flat_dim_expr; }

	private:

		ArrayLikeIdentifier& m_array_id;
		clang::VarDecl& m_host_var;
		clang::VarDecl& m_device_var;
		clang::VarDecl* m_tmp_host_var;
		clang::Expr&  m_flat_dim_expr;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_ARRAY_DECL_INFO_H