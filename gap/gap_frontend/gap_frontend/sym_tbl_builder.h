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

#ifndef GAP_FRONTEND_AST_VISITOR_BASE_H
#define GAP_FRONTEND_AST_VISITOR_BASE_H

#include <memory>
#include <vector>
#include <clang/AST/RecursiveASTVisitor.h>
#include "sym_tbl_tree.h"
#include "fake_stmt_mngr.h"
#include "clang_allocator.h"

namespace gap
{

	/// Special kind of RecursiveASTVisitor that traverses the function definition
	/// and builds symbol table map representing all the scopes within the function.
	/// The class also builds and stores the list of all loop nests found within the
	/// function. This completes the first pass of the parallelization
	///
	/// FIXES AND IMPROVEMENTS:
	/// 1) Arrays of pointers are currently not supported. Example int* a[56]
	/// 2) Consider the following code segment:
	/// int a = 0;
	/// {
	///		int b = (int)a + 5;
	///		double a = 10;
	/// }
	///
	/// According to scoping rules of C/C++, the def. stmt. 'int b = a + b' references
	/// the 'a' variable defined outside the compound stmt. This behaviour is not captured
	/// correctly in the symbol table implementation as it stores the identifiers within
	/// the single scope in a map. If, after the scope has been processed, one goes back
	/// to parse the mentioned def. stmt., symbol table will report that variable 'a' is
	/// 'double a' instead of 'int a' defined in enclosing scope.
	///
	/// 3) Consider the following code segment:
	/// int** m = (int**) malloc(400 * sizeof(int*));
	/// for (int i = 0; i < 200; ++i)
	///		m[i] = (int*) malloc(100 * sizeof(int));
	/// for (int i = 200; i < 400; ++i)
	///		m[i] = (int*) malloc(50 * sizeof(int));
	///
	/// 'm' is a matrix with uneven number of columns. Currently, the compiler will deduce
	/// that second dimension is 50, even though first 200 of rows have 100 elements. This
	/// will lead to undefined behaviour as not enough memory will be allocated for such a
	/// matrix and SEGFAULT may happen in any code accessing it. Possible solutions:
	/// - try to detect such matrices and prohibit parallelization of nests accessing them
	/// - figure out what is the biggest row length and use this as a second dimension.
	/// Going back to example, the deduced matrix dimension would be (400 x 100). This will
	/// lead to allocating more memory than required but it will solve the described problem
	/// and it will allow parallelization of nests containing stmts with such matrices
	class SymTblBuilder : public clang::RecursiveASTVisitor<SymTblBuilder>
	{
		typedef clang::RecursiveASTVisitor<SymTblBuilder>::DataRecursionQueue DataRecursionQueue;
		typedef clang::RecursiveASTVisitor<SymTblBuilder> RecursiveASTVisitorBase;
		typedef bool (TblCreationPredicate)(clang::Stmt*);

	public:

		typedef std::vector<clang::ForStmt*> LoopNestVector;

		SymTblBuilder(clang::ASTContext& ast_context);

		bool TraverseFunctionDecl(clang::FunctionDecl* fun_decl);
		bool TraverseIfStmt(clang::IfStmt* if_stmt, DataRecursionQueue* queue = nullptr);
		bool TraverseForStmt(clang::ForStmt* for_stmt, DataRecursionQueue* queue = nullptr);
		bool TraverseWhileStmt(clang::WhileStmt* while_stmt, DataRecursionQueue* queue = nullptr);
		bool TraverseDoStmt(clang::DoStmt* do_stmt, DataRecursionQueue* queue = nullptr);
		bool TraverseSwitchStmt(clang::SwitchStmt* switch_stmt, DataRecursionQueue* queue = nullptr);
		bool TraverseCompoundStmt(clang::CompoundStmt* cmpnd_stmt, DataRecursionQueue* queue = nullptr);

		bool TraverseVarDecl(clang::VarDecl* var_decl);
		bool TraverseBinAssign(clang::BinaryOperator* bin_op, DataRecursionQueue* queue = nullptr);

		/// Accessors
		SymTblTree& GetSymTblTree();
		LoopNestVector& GetLoopNests();

	private:

		SymTblBuilder(const SymTblBuilder& other) = delete;
		SymTblBuilder& operator=(const SymTblBuilder& other) = delete;

		clang::ParenExpr* MakeParenExpr(clang::Expr* expr);
		static bool IsControlFlowStmt(clang::Stmt* stmt);

		void CreateScope(clang::Stmt* stmt, bool is_fake_stmt = false);
		bool CreateScopeIf(clang::Stmt* stmt, TblCreationPredicate pred);
		void CreateDummyScope();
		void ResetLoopNestInfo();

		void TraverseChildren(clang::Stmt* stmt);
		void TraverseChildrenUntil(clang::Stmt* parent_stmt, clang::Stmt* end_stmt);
		void TraverseStmtOrChildrenIfCmpndStmt(clang::Stmt* cmpnd_stmt);

		void HandleIfStmt(clang::IfStmt* if_stmt);
		void HandleConstantArrayDecl(clang::VarDecl* var_decl);
		clang::Expr* GetAllocSizeExpr(clang::CallExpr* call_expr);
		void SetSizeForDimension(
			clang::Stmt* stmt
			, ArrayLikeIdentifier& arr_ident
			, clang::QualType pointee_type
			, unsigned int dim);

		void HandlePointerDecl(clang::VarDecl* var_decl);
		void HandleOtherDecl(clang::VarDecl* var_decl);

		FakeStmtMngr m_fake_stmt_mngr;
		SymTblTree m_symtbl_tree;
		LoopNestVector m_loop_nests;
		clang::ASTContext& m_ast_context;
		ClangAllocator m_allocator;
	};

} /// namespace gap

#endif /// GAP_FRONTEND_AST_VISITOR_BASE_H
