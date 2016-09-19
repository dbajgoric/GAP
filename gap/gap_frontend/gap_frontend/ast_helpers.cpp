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

#include "ast_helpers.h"
#include <clang/AST/ASTContext.h>
#include <algorithm>
#include <cassert>


namespace gap
{

	clang::FunctionDecl* CreateFunDecl(
		clang::ASTContext& ast_ctx
		, const std::string& fun_name
		, clang::QualType ret_type
		, const std::vector<std::pair<clang::QualType, std::string>>& args
		, const std::vector<clang::Attr*>& attrs)
	{
		std::vector<clang::QualType> arg_types;
		arg_types.reserve(args.size());
		std::transform(
			args.begin(), args.end(), std::back_inserter(arg_types),
			[](const std::pair<clang::QualType, std::string>& elem)
		{
			return elem.first;
		});

		auto fun_decl = clang::FunctionDecl::Create(
			ast_ctx
			, ast_ctx.getTranslationUnitDecl()
			, clang::SourceLocation()
			, clang::SourceLocation()
			, &ast_ctx.Idents.get(fun_name)
			, ast_ctx.getFunctionType(
				ret_type
				, llvm::ArrayRef<clang::QualType>(arg_types)
				, clang::FunctionProtoType::ExtProtoInfo())

			, nullptr
			, clang::SC_None);

		std::vector<clang::ParmVarDecl*> parm_decls;
		parm_decls.reserve(args.size());
		std::transform(
			args.begin(), args.end(), std::back_inserter(parm_decls),
			[&ast_ctx, &fun_decl](const std::pair<clang::QualType, std::string>& elem)
		{
			return
				clang::ParmVarDecl::Create(
					ast_ctx
					, fun_decl
					, clang::SourceLocation()
					, clang::SourceLocation()
					, &ast_ctx.Idents.get(elem.second)
					, elem.first
					, nullptr
					, clang::SC_None
					, nullptr);
		});

		fun_decl->setParams(parm_decls);
		for (auto attr : attrs)
			fun_decl->addAttr(attr);

		return fun_decl;
	}

	clang::FunctionDecl* CreateFunDeclNoParams(
		clang::ASTContext& ast_ctx
		, const std::string& fun_name
		, clang::QualType ret_type
		, const std::vector<clang::QualType>& param_types)
	{
		return
			clang::FunctionDecl::Create(
				ast_ctx
				, ast_ctx.getTranslationUnitDecl()
				, clang::SourceLocation()
				, clang::SourceLocation()
				, &ast_ctx.Idents.get(fun_name)
				, ast_ctx.getFunctionType(
					ret_type
					, llvm::ArrayRef<clang::QualType>(param_types)
					, clang::FunctionProtoType::ExtProtoInfo())

				, nullptr
				, clang::SC_None);
	}

	clang::ParmVarDecl* CreateParmVarDecl(
		clang::ASTContext& ast_ctx
		, clang::FunctionDecl* fun_decl
		, const std::string& param_name
		, clang::QualType param_type)
	{
		return
			clang::ParmVarDecl::Create(
				ast_ctx
				, fun_decl
				, clang::SourceLocation()
				, clang::SourceLocation()
				, &ast_ctx.Idents.get(param_name)
				, param_type
				, nullptr
				, clang::SC_None
				, nullptr);
	}

	clang::FieldDecl* CreateFieldDecl(
		clang::ASTContext& ast_ctx
		, clang::DeclContext* decl_ctx
		, const std::string& field_name
		, clang::QualType field_type
		, clang::InClassInitStyle init_style)
	{
		return clang::FieldDecl::Create(
			ast_ctx
			, decl_ctx
			, clang::SourceLocation()
			, clang::SourceLocation()
			, &ast_ctx.Idents.get(field_name)
			, field_type
			, ast_ctx.getTrivialTypeSourceInfo(field_type)
			, nullptr
			, false
			, init_style);
	}

	clang::RecordDecl* CreateRecordDecl(
		clang::ASTContext& ast_ctx
		, clang::TagTypeKind tag_kind
		, const std::string& record_name
		, const std::unordered_map<std::string, clang::QualType>& fields_map)
	{
		clang::RecordDecl* record_decl = clang::RecordDecl::Create(
			ast_ctx
			, tag_kind
			, ast_ctx.getTranslationUnitDecl()
			, clang::SourceLocation()
			, clang::SourceLocation()
			, &ast_ctx.Idents.get(record_name));

		for (auto field : fields_map)
			record_decl->addDecl(CreateFieldDecl(
				ast_ctx
				, record_decl
				, field.first
				, field.second));

		record_decl->completeDefinition();
		return record_decl;
	}

	clang::MemberExpr* CreateMemberExpr(
		clang::ASTContext& ast_ctx
		, clang::Expr* base_expr
		, bool is_arrow
		, clang::ValueDecl* member_decl
		, clang::ExprValueKind val_kind
		, clang::ExprObjectKind obj_kind)
	{
		return
			clang::MemberExpr::Create(
				ast_ctx
				, base_expr
				, is_arrow
				, clang::SourceLocation()
				, clang::NestedNameSpecifierLoc()
				, clang::SourceLocation()
				, member_decl
				, clang::DeclAccessPair()
				, clang::DeclarationNameInfo(member_decl->getDeclName(), clang::SourceLocation())
				, nullptr
				, member_decl->getType()
				, val_kind
				, obj_kind);
	}

	clang::ArraySubscriptExpr* CreateArrSubHelper(
		clang::ASTContext& ast_ctx
		, clang::Expr* lhs
		, clang::Expr* rhs
		, clang::QualType res_type = clang::QualType()
		, clang::ExprValueKind val_kind = clang::VK_RValue
		, clang::ExprObjectKind obj_kind = clang::OK_Ordinary)
	{
		return ClangAllocator(ast_ctx).Alloc<clang::ArraySubscriptExpr>(
			lhs
			, rhs
			, res_type
			, val_kind
			, obj_kind
			, clang::SourceLocation());
	}

	clang::ArraySubscriptExpr* CreateArrSubExpr(
		clang::ASTContext& ast_ctx
		, clang::VarDecl* array_decl
		, const std::vector<clang::Expr*>& subscript_exprs
		, clang::QualType res_type
		, clang::ExprValueKind val_kind
		, clang::ExprObjectKind obj_kind)
	{
		assert(subscript_exprs.size() > 0 && "there has to be at least one sub expr");
		clang::ArraySubscriptExpr* sub_exrp = CreateArrSubHelper(
			ast_ctx
			, CreateDeclRefExpr(ast_ctx, array_decl, array_decl->getType())
			, subscript_exprs.front());

		for (std::size_t i = 1; i < subscript_exprs.size(); ++i)
			sub_exrp = CreateArrSubHelper(
				ast_ctx
				, sub_exrp
				, subscript_exprs[i]);

		sub_exrp->setValueKind(val_kind);
		sub_exrp->setType(res_type);
		sub_exrp->setObjectKind(obj_kind);
		return sub_exrp;
	}

	clang::VarDecl* CreateVarDecl(
		clang::ASTContext& ast_ctx
		, const std::string& var_name
		, clang::QualType var_type
		, clang::Expr* init_expr
		, clang::StorageClass store_class)
	{
		clang::VarDecl* var_decl = clang::VarDecl::Create(
			ast_ctx
			, ast_ctx.getTranslationUnitDecl()
			, clang::SourceLocation()
			, clang::SourceLocation()
			, &ast_ctx.Idents.get(var_name)
			, var_type
			, ast_ctx.getTrivialTypeSourceInfo(var_type)
			, store_class);

		if (init_expr != nullptr)
			var_decl->setInit(init_expr);

		return var_decl;
	}

	clang::EnumDecl* CreateEnumDecl(
		clang::ASTContext& ast_ctx
		, const std::string& enum_name
		, clang::QualType enum_type
		, const std::vector<std::pair<std::string, llvm::APSInt>>& enum_constants)
	{
		clang::EnumDecl* enum_decl = clang::EnumDecl::Create(
			ast_ctx
			, ast_ctx.getTranslationUnitDecl()
			, clang::SourceLocation()
			, clang::SourceLocation()
			, &ast_ctx.Idents.get(enum_name)
			, nullptr
			, false
			, false
			, true);

		for (auto & name_val_pair : enum_constants)
			enum_decl->addDecl(
				CreateEnumConstDecl(
					ast_ctx
					, enum_decl
					, name_val_pair.first
					, enum_type
					, name_val_pair.second));

		enum_decl->completeDefinition(enum_type, clang::QualType(), 0, 0);
		return enum_decl;
	}

	clang::EnumConstantDecl* CreateEnumConstDecl(
		clang::ASTContext& ast_ctx
		, clang::EnumDecl* enum_decl
		, const std::string& enum_const_name
		, clang::QualType enum_type
		, const llvm::APSInt& value)
	{
		return clang::EnumConstantDecl::Create(
			ast_ctx
			, enum_decl
			, clang::SourceLocation()
			, &ast_ctx.Idents.get(enum_const_name)
			, enum_type
			, nullptr
			, value);
	}

	clang::DeclRefExpr* CreateDeclRefExpr(
		clang::ASTContext& ast_ctx
		, clang::ValueDecl* val_decl
		, clang::QualType eval_type
		, clang::NamedDecl* found_decl
		, clang::ExprValueKind expr_val_kind
		, bool refers_to_encl_var_or_capture)
	{
		return
			clang::DeclRefExpr::Create(
				ast_ctx
				, clang::NestedNameSpecifierLoc()
				, clang::SourceLocation()
				, val_decl
				, refers_to_encl_var_or_capture
				, clang::SourceLocation()
				, eval_type
				, expr_val_kind
				, found_decl);
	}

	clang::CallExpr* CreateCallExpr(
		clang::ASTContext& ast_ctx
		, clang::Expr* fn_expr
		, const std::vector<clang::Expr*>& args
		, clang::QualType res_type
		, clang::ExprValueKind expr_val_kind)
	{
		return ClangAllocator(ast_ctx).Alloc<clang::CallExpr>(
			ast_ctx
			, fn_expr
			, llvm::ArrayRef<clang::Expr*>(args)
			, res_type
			, expr_val_kind
			, clang::SourceLocation());
	}

	clang::CUDAKernelCallExpr* CreateCUDAKernelCallExpr(
		clang::ASTContext& ast_ctx
		, clang::Expr* fn_expr
		, const std::vector<clang::Expr*>& kernel_config
		, const std::vector<clang::Expr*>& args
		, clang::QualType res_type
		, clang::ExprValueKind expr_val_kind)
	{
		/// For some reason CUDAKernelCallExpr expects CallExpr* for kernel config, although
		/// only its arguments are used to form <<<expr, expr>>> when printing
		return
			ClangAllocator(ast_ctx).Alloc<clang::CUDAKernelCallExpr>(
				ast_ctx
				, fn_expr
				, CreateCallExpr(
					ast_ctx
					, fn_expr
					, kernel_config
					, res_type)

				, args
				, res_type
				, expr_val_kind
				, clang::SourceLocation());
	}

	clang::UnaryOperator* CreateUnaryOp(
		clang::ASTContext& ast_ctx
		, clang::Expr* operand
		, clang::UnaryOperatorKind op_kind
		, clang::QualType res_type
		, clang::ExprValueKind expr_val_kind
		, clang::ExprObjectKind expr_obj_kind)
	{
		return ClangAllocator(ast_ctx).Alloc<clang::UnaryOperator>(
			operand
			, op_kind
			, res_type
			, expr_val_kind
			, expr_obj_kind
			, clang::SourceLocation());
	}

	clang::BinaryOperator* CreateBinOp(
		clang::ASTContext& ast_ctx
		, clang::Expr *lhs
		, clang::Expr *rhs
		, clang::BinaryOperatorKind opcode
		, clang::QualType res_type
		, clang::ExprValueKind expr_val_kind)
	{
		return ClangAllocator(ast_ctx).Alloc<clang::BinaryOperator>(
			lhs
			, rhs
			, opcode
			, res_type
			, expr_val_kind
			, clang::OK_Ordinary
			, clang::SourceLocation()
			, false);
	}

	clang::IntegerLiteral* CreateIntLiteral(
		clang::ASTContext& ast_ctx
		, const llvm::APInt& value
		, clang::QualType type)
	{
		return clang::IntegerLiteral::Create(
			ast_ctx
			, value
			, type
			, clang::SourceLocation());
	}

	clang::FloatingLiteral* CreateFloatLiteral(
		clang::ASTContext& ast_ctx
		, const llvm::APFloat& value
		, clang::QualType type
		, bool is_exact)
	{
		return clang::FloatingLiteral::Create(
			ast_ctx
			, value
			, is_exact
			, type
			, clang::SourceLocation());
	}

	clang::CStyleCastExpr* CreateCStyleCastExpr(
		clang::ASTContext& ast_ctx
		, clang::QualType target_type
		, clang::CastKind cast_kind
		, clang::Expr* op
		, clang::ExprValueKind expr_val_kind)
	{
		return clang::CStyleCastExpr::Create(
			ast_ctx
			, target_type
			, expr_val_kind
			, cast_kind
			, op
			, nullptr
			, ast_ctx.getTrivialTypeSourceInfo(target_type)
			, clang::SourceLocation()
			, clang::SourceLocation());
	}

	clang::CompoundStmt* CreateCmpndStmt(
		clang::ASTContext& ast_ctx
		, const std::vector<clang::Stmt*>& stmts)
	{
		return
			ClangAllocator(ast_ctx).Alloc<clang::CompoundStmt>(
				ast_ctx
				, stmts
				, clang::SourceLocation()
				, clang::SourceLocation());
	}

	clang::CompoundLiteralExpr* CreateCmpndLiteralExpr(
		clang::ASTContext& ast_ctx
		, clang::QualType type
		, clang::Expr* init_expr
		, clang::ExprValueKind expr_value_kind)
	{
		return
			ClangAllocator(ast_ctx).Alloc<clang::CompoundLiteralExpr>(
				clang::SourceLocation()
				, ast_ctx.getTrivialTypeSourceInfo(type)
				, type
				, expr_value_kind
				, init_expr
				, false);
	}

	clang::QualType CreateIncompleteArrType(
		clang::ASTContext& ast_ctx
		, clang::QualType elem_type
		, clang::ArrayType::ArraySizeModifier arr_size_mod)
	{
		return ast_ctx.getIncompleteArrayType(
			elem_type
			, arr_size_mod
			, 0);
	}

	clang::InitListExpr* CreateInitListExpr(
		clang::ASTContext& ast_ctx
		, const std::vector<clang::Expr*>& init_exprs)
	{
		return ClangAllocator(ast_ctx).Alloc<clang::InitListExpr>(
			ast_ctx
			, clang::SourceLocation()
			, init_exprs
			, clang::SourceLocation());
	}

	clang::ParenExpr* CreateParenExpr(
		clang::ASTContext& ast_ctx
		, clang::Expr* expr)
	{
		return ClangAllocator(ast_ctx).Alloc<clang::ParenExpr>(
			clang::SourceLocation()
			, clang::SourceLocation()
			, expr);
	}

	clang::IfStmt* CreateIfStmt(
		clang::ASTContext& ast_ctx
		, clang::Expr* cond_expr
		, clang::Stmt* then_stmt
		, clang::Stmt* else_stmt)
	{
		return
			ClangAllocator(ast_ctx).Alloc<clang::IfStmt>(
				ast_ctx
				, clang::SourceLocation()
				, nullptr
				, cond_expr
				, then_stmt
				, clang::SourceLocation()
				, else_stmt);
	}

	clang::ReturnStmt* CreateReturnStmt(
		clang::ASTContext& ast_ctx
		, clang::Expr* expr)
	{
		return
			ClangAllocator(ast_ctx).Alloc<clang::ReturnStmt>(
				clang::SourceLocation()
				, expr
				, nullptr);
	}

	clang::UnaryExprOrTypeTraitExpr* CreateUnaryOrTypeTraitExpr(
		clang::ASTContext& ast_ctx
		, clang::UnaryExprOrTypeTrait expr_kind
		, clang::TypeSourceInfo* type_src_info
		, clang::QualType res_type)
	{
		return ClangAllocator(ast_ctx).Alloc<clang::UnaryExprOrTypeTraitExpr>(
			expr_kind
			, type_src_info
			, res_type
			, clang::SourceLocation()
			, clang::SourceLocation());
	}

	clang::UnaryExprOrTypeTraitExpr* CreateUnaryOrTypeTraitExpr(
		clang::ASTContext& ast_ctx
		, clang::UnaryExprOrTypeTrait expr_kind
		, clang::Expr* expr
		, clang::QualType res_type)
	{
		return ClangAllocator(ast_ctx).Alloc<clang::UnaryExprOrTypeTraitExpr>(
			expr_kind
			, expr
			, res_type
			, clang::SourceLocation()
			, clang::SourceLocation());
	}

	clang::ForStmt* CreateForStmt(
		clang::ASTContext& ast_ctx
		, clang::Stmt* init_stmt
		, clang::Expr* cond_expr
		, clang::Expr* inc_expr
		, clang::Stmt* body_stmt)
	{
		return ClangAllocator(ast_ctx).Alloc<clang::ForStmt>(
			ast_ctx
			, init_stmt
			, cond_expr
			, nullptr
			, inc_expr
			, body_stmt
			, clang::SourceLocation()
			, clang::SourceLocation()
			, clang::SourceLocation());
	}

	const clang::VarDecl* GetVarDecl(const clang::DeclRefExpr* decl_ref_expr)
	{
		const clang::ValueDecl* val_decl = decl_ref_expr->getDecl();
		return clang::dyn_cast<clang::VarDecl>(val_decl);
	}

	clang::VarDecl* GetVarDecl(clang::DeclRefExpr* decl_ref_expr)
	{
		return const_cast<clang::VarDecl*>(
			GetVarDecl(static_cast<const clang::DeclRefExpr*>(decl_ref_expr)));
	}

	llvm::APSInt GetSigned(unsigned int bit_width, std::uint64_t val)
	{
		return llvm::APSInt(llvm::APInt(bit_width, val, true), false);
	}

	llvm::APSInt GetUnsigned(unsigned int bit_width, std::uint64_t val)
	{
		return llvm::APSInt(llvm::APInt(bit_width, val, false), true);
	}

	llvm::APSInt GetInt(unsigned int bit_width, std::uint64_t val, bool is_signed)
	{
		return
			is_signed
			? GetSigned(bit_width, val)
			: GetUnsigned(bit_width, val);
	}

	void AddAssign(llvm::APSInt& lhs, const llvm::APSInt& rhs)
	{
		if (lhs.getBitWidth() == rhs.getBitWidth())
			lhs += rhs;
		else if (lhs.getBitWidth() < rhs.getBitWidth())
			lhs = GetInt(rhs.getBitWidth(), lhs.getZExtValue(), lhs.isSigned()) + rhs;
		else
			lhs += GetInt(lhs.getBitWidth(), rhs.getZExtValue(), rhs.isSigned());
	}

	bool IsIdenticalStmt(
		const clang::ASTContext &Ctx
		, const clang::Stmt *Stmt1
		, const clang::Stmt *Stmt2
		, bool IgnoreSideEffects)
	{
		if (!Stmt1 || !Stmt2)
			return !Stmt1 && !Stmt2;

		// If Stmt1 & Stmt2 are of different class then they are not
		// identical statements.
		if (Stmt1->getStmtClass() != Stmt2->getStmtClass())
			return false;

		const clang::Expr *Expr1 = clang::dyn_cast<clang::Expr>(Stmt1);
		const clang::Expr *Expr2 = clang::dyn_cast<clang::Expr>(Stmt2);

		if (Expr1 && Expr2)
		{
			// If Stmt1 has side effects then don't warn even if expressions
			// are identical.
			if (!IgnoreSideEffects && Expr1->HasSideEffects(Ctx))
				return false;
			// If either expression comes from a macro then don't warn even if
			// the expressions are identical.
			if ((Expr1->getExprLoc().isMacroID()) || (Expr2->getExprLoc().isMacroID()))
				return false;

			// If all children of two expressions are identical, return true.
			clang::Expr::const_child_iterator I1 = Expr1->child_begin();
			clang::Expr::const_child_iterator I2 = Expr2->child_begin();
			while (I1 != Expr1->child_end() && I2 != Expr2->child_end())
			{
				if (!*I1 || !*I2 || !IsIdenticalStmt(Ctx, *I1, *I2, IgnoreSideEffects))
					return false;

				++I1;
				++I2;
			}
			// If there are different number of children in the statements, return
			// false.
			if (I1 != Expr1->child_end())
				return false;
			if (I2 != Expr2->child_end())
				return false;
		}

		switch (Stmt1->getStmtClass())
		{
		default:
			return false;
		case clang::Stmt::CallExprClass:
		case clang::Stmt::ArraySubscriptExprClass:
		case clang::Stmt::OMPArraySectionExprClass:
		case clang::Stmt::ImplicitCastExprClass:
		case clang::Stmt::ParenExprClass:
		case clang::Stmt::BreakStmtClass:
		case clang::Stmt::ContinueStmtClass:
		case clang::Stmt::NullStmtClass:
			return true;
		case clang::Stmt::CStyleCastExprClass:
		{
			const clang::CStyleCastExpr* CastExpr1 = clang::cast<clang::CStyleCastExpr>(Stmt1);
			const clang::CStyleCastExpr* CastExpr2 = clang::cast<clang::CStyleCastExpr>(Stmt2);

			return CastExpr1->getTypeAsWritten() == CastExpr2->getTypeAsWritten();
		}
		case clang::Stmt::ReturnStmtClass:
		{
			const clang::ReturnStmt *ReturnStmt1 = clang::cast<clang::ReturnStmt>(Stmt1);
			const clang::ReturnStmt *ReturnStmt2 = clang::cast<clang::ReturnStmt>(Stmt2);

			return IsIdenticalStmt(Ctx, ReturnStmt1->getRetValue(),
				ReturnStmt2->getRetValue(), IgnoreSideEffects);
		}
		case clang::Stmt::ForStmtClass:
		{
			const clang::ForStmt *ForStmt1 = clang::cast<clang::ForStmt>(Stmt1);
			const clang::ForStmt *ForStmt2 = clang::cast<clang::ForStmt>(Stmt2);

			if (!IsIdenticalStmt(Ctx, ForStmt1->getInit(), ForStmt2->getInit(),
				IgnoreSideEffects))
				return false;
			if (!IsIdenticalStmt(Ctx, ForStmt1->getCond(), ForStmt2->getCond(),
				IgnoreSideEffects))
				return false;
			if (!IsIdenticalStmt(Ctx, ForStmt1->getInc(), ForStmt2->getInc(),
				IgnoreSideEffects))
				return false;
			if (!IsIdenticalStmt(Ctx, ForStmt1->getBody(), ForStmt2->getBody(),
				IgnoreSideEffects))
				return false;
			return true;
		}
		case clang::Stmt::DoStmtClass:
		{
			const clang::DoStmt *DStmt1 = clang::cast<clang::DoStmt>(Stmt1);
			const clang::DoStmt *DStmt2 = clang::cast<clang::DoStmt>(Stmt2);

			if (!IsIdenticalStmt(Ctx, DStmt1->getCond(), DStmt2->getCond(),
				IgnoreSideEffects))
				return false;
			if (!IsIdenticalStmt(Ctx, DStmt1->getBody(), DStmt2->getBody(),
				IgnoreSideEffects))
				return false;
			return true;
		}
		case clang::Stmt::WhileStmtClass:
		{
			const clang::WhileStmt *WStmt1 = clang::cast<clang::WhileStmt>(Stmt1);
			const clang::WhileStmt *WStmt2 = clang::cast<clang::WhileStmt>(Stmt2);

			if (!IsIdenticalStmt(Ctx, WStmt1->getCond(), WStmt2->getCond(),
				IgnoreSideEffects))
				return false;
			if (!IsIdenticalStmt(Ctx, WStmt1->getBody(), WStmt2->getBody(),
				IgnoreSideEffects))
				return false;
			return true;
		}
		case clang::Stmt::IfStmtClass:
		{
			const clang::IfStmt *IStmt1 = clang::cast<clang::IfStmt>(Stmt1);
			const clang::IfStmt *IStmt2 = clang::cast<clang::IfStmt>(Stmt2);

			if (!IsIdenticalStmt(Ctx, IStmt1->getCond(), IStmt2->getCond(),
				IgnoreSideEffects))
				return false;
			if (!IsIdenticalStmt(Ctx, IStmt1->getThen(), IStmt2->getThen(),
				IgnoreSideEffects))
				return false;
			if (!IsIdenticalStmt(Ctx, IStmt1->getElse(), IStmt2->getElse(),
				IgnoreSideEffects))
				return false;
			return true;
		}
		case clang::Stmt::CompoundStmtClass:
		{
			const clang::CompoundStmt *CompStmt1 = clang::cast<clang::CompoundStmt>(Stmt1);
			const clang::CompoundStmt *CompStmt2 = clang::cast<clang::CompoundStmt>(Stmt2);

			if (CompStmt1->size() != CompStmt2->size())
				return false;

			clang::CompoundStmt::const_body_iterator I1 = CompStmt1->body_begin();
			clang::CompoundStmt::const_body_iterator I2 = CompStmt2->body_begin();
			while (I1 != CompStmt1->body_end() && I2 != CompStmt2->body_end())
			{
				if (!IsIdenticalStmt(Ctx, *I1, *I2, IgnoreSideEffects))
					return false;
				++I1;
				++I2;
			}

			return true;
		}
		case clang::Stmt::CompoundAssignOperatorClass:
		case clang::Stmt::BinaryOperatorClass:
		{
			const clang::BinaryOperator *BinOp1 = clang::cast<clang::BinaryOperator>(Stmt1);
			const clang::BinaryOperator *BinOp2 = clang::cast<clang::BinaryOperator>(Stmt2);
			return BinOp1->getOpcode() == BinOp2->getOpcode();
		}
		case clang::Stmt::CharacterLiteralClass:
		{
			const clang::CharacterLiteral *CharLit1 = clang::cast<clang::CharacterLiteral>(Stmt1);
			const clang::CharacterLiteral *CharLit2 = clang::cast<clang::CharacterLiteral>(Stmt2);
			return CharLit1->getValue() == CharLit2->getValue();
		}
		case clang::Stmt::DeclRefExprClass:
		{
			const clang::DeclRefExpr *DeclRef1 = clang::cast<clang::DeclRefExpr>(Stmt1);
			const clang::DeclRefExpr *DeclRef2 = clang::cast<clang::DeclRefExpr>(Stmt2);
			return DeclRef1->getDecl() == DeclRef2->getDecl();
		}
		case clang::Stmt::IntegerLiteralClass:
		{
			const clang::IntegerLiteral *IntLit1 = clang::cast<clang::IntegerLiteral>(Stmt1);
			const clang::IntegerLiteral *IntLit2 = clang::cast<clang::IntegerLiteral>(Stmt2);

			llvm::APInt I1 = IntLit1->getValue();
			llvm::APInt I2 = IntLit2->getValue();
			if (I1.getBitWidth() != I2.getBitWidth())
				return false;
			return  I1 == I2;
		}
		case clang::Stmt::FloatingLiteralClass:
		{
			const clang::FloatingLiteral *FloatLit1 = clang::cast<clang::FloatingLiteral>(Stmt1);
			const clang::FloatingLiteral *FloatLit2 = clang::cast<clang::FloatingLiteral>(Stmt2);
			return FloatLit1->getValue().bitwiseIsEqual(FloatLit2->getValue());
		}
		case clang::Stmt::StringLiteralClass:
		{
			const clang::StringLiteral *StringLit1 = clang::cast<clang::StringLiteral>(Stmt1);
			const clang::StringLiteral *StringLit2 = clang::cast<clang::StringLiteral>(Stmt2);
			return StringLit1->getBytes() == StringLit2->getBytes();
		}
		case clang::Stmt::MemberExprClass:
		{
			const clang::MemberExpr *MemberStmt1 = clang::cast<clang::MemberExpr>(Stmt1);
			const clang::MemberExpr *MemberStmt2 = clang::cast<clang::MemberExpr>(Stmt2);
			return MemberStmt1->getMemberDecl() == MemberStmt2->getMemberDecl();
		}
		case clang::Stmt::UnaryOperatorClass:
		{
			const clang::UnaryOperator *UnaryOp1 = clang::cast<clang::UnaryOperator>(Stmt1);
			const clang::UnaryOperator *UnaryOp2 = clang::cast<clang::UnaryOperator>(Stmt2);
			return UnaryOp1->getOpcode() == UnaryOp2->getOpcode();
		}
		}
	}

} /// namespace gap