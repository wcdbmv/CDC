#include "ir_generator.hpp"

#include <iostream>
#include <list>
#include <system_error>

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>

#include "ast.hpp"


namespace {

class Tracer {
public:
	explicit Tracer(std::string_view text) {
		indent_ += 2;
		std::cout << std::string(indent_, ' ') << "Emit_(" << text << ")" << std::endl;
	}

	~Tracer() {
		indent_ -= 2;
	}

private:
	static inline size_t indent_ = 0;
};

#define TRACE(t) Tracer _t((#t))
//#define TRACE(t) (void)(#t)

}  // namespace


namespace bsq {

IrGenerator::IrGenerator(llvm::LLVMContext& context, llvm::Module& module)
	: context_{context}
	, ir_builder_{context_}
	, module_{module}
{
	PrepareLibrary_();
}

bool IrGenerator::Emit(ProgramAstNodePtr program) {
	try {
		Emit_(program);
		llvm::verifyModule(module_);
	}
	catch (...) {
		return false;
	}

	return true;
}

void IrGenerator::Emit_(ProgramAstNodePtr program) {
	TRACE(Program);

	DeclareSubroutines_(program);
	DefineSubroutines_(program);

	CreateEntryPoint_();  // main()
}

void IrGenerator::Emit_(SubroutineAstNodePtr subroutine) {
	TRACE(Subroutine);

	auto* function = module_.getFunction(subroutine->GetName());
	if (function == nullptr) {
		return;  // невозможно
	}

	auto* label_start = llvm::BasicBlock::Create(context_, "label_start", function);
	ir_builder_.SetInsertPoint(label_start);

	for (auto& arg : function->args()) {
		const auto index = arg.getArgNo();
		arg.setName(subroutine->GetParameters()[index]);
	}

	variable_addresses_.clear();

	std::list<llvm::Value*> local_text_variables;
	std::list<llvm::Value*> local_array_variables;

	for (const auto& local_variable : subroutine->local_variables) {
		auto* llvm_type = local_variable->GetType() == DataType::kBoolean
			? ir_builder_.getInt8Ty()
			: ToLlvmType_(local_variable->GetType());  // TODO
		auto* array_size = local_variable->GetType() == DataType::kArray
			? llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), local_variable->array_size)
			: nullptr;
		auto* address = ir_builder_.CreateAlloca(llvm_type, array_size, local_variable->GetName() + "_addr");
		variable_addresses_[local_variable->GetName()] = address;
		if (local_variable->OfType(DataType::kTextual)) {
			local_text_variables.push_back(address);
		} else if (local_variable->OfType(DataType::kArray)) {
			local_array_variables.push_back(address);
		}
	}

	for (auto& arg : function->args()) {
		if (arg.getType()->isPointerTy()) {
			auto* parameter_value = CreateLibraryFunctionCall_("bsq_text_clone", {&arg});
			ir_builder_.CreateStore(parameter_value, variable_addresses_[arg.getName().str()]);
			local_text_variables.remove(variable_addresses_[arg.getName().str()]);
			local_array_variables.remove(variable_addresses_[arg.getName().str()]);
		} else {
			ir_builder_.CreateStore(&arg, variable_addresses_[arg.getName().str()]);
		}
	}

	auto* one = ir_builder_.getInt64(1);
	for (auto* local_text_variable : local_text_variables) {
		auto* dev_alloc = CreateLibraryFunctionCall_("malloc", {one});
		ir_builder_.CreateStore(dev_alloc, local_text_variable);
	}

	Emit_(subroutine->body);

	// освобождение памяти под текстовые локальные переменные
	for (auto& local_variable : subroutine->local_variables) {
		if (local_variable->GetName() == subroutine->GetName()) {
			continue;
		}

		if (local_variable->OfType(DataType::kNumeric) || local_variable->OfType(DataType::kBoolean)) {
			continue;
		}

		if (DataType::kTextual == local_variable->GetType()) {
			auto* address = ir_builder_.CreateLoad(TextualType_, variable_addresses_[local_variable->GetName()]);
			CreateLibraryFunctionCall_("free", {address});
		}
	}

	if (function->getReturnType()->isVoidTy()) {
		ir_builder_.CreateRetVoid();
	} else {
		auto return_value = ir_builder_.CreateLoad(function->getReturnType(), variable_addresses_[subroutine->GetName()]);
		ir_builder_.CreateRet(return_value);
	}

	llvm::verifyFunction(*function);
}

void IrGenerator::Emit_(StatementAstNodePtr statement) {
	switch (statement->GetNodeType()) {
	case AstNodeType::kSequence:
		Emit_(std::dynamic_pointer_cast<SequenceAstNode>(statement));
		break;
	case AstNodeType::kInput:
		Emit_(std::dynamic_pointer_cast<InputAstNode>(statement));
		break;
	case AstNodeType::kPrint:
		Emit_(std::dynamic_pointer_cast<PrintAstNode>(statement));
		break;
	case AstNodeType::kLet:
		Emit_(std::dynamic_pointer_cast<LetAstNode>(statement));
		break;
	case AstNodeType::kIf:
		Emit_(std::dynamic_pointer_cast<IfAstNode>(statement));
		break;
	case AstNodeType::kWhile:
		Emit_(std::dynamic_pointer_cast<WhileAstNode>(statement));
		break;
	case AstNodeType::kFor:
		Emit_(std::dynamic_pointer_cast<ForAstNode>(statement));
		break;
	case AstNodeType::kCall:
		Emit_(std::dynamic_pointer_cast<CallAstNode>(statement));
		break;
	default:
		break;
	}
}

void IrGenerator::Emit_(SequenceAstNodePtr sequence) {
	TRACE(Sequence);

	for (auto& statement : sequence->items) {
		Emit_(statement);
	}
}

void IrGenerator::Emit_(LetAstNodePtr let) {
	TRACE(Let);

	auto* value = Emit_(let->expression);
	if (std::dynamic_pointer_cast<ItemAstNode>(let->expression)) {
		value = ir_builder_.CreateLoad(NumericType_, value);
	}
	auto* address = variable_addresses_[let->variable->GetName()];

	if (let->variable->OfType(DataType::kArray)) {
		auto* result = Emit_(let->array_index);
		auto* idx_p_1 = ir_builder_.CreateFPToSI(result, ir_builder_.getInt32Ty());
		auto* idx = ir_builder_.CreateAdd(ir_builder_.getInt32(-1), idx_p_1);
		address = ir_builder_.CreateGEP(NumericType_, address, idx);
	}
	else if (let->variable->OfType(DataType::kTextual)) {
		auto* load = ir_builder_.CreateLoad(TextualType_, address);
		CreateLibraryFunctionCall_("free", {load});
		if (!NeedCreateTemporaryText_(let->expression)) {
			value = CreateLibraryFunctionCall_("bsq_text_clone", {value});
		}
	} else if (let->variable->OfType(DataType::kBoolean)) {
		value = ir_builder_.CreateZExt(value, ir_builder_.getInt8Ty());
	}

	ir_builder_.CreateStore(value, address);
}

void IrGenerator::Emit_(InputAstNodePtr input) {
	TRACE(Input);

	auto* prompt = Emit_(input->prompt);

	std::string_view function_name;
	if (input->item) {
		function_name = "bsq_number_input";
	} else if (input->variable->OfType(DataType::kBoolean)) {
		function_name = "bool_input";
	} else if (input->variable->OfType(DataType::kNumeric)) {
		function_name = "bsq_number_input";
	} else if (input->variable->OfType(DataType::kTextual)) {
		function_name = "bsq_text_input";
	}

	auto* value = CreateLibraryFunctionCall_(function_name, {prompt});

	if (input->item) {
		auto* result = Emit_(input->item->expression);
		auto* idx_p_1 = ir_builder_.CreateFPToSI(result, ir_builder_.getInt32Ty());
		auto* idx = ir_builder_.CreateAdd(ir_builder_.getInt32(-1), idx_p_1);
		auto* gep = ir_builder_.CreateGEP(ToLlvmType_(input->item->array->GetType()), variable_addresses_[input->item->array->GetName()], idx);
		ir_builder_.CreateStore(value, gep);
	} else {
		ir_builder_.CreateStore(value, variable_addresses_[input->variable->GetName()]);
	}
}

void IrGenerator::Emit_(PrintAstNodePtr print) {
	TRACE(Print);

	auto* expression = Emit_(print->expression);

	if (print->expression->OfType(DataType::kBoolean)) {
		// CreateLibraryFunctionCall_("bool_print", {expression});
	} else if (print->expression->OfType(DataType::kTextual)) {
		CreateLibraryFunctionCall_("bsq_text_print", {expression});
		if (NeedCreateTemporaryText_(print->expression)) {
			CreateLibraryFunctionCall_("free", {expression});
		}
	} else if (print->expression->OfType(DataType::kNumeric)) {
		if (std::dynamic_pointer_cast<ItemAstNode>(print->expression)) {
			expression = ir_builder_.CreateLoad(NumericType_, expression);
		}
		CreateLibraryFunctionCall_("bsq_number_print", {expression});
	}
}

void IrGenerator::Emit_(IfAstNodePtr if_node) {
	TRACE(If);

	auto* function = ir_builder_.GetInsertBlock()->getParent();

	auto* end_if = llvm::BasicBlock::Create(context_, "", function);

	auto* first = llvm::BasicBlock::Create(context_, "", function, end_if);
	SetCurrentBlock_(function, first);

	StatementAstNodePtr statement = if_node;
	while (auto if_in_chain = std::dynamic_pointer_cast<IfAstNode>(statement)) {
		auto* then_block = llvm::BasicBlock::Create(context_, "", function, end_if);
		auto* else_block = llvm::BasicBlock::Create(context_, "", function, end_if);

		auto* condition = Emit_(if_in_chain->condition);
		//condition = ir_builder_.CreateFCmpUNE(condition, Zero);

		ir_builder_.CreateCondBr(condition, then_block, else_block);

		SetCurrentBlock_(function, then_block);

		Emit_(if_in_chain->then);
		ir_builder_.CreateBr(end_if);

		SetCurrentBlock_(function, else_block);

		statement = if_in_chain->otherwise;
	}

	if (statement != nullptr) {
		Emit_(statement);
	}

	SetCurrentBlock_(function, end_if);
}

void IrGenerator::Emit_(WhileAstNodePtr while_node) {
	TRACE(While);

	auto* function = ir_builder_.GetInsertBlock()->getParent();

	auto* condition_block = llvm::BasicBlock::Create(context_, "", function);
	auto* body_block = llvm::BasicBlock::Create(context_, "", function);
	auto* end_while = llvm::BasicBlock::Create(context_, "", function);

	SetCurrentBlock_(function, condition_block);

	auto* condition_expression = Emit_(while_node->condition);
	ir_builder_.CreateCondBr(condition_expression, body_block, end_while);

	SetCurrentBlock_(function, body_block);

	Emit_(while_node->body);
	ir_builder_.CreateBr(condition_block);

	SetCurrentBlock_(function, end_while);
}

void IrGenerator::Emit_(ForAstNodePtr for_node) {
	TRACE(For);

	auto* function = ir_builder_.GetInsertBlock()->getParent();

	auto* condition_block = llvm::BasicBlock::Create(context_, "", function);
	auto* body_block = llvm::BasicBlock::Create(context_, "", function);
	auto* end_for = llvm::BasicBlock::Create(context_, "", function);

	auto* parameter = variable_addresses_[for_node->variable->GetName()];
	auto* begin = Emit_(for_node->begin);
	ir_builder_.CreateStore(begin, parameter);
	auto* end = Emit_(for_node->end);
	auto* step = llvm::ConstantFP::get(NumericType_, for_node->step->GetValue());

	SetCurrentBlock_(function, condition_block);

	auto* parameter_value = ir_builder_.CreateLoad(NumericType_, parameter);
	llvm::Value* condition_expression = nullptr;
	if (for_node->step->GetValue() > 0.0) {
		condition_expression = ir_builder_.CreateFCmpOLT(parameter_value, end);
	} else if (for_node->step->GetValue() < 0.0) {
		condition_expression = ir_builder_.CreateFCmpOGT(parameter_value, end);
	}
	ir_builder_.CreateCondBr(condition_expression, body_block, end_for);

	SetCurrentBlock_(function, body_block);

	Emit_(for_node->body);

	auto* parameter_value2 = ir_builder_.CreateLoad(NumericType_, parameter);
	auto* add = ir_builder_.CreateFAdd(parameter_value2, step);
	ir_builder_.CreateStore(add, parameter);

	ir_builder_.CreateBr(condition_block);

	SetCurrentBlock_(function, end_for);
}

void IrGenerator::Emit_(CallAstNodePtr call) {
	TRACE(Call);

	Emit_(call->subroutine_call);
}

llvm::Value* IrGenerator::Emit_(ExpressionAstNodePtr expression) {
	llvm::Value* result = nullptr;

	switch (expression->GetNodeType()) {
	case AstNodeType::kBoolean:
		result = Emit_(std::dynamic_pointer_cast<BooleanAstNode>(expression));
		break;
	case AstNodeType::kNumber:
		result = Emit_(std::dynamic_pointer_cast<NumberAstNode>(expression));
		break;
	case AstNodeType::kText:
		result = Emit_(std::dynamic_pointer_cast<TextAstNode>(expression));
		break;
	case AstNodeType::kVariable:
		result = Emit_(std::dynamic_pointer_cast<VariableAstNode>(expression));
		break;
	case AstNodeType::kItem:
		result = Emit_(std::dynamic_pointer_cast<ItemAstNode>(expression));
		break;
	case AstNodeType::kUnary:
		result = Emit_(std::dynamic_pointer_cast<UnaryExpressionAstNode>(expression));
		break;
	case AstNodeType::kBinary:
		result = Emit_(std::dynamic_pointer_cast<BinaryExpressionAstNode>(expression));
		break;
	case AstNodeType::kApply:
		result = Emit_(std::dynamic_pointer_cast<ApplyAstNode>(expression));
		break;
	default:
		break;
	}

	return result;
}

llvm::Value* IrGenerator::Emit_(TextAstNodePtr text) {
	TRACE(Text);

	if (const auto it = textual_constants_.find(text->GetValue()); it != textual_constants_.end()) {
		return it->second;
	}

	auto* global_str = ir_builder_.CreateGlobalStringPtr(text->GetValue(), "g_str");
	textual_constants_[text->GetValue()] = global_str;

	return global_str;
}

llvm::Constant* IrGenerator::Emit_(NumberAstNodePtr number) {
	TRACE(Number);

	return llvm::ConstantFP::get(NumericType_, number->GetValue());
}

llvm::Constant* IrGenerator::Emit_(BooleanAstNodePtr boolean) {
	TRACE(BooleanAstNode);

	return llvm::ConstantInt::getBool(BooleanType_, boolean->GetValue());
}

llvm::UnaryInstruction* IrGenerator::Emit_(VariableAstNodePtr variable) {
	TRACE(Variable);

	auto* variable_address = variable_addresses_[variable->GetName()];

	if (variable->OfType(DataType::kBoolean)) {
		llvm::Type* ByteType = ir_builder_.getInt8Ty();
		llvm::LoadInst* result = ir_builder_.CreateLoad(ByteType, variable_address, variable->GetName());
		return llvm::dyn_cast<llvm::UnaryInstruction>(ir_builder_.CreateTrunc(result, BooleanType_));  // TODO
	}

	return ir_builder_.CreateLoad(ToLlvmType_(variable->GetType()), variable_address, variable->GetName());
}

llvm::Value* IrGenerator::Emit_(ItemAstNodePtr item) {
	TRACE(Item);

	auto* result = Emit_(item->expression);
	auto* idx_p_1 = ir_builder_.CreateFPToSI(result, ir_builder_.getInt32Ty());
	auto* idx = ir_builder_.CreateAdd(ir_builder_.getInt32(-1), idx_p_1);
	return ir_builder_.CreateGEP(NumericType_, variable_addresses_[item->array->GetName()], idx);
}

llvm::Value* IrGenerator::Emit_(ApplyAstNodePtr apply) {
	TRACE(Apply);

	llvm::SmallVector<llvm::Value*> arguments, temporaries;
	for (const auto& argument : apply->GetArguments()) {
		auto arg = Emit_(argument);
		arguments.push_back(arg);
		if (NeedCreateTemporaryText_(argument)) {
			temporaries.push_back(arg);
		}
	}

	auto callee = UserFunction_(apply->GetCallee()->GetName());
	auto* call = ir_builder_.CreateCall(callee, arguments);

	for (auto* temporary : temporaries) {
		if (temporary->getType()->isPointerTy()) {
			CreateLibraryFunctionCall_("free", {temporary});
		}
	}

	return call;
}

llvm::Value* IrGenerator::Emit_(BinaryExpressionAstNodePtr binary) {
	TRACE(Binary);

	const bool is_textual = binary->GetLeftOperand()->OfType(DataType::kTextual)
		&& binary->GetRightOperand()->OfType(DataType::kTextual);
	const bool is_numeric = binary->GetLeftOperand()->OfType(DataType::kNumeric)
		&& binary->GetRightOperand()->OfType(DataType::kNumeric);
	const bool is_boolean = binary->GetLeftOperand()->OfType(DataType::kBoolean)
		&& binary->GetRightOperand()->OfType(DataType::kBoolean);

	auto* lhs = Emit_(binary->GetLeftOperand());
	if (std::dynamic_pointer_cast<ItemAstNode>(binary->GetLeftOperand())) {
		lhs = ir_builder_.CreateLoad(NumericType_, lhs);
	}
	auto* rhs = Emit_(binary->GetRightOperand());
	if (std::dynamic_pointer_cast<ItemAstNode>(binary->GetRightOperand())) {
		rhs = ir_builder_.CreateLoad(NumericType_, rhs);
	}

	llvm::Value* return_value = nullptr;
	switch (binary->GetOperation()) {
	case Operation::kAdd:
		return_value = ir_builder_.CreateFAdd(lhs, rhs, "add");
		break;
	case Operation::kSub:
		return_value = ir_builder_.CreateFSub(lhs, rhs, "sub");
		break;
	case Operation::kMul:
		return_value = ir_builder_.CreateFMul(lhs, rhs, "mul");
		break;
	case Operation::kDiv:
		return_value = ir_builder_.CreateFDiv(lhs, rhs, "div");
		break;
	case Operation::kMod:
		return_value = ir_builder_.CreateFRem(lhs, rhs, "rem");
		break;
	case Operation::kPow:
		return_value = CreateLibraryFunctionCall_("pow", {lhs, rhs});
		break;

	case Operation::kEq:
		if (is_textual) {
			return_value = CreateLibraryFunctionCall_("bsq_text_eq", {lhs, rhs});
		} else if (is_numeric) {
			return_value = ir_builder_.CreateFCmpOEQ(lhs, rhs, "eq");
		} else if (is_boolean) {
			return_value = ir_builder_.CreateICmpEQ(lhs, rhs, "eq");
		}
		break;
	case Operation::kNe:
		if (is_textual) {
			return_value = CreateLibraryFunctionCall_("bsq_text_ne", {lhs, rhs});
		} else if (is_numeric) {
			return_value = ir_builder_.CreateFCmpONE(lhs, rhs, "ne");
		} else if (is_boolean) {
			return_value = ir_builder_.CreateICmpNE(lhs, rhs, "ne");
		}
		break;
	case Operation::kGt:
		if (is_textual) {
			return_value = CreateLibraryFunctionCall_("bsq_text_gt", {lhs, rhs});
		} else if (is_numeric) {
			return_value = ir_builder_.CreateFCmpOGT(lhs, rhs, "gt");
		}
		break;
	case Operation::kGe:
		if (is_textual) {
			return_value = CreateLibraryFunctionCall_("bsq_text_ge", {lhs, rhs});
		} else if (is_numeric) {
			return_value = ir_builder_.CreateFCmpOGE(lhs, rhs, "ge");
		}
		break;
	case Operation::kLt:
		if (is_textual) {
			return_value = CreateLibraryFunctionCall_("bsq_text_lt", {lhs, rhs});
		} else if (is_numeric) {
			return_value = ir_builder_.CreateFCmpOLT(lhs, rhs, "lt");
		}
		break;
	case Operation::kLe:
		if (is_textual) {
			return_value = CreateLibraryFunctionCall_("bsq_text_le", {lhs, rhs});
		} else if (is_numeric) {
			return_value = ir_builder_.CreateFCmpOLE(lhs, rhs, "le");
		}
		break;

	case Operation::kAnd:
		return_value = ir_builder_.CreateAnd(lhs, rhs, "and");
		break;
	case Operation::kOr:
		return_value = ir_builder_.CreateOr(lhs, rhs, "or");
		break;

	case Operation::kConc:
		return_value = CreateLibraryFunctionCall_("bsq_text_conc", {lhs, rhs});
		break;
	default:
		break;
	}

	return return_value;
}

llvm::Value* IrGenerator::Emit_(UnaryExpressionAstNodePtr unary) {
	TRACE(Unary);

	auto* operand = Emit_(unary->GetOperand());

	if (Operation::kSub == unary->GetOperation()) {
		return ir_builder_.CreateFNeg(operand, "neg");
	}

	if (Operation::kNot == unary->GetOperation()) {
		return ir_builder_.CreateNot(operand);
	}

	return operand;
}

void IrGenerator::SetCurrentBlock_(llvm::Function* function, llvm::BasicBlock* basic_block) {
	if (auto* ib = ir_builder_.GetInsertBlock(); ib && !ib->getTerminator()) {
		ir_builder_.CreateBr(basic_block);
	}

	ir_builder_.ClearInsertionPoint();

//	auto _ib = ir_builder_.GetInsertBlock();
//	if (_ib && _ib->getParent()) {
//		function->getBasicBlockList().insertAfter(_ib->getIterator(), basic_block);
//	} else {
//		function->getBasicBlockList().push_back(basic_block);
//	}

	function->getBasicBlockList().push_back(basic_block);

	ir_builder_.SetInsertPoint(basic_block);
}

void IrGenerator::PrepareLibrary_() {
	DeclareLibraryFunction_("bsq_text_clone", "T(T)");
	DeclareLibraryFunction_("bsq_text_input", "T(T)");
	DeclareLibraryFunction_("bsq_text_print", "V(T)");
	DeclareLibraryFunction_("bsq_text_conc", "T(TT)");
	DeclareLibraryFunction_("bsq_text_mid", "T(TNN)");
	DeclareLibraryFunction_("bsq_text_str", "T(N)");
	DeclareLibraryFunction_("bsq_text_eq", "B(TT)");
	DeclareLibraryFunction_("bsq_text_ne", "B(TT)");
	DeclareLibraryFunction_("bsq_text_gt", "B(TT)");
	DeclareLibraryFunction_("bsq_text_ge", "B(TT)");
	DeclareLibraryFunction_("bsq_text_lt", "B(TT)");
	DeclareLibraryFunction_("bsq_text_le", "B(TT)");

	DeclareLibraryFunction_("bsq_number_input", "N(T)");
	DeclareLibraryFunction_("bsq_number_print", "V(N)");

	DeclareLibraryFunction_("pow", "N(NN)");
	DeclareLibraryFunction_("sqrt", "N(N)");

	library_functions_["malloc"] = llvm::FunctionType::get(
		ir_builder_.getInt8PtrTy(), {ir_builder_.getInt64Ty()}, false
	);
	library_functions_["free"] = llvm::FunctionType::get(
		VoidType_, {ir_builder_.getInt8PtrTy()}, false
	);
}

void IrGenerator::DeclareLibraryFunction_(std::string_view name, std::string_view signature) {
	auto* return_type = ToLlvmType_(static_cast<DataType>(signature[0]));

	// T(TNN) -> TNN
	signature.remove_prefix(2);
	signature.remove_suffix(1);

	llvm::SmallVector<llvm::Type*> parameters_types;
	for (char t : signature) {
		parameters_types.push_back(ToLlvmType_(static_cast<DataType>(t)));
	}

	library_functions_[std::string{name}] = llvm::FunctionType::get(return_type, parameters_types, false);
}

llvm::FunctionCallee IrGenerator::LibraryFunction_(std::string_view name) {
	return module_.getOrInsertFunction(name, library_functions_[std::string{name}]);
}

llvm::FunctionCallee IrGenerator::UserFunction_(std::string_view name) {
	if ("MID$" == name) {
		return LibraryFunction_("bsq_text_mid");
	}

	if ("STR$" == name) {
		return LibraryFunction_("bsq_text_str");
	}

	if ("SQR" == name) {
		return LibraryFunction_("sqrt");
	}

	return module_.getFunction(name);
}

void IrGenerator::CreateEntryPoint_() {
	auto* Int32Ty = ir_builder_.getInt32Ty();

	auto* main_type = llvm::FunctionType::get(Int32Ty, {}, false);
	const auto linkage = llvm::GlobalValue::ExternalLinkage;
	auto* main_function = llvm::Function::Create(main_type, linkage, "main", &module_);

	auto* start = llvm::BasicBlock::Create(context_, "start", main_function);
	ir_builder_.SetInsertPoint(start);

	if (auto* user_defined_main = module_.getFunction("Main")) {
		ir_builder_.CreateCall(user_defined_main, {});
	}

	auto* return_value = llvm::ConstantInt::get(Int32Ty, 0);
	ir_builder_.CreateRet(return_value);
}

void IrGenerator::DeclareSubroutines_(ProgramAstNodePtr program) {
	for (const auto& subroutine : program->subroutines) {
		llvm::SmallVector<llvm::Type*> parameters_types;
		for (const auto& parameter : subroutine->GetParameters()) {
			parameters_types.push_back(ToLlvmType_(parameter));
		}

		llvm::Type* return_type = subroutine->is_returning_value
			? ToLlvmType_(subroutine->GetName())
			: ir_builder_.getVoidTy();

		auto* function_type = llvm::FunctionType::get(return_type, parameters_types, false);
		const auto linkage = llvm::GlobalValue::ExternalLinkage;
		llvm::Function::Create(function_type, linkage, subroutine->GetName(), &module_);
	}
}

void IrGenerator::DefineSubroutines_(ProgramAstNodePtr program) {
	for (const auto& subroutine : program->subroutines) {
		if (!subroutine->is_builtin) {
			Emit_(subroutine);
		}
	}
}

llvm::Type* IrGenerator::ToLlvmType_(DataType type) {
	switch (type) {
	case DataType::kBoolean:
		return BooleanType_;
	case DataType::kNumeric:
		return NumericType_;
	case DataType::kTextual:
		return TextualType_;
	case DataType::kArray:
		return NumericType_;
	default:
		return VoidType_;
	}
}

llvm::Type* IrGenerator::ToLlvmType_(std::string_view name) {
	return ToLlvmType_(GetIdentifierType(name));
}

bool IrGenerator::NeedCreateTemporaryText_(ExpressionAstNodePtr expression) {
	// для чисел не создаются временные объекты
	if (expression->OfType(DataType::kNumeric) || expression->OfType(DataType::kBoolean)) {
		return false;
	}

	// как и для текстовых литералов и переменных
	if (expression->GetNodeType() == AstNodeType::kText || expression->GetNodeType() == AstNodeType::kVariable) {
		return false;
	}

	return true;
}

llvm::CallInst* IrGenerator::CreateLibraryFunctionCall_(std::string_view function_name, const llvm::ArrayRef<llvm::Value*>& arguments) {
	return ir_builder_.CreateCall(LibraryFunction_(function_name), arguments);
}

}  // namespace bsq
