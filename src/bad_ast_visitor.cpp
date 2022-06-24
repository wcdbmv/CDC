#include "bad_ast_visitor.hpp"

#include <memory>


namespace bsq {

void BadAstVisitor::visit(AstNodePtr node) {
	if (node == nullptr) {
		return;
	}

	switch (node->GetNodeType()) {
	case AstNodeType::kBoolean:
		visit(std::dynamic_pointer_cast<BooleanAstNode>(node));
		break;
	case AstNodeType::kNumber:
		visit(std::dynamic_pointer_cast<NumberAstNode>(node));
		break;
	case AstNodeType::kText:
		visit(std::dynamic_pointer_cast<TextAstNode>(node));
		break;
	case AstNodeType::kVariable:
		visit(std::dynamic_pointer_cast<VariableAstNode>(node));
		break;
	case AstNodeType::kUnary:
		visit(std::dynamic_pointer_cast<UnaryExpressionAstNode>(node));
		break;
	case AstNodeType::kBinary:
		visit(std::dynamic_pointer_cast<BinaryExpressionAstNode>(node));
		break;
	case AstNodeType::kApply:
		visit(std::dynamic_pointer_cast<ApplyAstNode>(node));
		break;
	case AstNodeType::kSequence:
		visit(std::dynamic_pointer_cast<SequenceAstNode>(node));
		break;
	case AstNodeType::kInput:
		visit(std::dynamic_pointer_cast<InputAstNode>(node));
		break;
	case AstNodeType::kPrint:
		visit(std::dynamic_pointer_cast<PrintAstNode>(node));
		break;
	case AstNodeType::kLet:
		visit(std::dynamic_pointer_cast<LetAstNode>(node));
		break;
	case AstNodeType::kDim:
		visit(std::dynamic_pointer_cast<DimAstNode>(node));
		break;
	case AstNodeType::kItem:
		visit(std::dynamic_pointer_cast<ItemAstNode>(node));
		break;
	case AstNodeType::kIf:
		visit(std::dynamic_pointer_cast<IfAstNode>(node));
		break;
	case AstNodeType::kWhile:
		visit(std::dynamic_pointer_cast<WhileAstNode>(node));
		break;
	case AstNodeType::kFor:
		visit(std::dynamic_pointer_cast<ForAstNode>(node));
		break;
	case AstNodeType::kCall:
		visit(std::dynamic_pointer_cast<CallAstNode>(node));
		break;
	case AstNodeType::kSubroutine:
		visit(std::dynamic_pointer_cast<SubroutineAstNode>(node));
		break;
	case AstNodeType::kProgram:
		visit(std::dynamic_pointer_cast<ProgramAstNode>(node));
		break;
	default:
		break;
	}
}

}  // namespace bsq
