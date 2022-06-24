#include "compiler.hpp"

#include <iostream>

#include <llvm/AsmParser/Parser.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Pass.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>

#include "ast.hpp"
#include "ir_generator.hpp"
#include "semantic_checker.hpp"
#include "syntax_parser.hpp"


namespace {

using namespace bsq;

std::unique_ptr<llvm::Module> sCompileBasicIr(llvm::LLVMContext& context, const std::filesystem::path& source) {
	if (!std::filesystem::exists(source)) {
		return nullptr;
	}

	ProgramAstNodePtr program = SyntaxParser(source).Parse();
	if (!program) {
		return nullptr;
	}

	if (const auto check = SemanticChecker().Check(program); check.has_value()) {
		std::cout << check.value() << std::endl;
		return nullptr;
	}

	auto module = std::make_unique<llvm::Module>(source.string(), context);
	if (!IrGenerator(context, *module.get()).Emit(program)) {
		return nullptr;
	}
	return module;
}

}  // namespace


namespace bsq {

bool Compile(const std::filesystem::path& source) {
	const std::filesystem::path self_path = llvm::sys::fs::getMainExecutable(nullptr, nullptr);
	const auto library_path = self_path.parent_path() / "bsq_lib.ll";

	llvm::LLVMContext context;

	llvm::SMDiagnostic smd;
	auto library_module = llvm::parseAssemblyFile(library_path.string(), smd, context);
	if (!library_module) {
		return false;
	}

	auto program_module = sCompileBasicIr(context, source);
	if (!program_module) {
		return false;
	}

	auto ir_module_all = source;
	ir_module_all.replace_extension("ll");
	auto linked_module = std::make_unique<llvm::Module>(ir_module_all.string(), context);

	llvm::Linker::linkModules(*linked_module, std::move(program_module));
	llvm::Linker::linkModules(*linked_module, std::move(library_module));

	std::error_code ec;
	llvm::raw_fd_ostream out(ir_module_all.string(), ec, llvm::sys::fs::OF_None);
	if (ec) {
		std::cout << ec.value() << ": " << ec.message() << std::endl;
		return false;
	}

	llvm::legacy::PassManager pm;
	pm.add(llvm::createVerifierPass());
	pm.add(llvm::createPrintModulePass(out, ""));
	pm.run(*linked_module.get());

	return true;
}

}  // namespace bsq
