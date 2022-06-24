#pragma once

#include "ast.hpp"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/IRBuilder.h>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>


namespace llvm {

class BasicBlock;
class CallInst;
class Constant;
class Function;
class LLVMContext;
class Module;
class Type;
class UnaryInstruction;
class Value;

}  // namespace llvm


namespace bsq {

class IrGenerator {
public:
	IrGenerator(llvm::LLVMContext&, llvm::Module&);

	bool Emit(ProgramAstNodePtr);

private:
	void Emit_(ProgramAstNodePtr);
	void Emit_(SubroutineAstNodePtr);

	void Emit_(StatementAstNodePtr);
	void Emit_(SequenceAstNodePtr);
	void Emit_(LetAstNodePtr);
	void Emit_(InputAstNodePtr);
	void Emit_(PrintAstNodePtr);
	void Emit_(IfAstNodePtr);
	void Emit_(ForAstNodePtr);
	void Emit_(WhileAstNodePtr);
	void Emit_(CallAstNodePtr);

	llvm::Value* Emit_(ExpressionAstNodePtr);
	llvm::Value* Emit_(ApplyAstNodePtr);
	llvm::Value* Emit_(BinaryExpressionAstNodePtr);
	llvm::Value* Emit_(UnaryExpressionAstNodePtr);
	llvm::Value* Emit_(TextAstNodePtr);
	llvm::Constant* Emit_(NumberAstNodePtr);
	llvm::Constant* Emit_(BooleanAstNodePtr);
	llvm::UnaryInstruction* Emit_(VariableAstNodePtr);
	llvm::Value* Emit_(ItemAstNodePtr);

	llvm::Type* ToLlvmType_(DataType type);
	llvm::Type* ToLlvmType_(std::string_view name);

	/// Определяет позицию следующего BB
	void SetCurrentBlock_(llvm::Function*, llvm::BasicBlock*);

	void PrepareLibrary_();
	void DeclareLibraryFunction_(std::string_view name, std::string_view signature);
	llvm::FunctionCallee LibraryFunction_(std::string_view name);
	llvm::FunctionCallee UserFunction_(std::string_view name);

	void CreateEntryPoint_();
	void DeclareSubroutines_(ProgramAstNodePtr);
	void DefineSubroutines_(ProgramAstNodePtr);
	bool NeedCreateTemporaryText_(ExpressionAstNodePtr expression);
	llvm::CallInst* CreateLibraryFunctionCall_(std::string_view function_name, const llvm::ArrayRef<llvm::Value*>& args);

private:
	llvm::LLVMContext& context_;
	llvm::IRBuilder<> ir_builder_;

	ProgramAstNodePtr program_;
	llvm::Module& module_;

	std::unordered_map<std::string, llvm::FunctionType*> library_functions_;
	std::unordered_map<std::string, llvm::Value*> textual_constants_;
	std::unordered_map<std::string, llvm::Value*> variable_addresses_;

	llvm::Type* VoidType_ = ir_builder_.getVoidTy();
	llvm::Type* BooleanType_ = ir_builder_.getInt1Ty();
	llvm::Type* NumericType_ = ir_builder_.getDoubleTy();
	llvm::Type* TextualType_ = ir_builder_.getInt8PtrTy();
};

}  // namespace bsq
