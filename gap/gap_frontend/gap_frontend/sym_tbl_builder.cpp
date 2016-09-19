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

#include "sym_tbl_builder.h"
#include "identifier.h"
#include "ast_helpers.h"
#include <exception>
#include <cassert>


namespace gap
{
	namespace
	{

		/// Used to track the loop nests within the function definition
		class LoopNestTracker
		{
		public:

			LoopNestTracker(clang::Stmt* stmt);
			~LoopNestTracker();

			static SymTblBuilder::LoopNestVector GetLoopNests();
			static void Reset();

		private:

			static unsigned int m_nest_cnt;
			static SymTblBuilder::LoopNestVector m_loop_nests;
		};

		unsigned int LoopNestTracker::m_nest_cnt = 0;
		SymTblBuilder::LoopNestVector LoopNestTracker::m_loop_nests;

		LoopNestTracker::LoopNestTracker(clang::Stmt* stmt)
		{
			assert(
				stmt != nullptr
				&& stmt->getStmtClass() == clang::Stmt::ForStmtClass
				&& "stmt must be a for stmt");

			if (m_nest_cnt++ == 0)
				m_loop_nests.push_back(clang::cast<clang::ForStmt>(stmt));
		}

		LoopNestTracker::~LoopNestTracker()
		{
			--m_nest_cnt;
		}

		SymTblBuilder::LoopNestVector LoopNestTracker::GetLoopNests()
		{
			return m_loop_nests;
		}

		void LoopNestTracker::Reset()
		{
			m_loop_nests.clear();
		}

	} /// Anonymous namespace

	SymTblBuilder::SymTblBuilder(clang::ASTContext& ast_context)
		: m_ast_context(ast_context)
		, m_symtbl_tree(ast_context)
		, m_allocator(ast_context)
	{
	}

	clang::ParenExpr* SymTblBuilder::MakeParenExpr(clang::Expr* expr)
	{
		return m_allocator.Alloc<clang::ParenExpr>(
			clang::SourceLocation()
			, clang::SourceLocation()
			, expr);
	}

	bool SymTblBuilder::IsControlFlowStmt(clang::Stmt* stmt)
	{
		auto stmt_class = stmt->getStmtClass();
		return
			stmt_class == clang::Stmt::IfStmtClass
			|| stmt_class == clang::Stmt::ForStmtClass
			|| stmt_class == clang::Stmt::WhileStmtClass
			|| stmt_class == clang::Stmt::DoStmtClass
			|| stmt_class == clang::Stmt::SwitchStmtClass;
	}

	void SymTblBuilder::CreateScope(clang::Stmt* stmt, bool is_fake_stmt)
	{
		m_symtbl_tree.Insert(
			m_fake_stmt_mngr.GetTopParStmt()
			, stmt
			, SymbolTable(stmt));

		m_fake_stmt_mngr.PushParStmt(stmt, is_fake_stmt);
	}

	bool SymTblBuilder::CreateScopeIf(clang::Stmt* stmt, TblCreationPredicate pred)
	{
		if (pred(stmt))
		{
			/// If stmt is not CmpndStmt, create a fake CmpndStmt representing the scope
			CreateScope(
				clang::isa<clang::CompoundStmt>(stmt)
				? stmt
				: m_allocator.Alloc<clang::CompoundStmt>(clang::Stmt::EmptyShell()));

			return true;
		}
		return false;
	}

	void SymTblBuilder::CreateDummyScope()
	{
		CreateScope(m_allocator.Alloc<clang::CompoundStmt>(clang::Stmt::EmptyShell()), true);
	}

	void SymTblBuilder::ResetLoopNestInfo()
	{
		LoopNestTracker::Reset();
		m_loop_nests.clear();
	}

	void SymTblBuilder::TraverseChildren(clang::Stmt* stmt)
	{
		assert(
			stmt != nullptr &&
			__FUNCTION__ "(): stmt mustn't be nullptr");

		for (auto child_stmt : stmt->children())
			RecursiveASTVisitorBase::TraverseStmt(child_stmt);
	}

	void SymTblBuilder::TraverseChildrenUntil(clang::Stmt* parent_stmt, clang::Stmt* end_stmt)
	{
		assert(
			parent_stmt != nullptr &&
			__FUNCTION__ "(): parents_stmt mustn't be nullptr");

		for (auto child_stmt : parent_stmt->children())
		{
			if (child_stmt == end_stmt)
				break;

			RecursiveASTVisitorBase::TraverseStmt(child_stmt);
		}
	}

	void SymTblBuilder::TraverseStmtOrChildrenIfCmpndStmt(clang::Stmt* stmt)
	{
		assert(
			stmt != nullptr &&
			__FUNCTION__ "(): stmt mustn't be nullptr");

		if (clang::isa<clang::CompoundStmt>(stmt))
			TraverseChildren(stmt);
		else
			RecursiveASTVisitorBase::TraverseStmt(stmt);
	}

	void SymTblBuilder::HandleIfStmt(clang::IfStmt* if_stmt)
	{
		/// Walk up from this if stmt
		RecursiveASTVisitorBase::WalkUpFromIfStmt(if_stmt);

		/// Traverse ifstmt cond. expr.
		auto then_stmt = if_stmt->getThen();
		TraverseChildrenUntil(if_stmt, then_stmt);

		/// Conditionally create scope for 'then' and traverse it
		bool scope_created = CreateScopeIf(
			then_stmt
			, [](clang::Stmt* stmt) { return !SymTblBuilder::IsControlFlowStmt(stmt); });

		TraverseStmtOrChildrenIfCmpndStmt(then_stmt);
		m_fake_stmt_mngr.ClearFakesTopPar();

		if (scope_created)
			m_fake_stmt_mngr.PopParStmt();
		else
			m_fake_stmt_mngr.ResetTopParCnt();

		auto else_stmt = if_stmt->getElse();
		if (else_stmt != nullptr)
		{
			/// There is an 'else' part
			if (clang::isa<clang::IfStmt>(else_stmt))
			{
				/// The 'else' part is actually an else-if stmt
				HandleIfStmt(clang::cast<clang::IfStmt>(else_stmt));
			}
			else
			{
				/// The 'else' part is an actual else stmt. Create scope for it and
				/// traverse it
				scope_created = CreateScopeIf(
					else_stmt
					, [](clang::Stmt* stmt) { return !SymTblBuilder::IsControlFlowStmt(stmt); });

				TraverseStmtOrChildrenIfCmpndStmt(else_stmt);
				m_fake_stmt_mngr.ClearFakesTopPar();

				if (scope_created)
					m_fake_stmt_mngr.PopParStmt();
				else
					m_fake_stmt_mngr.ResetTopParCnt();
			}
		}
	}

	bool SymTblBuilder::TraverseFunctionDecl(clang::FunctionDecl* fun_decl)
	{
		assert(
			m_symtbl_tree.Empty()
			&& "function decl must be the top level stmt");

		ResetLoopNestInfo();

		/// Interested only in function definitions
		if (!fun_decl->hasBody())
			return true;

		/// Walk up from this decl. (calls VisitFnDecl of the derived class)
		RecursiveASTVisitorBase::WalkUpFromFunctionDecl(fun_decl);

		/// Create function scope and parse function params
		auto body = fun_decl->getBody();
		assert(
			clang::isa<clang::CompoundStmt>(body) &&
			__FUNCTION__ "(): body is expected to be a compund stmt");

		CreateScope(body);
		for (auto param : fun_decl->params())
			RecursiveASTVisitorBase::TraverseVarDecl(param);

		/// Directly visit children of the function body
		TraverseChildren(clang::cast<clang::CompoundStmt>(body));

		m_fake_stmt_mngr.ClearFakesTopPar();
		m_fake_stmt_mngr.PopParStmt();

		/// Save the found loop nests to the local nest collection
		m_loop_nests = LoopNestTracker::GetLoopNests();

		return true;
	}

	bool SymTblBuilder::TraverseIfStmt(clang::IfStmt* if_stmt, DataRecursionQueue* queue)
	{
		assert(
			!m_symtbl_tree.Empty()
			&& "if stmt may not be the top level stmt");

		/// Create scope enclosing entire if-elseif-else block
		CreateScope(if_stmt);

		/// Handle the if stmt
		HandleIfStmt(if_stmt);

		m_fake_stmt_mngr.ClearFakesTopPar();
		m_fake_stmt_mngr.PopParStmt();
		CreateDummyScope();

		return true;
	}

	bool SymTblBuilder::TraverseForStmt(clang::ForStmt* for_stmt, DataRecursionQueue* queue)
	{
		assert(
			!m_symtbl_tree.Empty()
			&& "for stmt may not be the top level stmt");

		LoopNestTracker tracker(for_stmt);

		/// Create scope for the for stmt. and walk up from it
		CreateScope(for_stmt);
		RecursiveASTVisitorBase::WalkUpFromForStmt(for_stmt);

		/// Traverse for stmt header
		auto body = for_stmt->getBody();
		TraverseChildrenUntil(for_stmt, body);

		/// Traverse for stmt body
		TraverseStmtOrChildrenIfCmpndStmt(body);

		m_fake_stmt_mngr.ClearFakesTopPar();
		m_fake_stmt_mngr.PopParStmt();
		CreateDummyScope();

		return true;
	}

	bool SymTblBuilder::TraverseWhileStmt(clang::WhileStmt* while_stmt, DataRecursionQueue* queue)
	{
		assert(
			!m_symtbl_tree.Empty()
			&& "while stmt may not be the top level stmt");

		/// Create scope for the while stmt. and walk up from it
		CreateScope(while_stmt);
		RecursiveASTVisitorBase::WalkUpFromWhileStmt(while_stmt);

		/// Traverse while stmt header
		auto body = while_stmt->getBody();
		TraverseChildrenUntil(while_stmt, body);

		/// Traverse do stmt body
		TraverseStmtOrChildrenIfCmpndStmt(body);

		m_fake_stmt_mngr.ClearFakesTopPar();
		m_fake_stmt_mngr.PopParStmt();
		CreateDummyScope();

		return true;
	}

	bool SymTblBuilder::TraverseDoStmt(clang::DoStmt* do_stmt, DataRecursionQueue* queue)
	{
		assert(
			!m_symtbl_tree.Empty()
			&& "do stmt may not be the top level stmt");

		/// Create scope for the do stmt. and walk up from it
		CreateScope(do_stmt);
		RecursiveASTVisitorBase::WalkUpFromDoStmt(do_stmt);

		/// Traverse do stmt body
		auto body = do_stmt->getBody();
		TraverseStmtOrChildrenIfCmpndStmt(body);

		m_fake_stmt_mngr.ClearFakesTopPar();
		m_fake_stmt_mngr.PopParStmt();
		CreateDummyScope();

		return true;
	}

	bool SymTblBuilder::TraverseSwitchStmt(clang::SwitchStmt* switch_stmt, DataRecursionQueue* queue)
	{
		assert(
			!m_symtbl_tree.Empty()
			&& "switch stmt may not be the top level stmt");

		/// Create scope for the switch stmt. and walk up from it
		CreateScope(switch_stmt);
		RecursiveASTVisitorBase::WalkUpFromSwitchStmt(switch_stmt);

		/// Traverse switch stmt header
		auto body = switch_stmt->getBody();
		TraverseChildrenUntil(switch_stmt, body);

		/// Traverse switch stmt body
		TraverseStmtOrChildrenIfCmpndStmt(body);

		m_fake_stmt_mngr.ClearFakesTopPar();
		m_fake_stmt_mngr.PopParStmt();
		CreateDummyScope();

		return true;
	}

	bool SymTblBuilder::TraverseCompoundStmt(clang::CompoundStmt* cmpnd_stmt, DataRecursionQueue* queue)
	{
		assert(
			!m_symtbl_tree.Empty()
			&& "compound stmt may not be the top level stmt");

		/// Create scope for the compound stmt. and walk up from it
		CreateScope(cmpnd_stmt);
		RecursiveASTVisitorBase::WalkUpFromCompoundStmt(cmpnd_stmt);

		/// Traverse stms's children
		TraverseChildren(cmpnd_stmt);

		m_fake_stmt_mngr.ClearFakesTopPar();
		m_fake_stmt_mngr.PopParStmt();
		CreateDummyScope();

		return true;
	}

	SymTblTree& SymTblBuilder::GetSymTblTree()
	{
		return m_symtbl_tree;
	}

	SymTblBuilder::LoopNestVector& SymTblBuilder::GetLoopNests()
	{
		return m_loop_nests;
	}

	void SymTblBuilder::HandleConstantArrayDecl(clang::VarDecl* var_decl)
	{
		clang::QualType type = var_decl->getType();
		assert(type->isConstantArrayType() && "type must be a ConstantArrayType");
		std::vector<clang::Expr*> size_exprs;
		auto base_elem_type = type->getBaseElementTypeUnsafe();
		assert(base_elem_type != nullptr && "base_elem_type must not be nullptr");

		while (type->isConstantArrayType())
		{
			auto const_arr_type = clang::cast<clang::ConstantArrayType>(type);
			size_exprs.push_back(m_allocator.Alloc<clang::IntegerLiteral>(
				m_ast_context
				, const_arr_type->getSize()
				, m_ast_context.getIntTypeForBitwidth(const_arr_type->getSize().getBitWidth(), 0)
				, clang::SourceLocation()));

			type = const_arr_type->getElementType();
		}

		SymbolTable* symtbl = m_symtbl_tree.FindSymTable(m_fake_stmt_mngr.GetTopParStmt());
		assert(symtbl != nullptr && "tree search failed unexpectedly");
		symtbl->AddSymbol(
			var_decl->getName().str()
			, std::make_unique<ArrayLikeIdentifier>(
				var_decl->getName().str()
				, var_decl->getType()
				, clang::QualType(base_elem_type, 0)
				, std::move(size_exprs)));
	}

	clang::Expr* SymTblBuilder::GetAllocSizeExpr(clang::CallExpr* call_expr)
	{
		if (call_expr == nullptr)
			return nullptr;

		/// malloc and calloc allocation routines are supported
		clang::FunctionDecl* fun_decl = call_expr->getDirectCallee();
		assert(fun_decl != nullptr && "fun_decl mustn't be nullptr");
		if (fun_decl->getName() == "malloc")
		{
			if (fun_decl->getNumParams() != 1
				|| call_expr->getNumArgs() != 1)
				return nullptr;

			return MakeParenExpr(call_expr->getArg(0));
		}
		else if (fun_decl->getName() == "calloc")
		{
			if (fun_decl->getNumParams() != 2
				|| call_expr->getNumArgs() != 2)
				return nullptr;

			clang::ParenExpr* par_expr = MakeParenExpr(call_expr->getArg(0));

			clang::Expr* arg_1(call_expr->getArg(1));
			return MakeParenExpr(
				m_allocator.Alloc<clang::BinaryOperator>(
					par_expr
					, arg_1
					, clang::BO_Mul
					, arg_1->getType()
					, clang::VK_RValue
					, clang::OK_Ordinary
					, clang::SourceLocation()
					, false));
		}
		return nullptr;
	}

	void SymTblBuilder::SetSizeForDimension(
		clang::Stmt* stmt
		, ArrayLikeIdentifier& arr_ident
		, clang::QualType pointee_type
		, unsigned int dim)
	{
		assert(pointee_type.getTypePtrOrNull() != nullptr && "pointee_type must not be nullptr");
		clang::Expr* alloc_size_expr = GetAllocSizeExpr(
			SearchForChildOfType<clang::CallExpr>(stmt));

		if (alloc_size_expr != nullptr)
		{
			/// If memory was allocated with: malloc(40 * sizeof(int)), the size of the
			/// first dimension is set to expr: ((40 * sizeof(int)) / sizeof(int)). This
			/// way the number of allocated elements is stored instead of bytes (making
			/// it easier to create flat-arrays from multidimensional ones)
			clang::UnaryExprOrTypeTraitExpr* sizeof_expr = m_allocator.Alloc<clang::UnaryExprOrTypeTraitExpr>(
				clang::UETT_SizeOf
				, m_ast_context.CreateTypeSourceInfo(pointee_type)
				, m_ast_context.getSizeType()
				, clang::SourceLocation()
				, clang::SourceLocation());

			arr_ident.SetSize(
				dim
				, MakeParenExpr(
					m_allocator.Alloc<clang::BinaryOperator>(
						alloc_size_expr
						, sizeof_expr
						, clang::BO_Div
						, sizeof_expr->getType()
						, clang::VK_RValue
						, clang::OK_Ordinary
						, clang::SourceLocation()
						, false)));
		}
	}

	void SymTblBuilder::HandlePointerDecl(clang::VarDecl* var_decl)
	{
		clang::QualType type = var_decl->getType();
		assert(type->isPointerType() && "type must be a PointerType");
		auto ptr_type = clang::cast<clang::PointerType>(type);
		unsigned dimensionality(0);

		/// type holds base elem type after this loop
		while (type->isPointerType())
		{
			++dimensionality;
			type = type->getPointeeType();
		}

		auto identifier = std::make_unique<ArrayLikeIdentifier>(
			var_decl->getName().str()
			, var_decl->getType()
			, type
			, dimensionality);

		SetSizeForDimension(
			var_decl->getInit()
			, *identifier.get()
			, ptr_type->getPointeeType()
			, 0);

		SymbolTable* symtbl = m_symtbl_tree.FindSymTable(m_fake_stmt_mngr.GetTopParStmt());
		assert(symtbl != nullptr && "tree search failed unexpectedly");
		symtbl->AddSymbol(
			var_decl->getName().str()
			, std::move(identifier));
	}

	void SymTblBuilder::HandleOtherDecl(clang::VarDecl* var_decl)
	{
		SymbolTable* symtbl = m_symtbl_tree.FindSymTable(m_fake_stmt_mngr.GetTopParStmt());
		assert(symtbl != nullptr && "tree search failed unexpectedly");
		symtbl->AddSymbol(
			var_decl->getName().str()
			, std::make_unique<Identifier>(
				var_decl->getName().str()
				, var_decl->getType()));
	}

	bool SymTblBuilder::TraverseVarDecl(clang::VarDecl* var_decl)
	{
		clang::QualType type = var_decl->getType();
		if (type->isConstantArrayType())
			HandleConstantArrayDecl(var_decl);
		else if (type->isPointerType())
			HandlePointerDecl(var_decl);
		else
			HandleOtherDecl(var_decl);

		return true;
	}

	bool SymTblBuilder::TraverseBinAssign(clang::BinaryOperator* bin_op, DataRecursionQueue* queue)
	{
		auto lhs = bin_op->getLHS();
		if (!lhs->getType()->isPointerType())
			return true;

		auto declref_expr = SearchForChildOfType<clang::DeclRefExpr>(lhs);
		Identifier* ident = m_symtbl_tree.FindIdentifier(m_fake_stmt_mngr.GetTopParStmt(), declref_expr->getNameInfo().getAsString());
		assert(ident != nullptr && "unknown identifier encountered");
		assert(ident->IsArrayLikeIdentifier() && "ident should be an array like identifier");
		if (ident == nullptr)
			return true;

		unsigned int offset = 0;
		auto type = lhs->getType();
		while (type->isPointerType())
		{
			++offset;
			type = type->getPointeeType();
		}

		ArrayLikeIdentifier* arr_ident = CastAsArrayId(ident);
		unsigned int dim = arr_ident->GetDimensionality() - offset;
		arr_ident->ResetSize(dim);

		SetSizeForDimension(
			bin_op->getRHS()
			, *arr_ident
			, lhs->getType()->getPointeeType()
			, dim);

		return true;
	}

} /// namespace gap