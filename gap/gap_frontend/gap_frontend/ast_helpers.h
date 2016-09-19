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

#ifndef GAP_FRONTEND_AST_HELPERS_H
#define GAP_FRONTEND_AST_HELPERS_H

#include <clang/AST/Expr.h>
#include <clang/AST/Decl.h>
#include <llvm/ADT/APSInt.h>
#include <clang/AST/ExprCXX.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include "clang_allocator.h"

namespace gap
{

	template<class RequestedType>
	const RequestedType* SearchForChildOfType(const clang::Stmt* stmt)
	{
		if (stmt == nullptr)
			return nullptr;
		if (clang::isa<RequestedType>(stmt))
			return clang::cast<RequestedType>(stmt);

		const RequestedType* requested_stmt = nullptr;
		for (auto stmt_child : stmt->children())
		{
			requested_stmt = SearchForChildOfType<RequestedType>(stmt_child);
			if (requested_stmt != nullptr)
				break;
		}
		return requested_stmt;
	}

	template<class RequestedType>
	RequestedType* SearchForChildOfType(clang::Stmt* stmt)
	{
		return const_cast<RequestedType*>(
			SearchForChildOfType<RequestedType>(static_cast<const clang::Stmt*>(stmt)));
	}

	/// Wraps the tedious task of instantiating a clang's FunctionDecl
	clang::FunctionDecl* CreateFunDecl(
		clang::ASTContext& ast_ctx
		, const std::string& fun_name
		, clang::QualType ret_type
		, const std::vector<std::pair<clang::QualType, std::string>>& args
		, const std::vector<clang::Attr*>& attrs = {});

	/// Similar to the function above but it doesn't set the parameters of the created
	/// function decl
	clang::FunctionDecl* CreateFunDeclNoParams(
		clang::ASTContext& ast_ctx
		, const std::string& fun_name
		, clang::QualType ret_type
		, const std::vector<clang::QualType>& param_types);

	clang::ParmVarDecl* CreateParmVarDecl(
		clang::ASTContext& ast_ctx
		, clang::FunctionDecl* fun_decl
		, const std::string& param_name
		, clang::QualType param_type);

	clang::FieldDecl* CreateFieldDecl(
		clang::ASTContext& ast_ctx
		, clang::DeclContext* decl_ctx
		, const std::string& field_name
		, clang::QualType field_type
		, clang::InClassInitStyle init_style = clang::ICIS_NoInit);

	clang::RecordDecl* CreateRecordDecl(
		clang::ASTContext& ast_ctx
		, clang::TagTypeKind tag_kind
		, const std::string& record_name
		, const std::unordered_map<std::string, clang::QualType>& fields_map);

	clang::MemberExpr* CreateMemberExpr(
		clang::ASTContext& ast_ctx
		, clang::Expr* base_expr
		, bool is_arrow
		, clang::ValueDecl* member_decl
		, clang::ExprValueKind val_kind = clang::VK_RValue
		, clang::ExprObjectKind obj_kind = clang::OK_Ordinary);

	clang::ArraySubscriptExpr* CreateArrSubExpr(
		clang::ASTContext& ast_ctx
		, clang::VarDecl* array_decl
		, const std::vector<clang::Expr*>& subscript_exprs
		, clang::QualType res_type = clang::QualType()
		, clang::ExprValueKind val_kind = clang::VK_RValue
		, clang::ExprObjectKind obj_kind = clang::OK_Ordinary);

	clang::VarDecl* CreateVarDecl(
		clang::ASTContext& ast_ctx
		, const std::string& var_name
		, clang::QualType var_type
		, clang::Expr* init_expr = nullptr
		, clang::StorageClass store_class = clang::SC_None);

	clang::EnumDecl* CreateEnumDecl(
		clang::ASTContext& ast_ctx
		, const std::string& enum_name
		, clang::QualType enum_type
		, const std::vector<std::pair<std::string, llvm::APSInt>>& enum_constants);

	clang::EnumConstantDecl* CreateEnumConstDecl(
		clang::ASTContext& ast_ctx
		, clang::EnumDecl* enum_decl
		, const std::string& enum_const_name
		, clang::QualType enum_type
		, const llvm::APSInt& value);

	clang::DeclRefExpr* CreateDeclRefExpr(
		clang::ASTContext& ast_ctx
		, clang::ValueDecl* val_decl
		, clang::QualType eval_type
		, clang::NamedDecl* found_decl = nullptr
		, clang::ExprValueKind expr_val_kind = clang::VK_RValue
		, bool refers_to_encl_var_or_capture = false);

	clang::CallExpr* CreateCallExpr(
		clang::ASTContext& ast_ctx
		, clang::Expr* fn_expr
		, const std::vector<clang::Expr*>& args
		, clang::QualType res_type
		, clang::ExprValueKind expr_val_kind = clang::VK_RValue);

	clang::CUDAKernelCallExpr* CreateCUDAKernelCallExpr(
		clang::ASTContext& ast_ctx
		, clang::Expr* fn_expr
		, const std::vector<clang::Expr*>& kernel_config
		, const std::vector<clang::Expr*>& args
		, clang::QualType res_type = clang::QualType()
		, clang::ExprValueKind expr_val_kind = clang::VK_RValue);

	clang::UnaryOperator* CreateUnaryOp(
		clang::ASTContext& ast_ctx
		, clang::Expr* operand
		, clang::UnaryOperatorKind op_kind
		, clang::QualType res_type
		, clang::ExprValueKind expr_val_kind = clang::VK_RValue
		, clang::ExprObjectKind expr_obj_kind = clang::OK_Ordinary);

	clang::BinaryOperator* CreateBinOp(
		clang::ASTContext& ast_ctx
		, clang::Expr *lhs
		, clang::Expr *rhs
		, clang::BinaryOperatorKind opcode
		, clang::QualType res_type
		, clang::ExprValueKind expr_val_kind = clang::VK_RValue);

	clang::IntegerLiteral* CreateIntLiteral(
		clang::ASTContext& ast_ctx
		, const llvm::APInt& value
		, clang::QualType type);

	clang::FloatingLiteral* CreateFloatLiteral(
		clang::ASTContext& ast_ctx
		, const llvm::APFloat& value
		, clang::QualType type
		, bool is_exact = false);

	clang::CStyleCastExpr* CreateCStyleCastExpr(
		clang::ASTContext& ast_ctx
		, clang::QualType target_type
		, clang::CastKind cast_kind
		, clang::Expr* op
		, clang::ExprValueKind expr_val_kind = clang::VK_RValue);

	clang::CompoundStmt* CreateCmpndStmt(
		clang::ASTContext& ast_ctx
		, const std::vector<clang::Stmt*>& stmts);

	clang::CompoundLiteralExpr* CreateCmpndLiteralExpr(
		clang::ASTContext& ast_ctx
		, clang::QualType type
		, clang::Expr* init_expr
		, clang::ExprValueKind expr_value_kind = clang::VK_RValue);

	clang::QualType CreateIncompleteArrType(
		clang::ASTContext& ast_ctx
		, clang::QualType elem_type
		, clang::ArrayType::ArraySizeModifier arr_size_mod = clang::ArrayType::Star);

	clang::InitListExpr* CreateInitListExpr(
		clang::ASTContext& ast_ctx
		, const std::vector<clang::Expr*>& init_exprs);

	clang::ParenExpr* CreateParenExpr(
		clang::ASTContext& ast_ctx
		, clang::Expr* expr);

	clang::IfStmt* CreateIfStmt(
		clang::ASTContext& ast_ctx
		, clang::Expr* cond_expr
		, clang::Stmt* then_stmt
		, clang::Stmt* else_stmt = nullptr);

	clang::ReturnStmt* CreateReturnStmt(
		clang::ASTContext& ast_ctx
		, clang::Expr* expr = nullptr);

	template<class DeclType>
	clang::DeclGroup* CreateDeclGroup(
		clang::ASTContext& ast_ctx
		, const std::vector<DeclType*>& decls)
	{
		std::unique_ptr<clang::Decl*[]> raw_decls(new clang::Decl*[decls.size()]);
		std::copy(decls.begin(), decls.end(), raw_decls.get());
		return clang::DeclGroup::Create(
			ast_ctx
			, raw_decls.release()
			, decls.size());
	}

	template<class DeclType>
	clang::DeclStmt* CreateDeclStmt(
		clang::ASTContext& ast_ctx
		, const std::vector<DeclType*>& decls)
	{
		assert(decls.size() > 0 && "there must be at least one decl");
		return ClangAllocator(ast_ctx).Alloc<clang::DeclStmt>(
			decls.size() == 1
			? clang::DeclGroupRef(decls[0])
			: clang::DeclGroupRef(CreateDeclGroup(ast_ctx, decls))

			, clang::SourceLocation()
			, clang::SourceLocation());
	}

	clang::UnaryExprOrTypeTraitExpr* CreateUnaryOrTypeTraitExpr(
		clang::ASTContext& ast_ctx
		, clang::UnaryExprOrTypeTrait expr_kind
		, clang::TypeSourceInfo* type_src_info
		, clang::QualType res_type);

	clang::UnaryExprOrTypeTraitExpr* CreateUnaryOrTypeTraitExpr(
		clang::ASTContext& ast_ctx
		, clang::UnaryExprOrTypeTrait expr_kind
		, clang::Expr* expr
		, clang::QualType res_type);

	clang::ForStmt* CreateForStmt(
		clang::ASTContext& ast_ctx
		, clang::Stmt* init_stmt
		, clang::Expr* cond_expr
		, clang::Expr* inc_expr
		, clang::Stmt* body_stmt);

	template<class DeclType>
	DeclType* NoLoadLookup(
		clang::ASTContext& ast_ctx
		, clang::DeclContext& decl_ctx
		, const std::string& id_name)
	{
		clang::DeclContextLookupResult res = decl_ctx.noload_lookup(
			clang::DeclarationName(&ast_ctx.Idents.get(id_name)));

		for (auto iter = res.begin(); iter != res.end(); ++iter)
			if (clang::isa<DeclType>(*iter))
				return clang::cast<DeclType>(*iter);

		return nullptr;
	}

	const clang::VarDecl* GetVarDecl(const clang::DeclRefExpr* decl_ref_expr);
	clang::VarDecl* GetVarDecl(clang::DeclRefExpr* decl_ref_expr);

	llvm::APSInt GetSigned(unsigned int bit_width, std::uint64_t val);
	llvm::APSInt GetUnsigned(unsigned int bit_width, std::uint64_t val);
	llvm::APSInt GetInt(unsigned int bit_width, std::uint64_t val, bool is_signed);
	void AddAssign(llvm::APSInt& lhs, const llvm::APSInt& rhs);

	/// This function has been entirely taken from the clangStaticAnalyzerCheckers/
	/// IdenticalExprChecker.cpp source. The following comment can be found near
	/// the function definition:
	///
	/// Determines whether two statement trees are identical regarding
	/// operators and symbols.
	///
	/// Exceptions: expressions containing macros or functions with possible side
	/// effects are never considered identical.
	/// Limitations: (t + u) and (u + t) are not considered identical.
	/// t*(u + t) and t*u + t*t are not considered identical.
	bool IsIdenticalStmt(
		const clang::ASTContext &Ctx
		, const clang::Stmt *Stmt1
		, const clang::Stmt *Stmt2
		, bool IgnoreSideEffects = true);

} /// namespace gap

#endif /// GAP_FRONTEND_AST_HELPERS_H
