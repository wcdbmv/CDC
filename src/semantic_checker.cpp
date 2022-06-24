#include "semantic_checker.hpp"

#include <exception>
#include <memory>
#include <string_view>
#include <utility>


namespace bsq {

class TypeCheckError : public std::exception {
public:
	explicit TypeCheckError(std::string_view message)
		: message_{std::string{message}}
	{
	}

	TypeCheckError(Operation operation, std::string_view message)
		: message_{"'" + ToString(operation) + "' " + std::string{message}}
	{
	}

	[[nodiscard]] const char* what() const noexcept override {
		return message_.c_str();
	}

private:
	std::string message_;
};


std::optional<std::string> SemanticChecker::Check(AstNodePtr node) {
	try {
		visit(std::move(node));
	}
	catch (TypeCheckError& e) {
		return "Ошибка проверки типов: " + std::string{e.what()};
	}

	return std::nullopt;
}

void SemanticChecker::visit(ProgramAstNodePtr node) {
	for (const auto& subroutine : node->subroutines) {
		visit(subroutine);
	}
}

void SemanticChecker::visit(SubroutineAstNodePtr node) {
	if ("Main" == node->GetName()) {
		if (!node->GetParameters().empty()) {
			throw TypeCheckError("Подпрограмма Main не должна принимать аргументы");
		}
	}

	visit(node->body);
}

void SemanticChecker::visit(SequenceAstNodePtr node) {
	for (const auto& statement : node->items) {
		visit(statement);
	}
}

void SemanticChecker::visit(LetAstNodePtr node) {
	if (node->array_index) {
		visit(node->array_index);
		visit(node->expression);
		if (node->expression->GetType() != DataType::kNumeric) {
			throw TypeCheckError{
				"Переменной типа " + ToString(node->variable->GetType()) + " присваивается выражение типа " + ToString(node->expression->GetType())
			};
		}
		return;
	}
	visit(node->expression);
	if (node->expression->GetType() != node->variable->GetType()) {
		throw TypeCheckError{
			"Переменной типа " + ToString(node->variable->GetType()) +
			" присваивается выражение типа " + ToString(node->expression->GetType())
		};
	}
}

void SemanticChecker::visit(DimAstNodePtr node) {
	if (node->size->GetValue() <= 0) {
		throw TypeCheckError{"Размер массива должен быть натуральным числом"};
	}
	node->variable->SetType(DataType::kArray);
	node->variable->array_size = static_cast<size_t>(node->size->GetValue());
	if (static_cast<double>(node->variable->array_size) != node->size->GetValue()) {
		throw TypeCheckError{"Размер массива должен быть натуральным числом"};
	}
}

void SemanticChecker::visit(InputAstNodePtr node) {}

void SemanticChecker::visit(PrintAstNodePtr node) {
	visit(node->expression);
}

void SemanticChecker::visit(IfAstNodePtr node) {
	visit(node->condition);
	if (node->condition->NotOfType(DataType::kBoolean)) {
		throw TypeCheckError{
			"Тип условия в операторе IF — " + ToString(node->condition->GetType()) +
			", а должен быть " + ToString(DataType::kBoolean)
		};
	}

	visit(node->then);
	visit(node->otherwise);
}

void SemanticChecker::visit(WhileAstNodePtr node) {
	visit(node->condition);
	if (node->condition->NotOfType(DataType::kBoolean)) {
		throw TypeCheckError{
			"Тип условия в цикле WHILE — " + ToString(node->condition->GetType()) +
			", а должен быть " + ToString(DataType::kBoolean)
		};
	}

	visit(node->body);
}

void SemanticChecker::visit(ForAstNodePtr node) {
	if (node->variable->NotOfType(DataType::kNumeric)) {
		throw TypeCheckError{
			"Тип переменной в цикле FOR — " + ToString(node->variable->GetType()) +
			", а должен быть " + ToString(DataType::kNumeric)
		};
	}

	visit(node->begin);
	if (node->begin->NotOfType(DataType::kNumeric)) {
		throw TypeCheckError{
			"Тип начального значения переменной в цикле FOR — " + ToString(node->begin->GetType()) +
			", а должен быть " + ToString(DataType::kNumeric)
		};
	}

	visit(node->end);
	if (node->end->NotOfType(DataType::kNumeric)) {
		throw TypeCheckError{
			"Тип конечного значения переменной в цикле FOR — " + ToString(node->begin->GetType()) +
			", а должен быть " + ToString(DataType::kNumeric)
		};
	}

	if (node->step->GetValue() == 0) {
		throw TypeCheckError("Шаг переменной в цикле FOR равен нулю");
	}

	visit(node->body);
}

void SemanticChecker::visit(CallAstNodePtr node) {
	// Проверка совпадает с ApplyAstNode, надо временно изменить is_returning_value;

	auto procedure = node->subroutine_call->GetCallee();

	auto cached_is_returning_value = procedure->is_returning_value;
	procedure->is_returning_value = true;

	visit(node->subroutine_call);

	procedure->is_returning_value = cached_is_returning_value;
}

void SemanticChecker::visit(ApplyAstNodePtr node) {
	if (!node->GetCallee()->is_returning_value) {
		throw TypeCheckError{"Подпрограмма " + node->GetCallee()->GetName() + " не является функцией"};
	}

	const auto& parameters = node->GetCallee()->GetParameters();
	const auto& arguments = node->GetArguments();

	if (parameters.size() != arguments.size()) {
		throw TypeCheckError{
			"Количество параметров = " + std::to_string(parameters.size()) +
			", количество аргументов = " + std::to_string(arguments.size())
		};
	}

	for (int i = 0; i < arguments.size(); ++i) {
		if (GetIdentifierType(parameters[i]) != arguments[i]->GetType()) {
			throw TypeCheckError{
				"Тип " + std::to_string(i + 1) + "-го параметра — " + ToString(GetIdentifierType(parameters[i])) +
				", тип " + std::to_string(i + 1) + "-го аргумента — " + ToString(arguments[i]->GetType())
			};
		}
	}

	node->SetType(GetIdentifierType(node->GetCallee()->GetName()));
}

void SemanticChecker::visit(BinaryExpressionAstNodePtr node) {
	visit(node->GetLeftOperand());
	visit(node->GetRightOperand());

	const auto lhs_type = node->GetLeftOperand()->GetType();
	const auto rhs_type = node->GetRightOperand()->GetType();
	const auto operation = node->GetOperation();

	if (lhs_type != rhs_type) {
		throw TypeCheckError{operation, "операнды имеют различные типы: " + ToString(lhs_type) + " и " + ToString(rhs_type)};
	}
	if (lhs_type == DataType::kBoolean) {
		const auto is_allowed =    operation == Operation::kAnd
		                        || operation == Operation::kOr
		                        || operation == Operation::kEq
		                        || operation == Operation::kNe;
		if (!is_allowed) {
			throw TypeCheckError{operation, "не применяется к операндам типа " + ToString(DataType::kBoolean)};
		}
		node->SetType(DataType::kBoolean);
	} else if (lhs_type == DataType::kNumeric) {
		const auto is_not_allowed =    operation == Operation::kConc
		                            || operation == Operation::kAnd
		                            || operation == Operation::kOr;
		if (is_not_allowed) {
			throw TypeCheckError{operation, "не применяется к операндам типа " + ToString(DataType::kNumeric)};
		}

		if (operation >= Operation::kEq && operation <= Operation::kLe) {
			node->SetType(DataType::kBoolean);
		} else {
			node->SetType(DataType::kNumeric);
		}
	} else if (lhs_type == DataType::kTextual) {
		if (Operation::kConc == operation) {
			node->SetType(DataType::kTextual);
		} else if (operation >= Operation::kEq && operation <= Operation::kLe) {
			node->SetType(DataType::kBoolean);
		} else {
			throw TypeCheckError{operation, "не применяется к операндам типа " + ToString(DataType::kTextual)};
		}
	}
}

void SemanticChecker::visit(UnaryExpressionAstNodePtr node) {
	visit(node->GetOperand());

	if (node->GetOperation() == Operation::kNot && node->GetOperand()->NotOfType(DataType::kBoolean)) {
		throw TypeCheckError{
			node->GetOperation(),
			"Тип операнда — " + ToString(node->GetOperand()->GetType()) +
			", а должен быть " + ToString(DataType::kBoolean)
		};
	} else {
		node->SetType(DataType::kBoolean);
	}

	if (node->GetOperation() == Operation::kSub && node->GetOperand()->NotOfType(DataType::kNumeric)) {
		throw TypeCheckError{
			node->GetOperation(),
			"Тип операнда — " + ToString(node->GetOperand()->GetType()) +
			", а должен быть " + ToString(DataType::kNumeric)
		};
	} else {
		node->SetType(DataType::kNumeric);
	}
}

void SemanticChecker::visit(ItemAstNodePtr node) {
	if (node->array->NotOfType(DataType::kArray)) {
		throw TypeCheckError{"Обращаться по индексу можно только к переменным типа ARRAY"};
	}
	visit(node->expression);
	if (node->expression->NotOfType(DataType::kNumeric)) {
		throw TypeCheckError{"Выражение для доступа по индексу должно быть числовым"};
	}
}

void SemanticChecker::visit(VariableAstNodePtr node) {}

void SemanticChecker::visit(TextAstNodePtr node) {}

void SemanticChecker::visit(NumberAstNodePtr node) {}

void SemanticChecker::visit(BooleanAstNodePtr node) {}

void SemanticChecker::visit(AstNodePtr node) {
	BadAstVisitor::visit(node);
}

}  // namespace bsq
