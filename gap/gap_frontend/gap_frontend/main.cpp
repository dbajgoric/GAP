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

#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include "compilation_driver.h"
#include "ast_helpers.h"


static llvm::cl::OptionCategory option_category("C2CUDA");

class CustomFrontendAction : public clang::ASTFrontendAction
{
public:

	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance &CI
		, StringRef file)
	{
		RegisterDeclsWithAstCtx(CI.getASTContext());
		return llvm::make_unique<gap::CompilationDriver>(CI.getASTContext());
	}

private:

	void RegisterDeclsWithAstCtx(clang::ASTContext& ast_ctx)
	{
		auto cuda_memcpy_kind_enum =
			gap::CreateEnumDecl(
				ast_ctx
				, "cudaMemcpyKind"
				, ast_ctx.UnsignedIntTy
				,
				{
					std::make_pair("cudaMemcpyHostToHost", gap::GetInt(ast_ctx.getIntWidth(ast_ctx.UnsignedIntTy), 0, false))
					, std::make_pair("cudaMemcpyHostToDevice", gap::GetInt(ast_ctx.getIntWidth(ast_ctx.UnsignedIntTy), 1, false))
				, std::make_pair("cudaMemcpyDeviceToHost", gap::GetInt(ast_ctx.getIntWidth(ast_ctx.UnsignedIntTy), 2, false))
				, std::make_pair("cudaMemcpyDeviceToDevice", gap::GetInt(ast_ctx.getIntWidth(ast_ctx.UnsignedIntTy), 3, false))
				, std::make_pair("cudaMemcpyDefault", gap::GetInt(ast_ctx.getIntWidth(ast_ctx.UnsignedIntTy), 4, false))
				});

		ast_ctx.getTranslationUnitDecl()->addDecl(
			cuda_memcpy_kind_enum);

		std::vector<
			std::tuple<
			std::string
			, clang::QualType
			, std::vector<std::pair<clang::QualType, std::string>> >> funs =
		{
			std::make_tuple(
			"ceilf"
				, ast_ctx.FloatTy
				, std::vector<std::pair<clang::QualType, std::string>>({ std::make_pair(ast_ctx.FloatTy, "x") }))

			, std::make_tuple(
				"floorf"
				, ast_ctx.FloatTy
				, std::vector<std::pair<clang::QualType, std::string>>({ std::make_pair(ast_ctx.FloatTy, "x") }))

			, std::make_tuple(
				"__min_arg"
				, ast_ctx.FloatTy
				, std::vector<std::pair<clang::QualType, std::string>>({
					std::make_pair(ast_ctx.getSizeType(), "size")
					, std::make_pair(gap::CreateIncompleteArrType(ast_ctx, ast_ctx.FloatTy), "arr") }))

			, std::make_tuple(
				"__max_arg"
				, ast_ctx.FloatTy
				, std::vector<std::pair<clang::QualType, std::string>>({
					std::make_pair(ast_ctx.getSizeType(), "size")
					, std::make_pair(gap::CreateIncompleteArrType(ast_ctx, ast_ctx.FloatTy), "arr") }))

			, std::make_tuple(
				"malloc"
				, ast_ctx.VoidPtrTy
				, std::vector<std::pair<clang::QualType, std::string>>(
					{ std::make_pair(ast_ctx.getSizeType(), "size") }))

			, std::make_tuple(
				"free"
				, ast_ctx.VoidTy
				, std::vector<std::pair<clang::QualType, std::string>>(
					{ std::make_pair(ast_ctx.VoidPtrTy, "memory") }))

			, std::make_tuple(
				"cudaMalloc"
				, ast_ctx.VoidTy
				, std::vector<std::pair<clang::QualType, std::string>>({
					std::make_pair(ast_ctx.getPointerType(ast_ctx.VoidPtrTy), "dev_ptr")
					, std::make_pair(ast_ctx.getSizeType(), "size") }))

			, std::make_tuple(
				"cudaFree"
				, ast_ctx.VoidTy
				, std::vector<std::pair<clang::QualType, std::string>>(
					{ std::make_pair(ast_ctx.VoidPtrTy, "dev_ptr") }))

			, std::make_tuple(
				"memcpy"
				, ast_ctx.VoidPtrTy
				, std::vector<std::pair<clang::QualType, std::string>>({
					std::make_pair(ast_ctx.VoidPtrTy, "dest")
					, std::make_pair(ast_ctx.getConstType(ast_ctx.VoidPtrTy), "src")
					, std::make_pair(ast_ctx.getSizeType(), "size") }))

			, std::make_tuple(
				"cudaMemcpy"
				, ast_ctx.VoidTy
				, std::vector<std::pair<clang::QualType, std::string>>({
					std::make_pair(ast_ctx.VoidPtrTy, "dest")
					, std::make_pair(ast_ctx.VoidPtrTy, "src")
					, std::make_pair(ast_ctx.getSizeType(), "size")
					, std::make_pair(ast_ctx.getEnumType(cuda_memcpy_kind_enum), "kind") }))

			, std::make_tuple(
				"cudaDeviceSynchronize"
				, ast_ctx.VoidTy
				, std::vector<std::pair<clang::QualType, std::string>>({}))

			, std::make_tuple(
				"cudaPeekAtLastError"
				, ast_ctx.VoidTy
				, std::vector<std::pair<clang::QualType, std::string>>({}))
		};

		for (auto & fn : funs)
			ast_ctx.getTranslationUnitDecl()->addDecl(
				gap::CreateFunDecl(
					ast_ctx
					, std::get<0>(fn)
					, std::get<1>(fn)
					, std::get<2>(fn)));

		ast_ctx.getTranslationUnitDecl()->addDecl(
			gap::CreateRecordDecl(
				ast_ctx
				, clang::TTK_Struct
				, "dim3"
				,
				{
					std::make_pair("x", ast_ctx.UnsignedIntTy)
					, std::make_pair("y", ast_ctx.UnsignedIntTy)
					, std::make_pair("z", ast_ctx.UnsignedIntTy)
				}));
	}
};

int main(int argc, const char **argv)
{
	clang::tooling::CommonOptionsParser opt_parser(argc, argv, option_category);
	clang::tooling::ClangTool Tool(opt_parser.getCompilations(), opt_parser.getSourcePathList());

	return Tool.run(clang::tooling::newFrontendActionFactory<CustomFrontendAction>().get());
}