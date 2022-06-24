#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace bsq {

enum class AstNodeType {
	kEmpty,
	kBoolean,
	kNumber,
	kText,
	kVariable,
	kUnary,
	kBinary,
	kApply,
	kSequence,
	kInput,
	kPrint,
	kLet,
	kIf,
	kWhile,
	kFor,
	kCall,
	kSubroutine,
	kProgram,
};


class AstNode {
public:
	explicit AstNode(AstNodeType node_type = AstNodeType::kEmpty)
		: node_type_{node_type}
	{
	}

	virtual ~AstNode() = default;

	[[nodiscard]] AstNodeType GetNodeType() const { return node_type_; }

private:
	AstNodeType node_type_;
};

using AstNodePtr = std::shared_ptr<AstNode>;
using AstNodeCPtr = std::shared_ptr<const AstNode>;


template <typename P, typename... Args>
std::shared_ptr<P> MakeAstNode(Args&& ... args) {
	return std::make_shared<P>(std::forward<Args>(args)...);
}


enum class DataType : char {
	kVoid = 'V',
	kBoolean = 'B',
	kNumeric = 'N',
	kTextual = 'T',
};

/// @brief Тип идентификатора
///
/// Тип идентификатора определяется следующим образом:
/// - если он заканчивается на '$' — текстовый;
/// - если он заканчивается на '?' — логический;
/// - иначе — числовой.
DataType GetIdentifierType(std::string_view name);
std::string ToString(DataType type);


class ExpressionAstNode : public AstNode {
public:
	ExpressionAstNode(AstNodeType node_type, DataType type)
		: AstNode{node_type}
		, type_{type}
	{
	}

	[[nodiscard]] DataType GetType() const { return type_; }
	void SetType(DataType type) { type_ = type; }

	[[nodiscard]] bool OfType(DataType type) const { return type_ == type; }
	[[nodiscard]] bool NotOfType(DataType type) const { return type_ != type; }

private:
	DataType type_;
};

using ExpressionAstNodePtr = std::shared_ptr<ExpressionAstNode>;
using ExpressionAstNodeCPtr = std::shared_ptr<const ExpressionAstNode>;


class BooleanAstNode : public ExpressionAstNode {
public:
	explicit BooleanAstNode(bool value)
		: ExpressionAstNode{AstNodeType::kBoolean, DataType::kBoolean}
		, value_{value}
	{
	}

	[[nodiscard]] bool GetValue() const { return value_; }

private:
	bool value_;
};

using BooleanAstNodePtr = std::shared_ptr<BooleanAstNode>;


class NumberAstNode : public ExpressionAstNode {
public:
	explicit NumberAstNode(double value)
		: ExpressionAstNode{AstNodeType::kNumber, DataType::kNumeric}
		, value_{value}
	{
	}

	[[nodiscard]] double GetValue() const { return value_; }

private:
	double value_;
};

using NumberAstNodePtr = std::shared_ptr<NumberAstNode>;
using NumberAstNodeCPtr = std::shared_ptr<const NumberAstNode>;


class TextAstNode : public ExpressionAstNode {
public:
	explicit TextAstNode(std::string_view value)
		: ExpressionAstNode{AstNodeType::kText, DataType::kTextual}
		, value_{value}
	{
	}

	[[nodiscard]] const std::string& GetValue() const { return value_; }

private:
	std::string value_;
};

using TextAstNodePtr = std::shared_ptr<TextAstNode>;
using TextAstNodeCPtr = std::shared_ptr<const TextAstNode>;


class VariableAstNode : public ExpressionAstNode {
public:
	explicit VariableAstNode(std::string_view name)
		: ExpressionAstNode{AstNodeType::kVariable, GetIdentifierType(name)}
		, name_{name}
	{
	}

	[[nodiscard]] const std::string& GetName() const { return name_; }

private:
	std::string name_;
};

using VariableAstNodePtr = std::shared_ptr<VariableAstNode>;
using VariableAstNodeCPtr = std::shared_ptr<const VariableAstNode>;


enum class Operation {
	kNone,
	kAdd,
	kSub,
	kMul,
	kDiv,
	kMod,
	kPow,
	kEq,
	kNe,
	kGt,
	kGe,
	kLt,
	kLe,
	kAnd,
	kOr,
	kNot,
	kConc,
};

std::string ToString(Operation opc);


class UnaryExpressionAstNode : public ExpressionAstNode {
public:
	UnaryExpressionAstNode(Operation operation, ExpressionAstNodePtr operand)
		: ExpressionAstNode{AstNodeType::kUnary, DataType::kNumeric}
		, operation_{operation}
		, operand_{std::move(operand)}
	{
	}

	[[nodiscard]] Operation GetOperation() const { return operation_; }
	[[nodiscard]] const ExpressionAstNodePtr& GetOperand() const { return operand_; }

private:
	Operation operation_;
	ExpressionAstNodePtr operand_;
};

using UnaryExpressionAstNodePtr = std::shared_ptr<UnaryExpressionAstNode>;
using UnaryExpressionAstNodeCPtr = std::shared_ptr<const UnaryExpressionAstNode>;


class BinaryExpressionAstNode : public ExpressionAstNode {
public:
	BinaryExpressionAstNode(Operation operation, ExpressionAstNodePtr left_operand, ExpressionAstNodePtr right_operand)
		: ExpressionAstNode{AstNodeType::kBinary, DataType::kVoid}
		, operation_{operation}
		, left_operand_{std::move(left_operand)}
		, right_operand_{std::move(right_operand)}
	{
	}

	[[nodiscard]] Operation GetOperation() const { return operation_; }
	[[nodiscard]] const ExpressionAstNodePtr& GetLeftOperand() const { return left_operand_; }
	[[nodiscard]] const ExpressionAstNodePtr& GetRightOperand() const { return right_operand_; }

private:
	Operation operation_;
	ExpressionAstNodePtr left_operand_;
	ExpressionAstNodePtr right_operand_;
};

using BinaryExpressionAstNodePtr = std::shared_ptr<BinaryExpressionAstNode>;
using BinaryExpressionAstNodeCPtr = std::shared_ptr<const BinaryExpressionAstNode>;


class SubroutineAstNode;

using SubroutineAstNodePtr = std::shared_ptr<SubroutineAstNode>;
using SubroutineAstNodeCPtr = std::shared_ptr<const SubroutineAstNode>;


class ApplyAstNode : public ExpressionAstNode {
public:
	ApplyAstNode(SubroutineAstNodePtr callee, std::vector<ExpressionAstNodePtr> arguments)
		: ExpressionAstNode{AstNodeType::kApply, DataType::kVoid}
		, callee_{std::move(callee)}
		, arguments_{std::move(arguments)}
	{
	}

	[[nodiscard]] const SubroutineAstNodePtr& GetCallee() const { return callee_; }
	void SetCallee(SubroutineAstNodePtr callee) { callee_ = std::move(callee); }

	[[nodiscard]] const std::vector<ExpressionAstNodePtr>& GetArguments() const { return arguments_; }

private:
	SubroutineAstNodePtr callee_;
	std::vector<ExpressionAstNodePtr> arguments_;
};

using ApplyAstNodePtr = std::shared_ptr<ApplyAstNode>;
using ApplyAstNodeCPtr = std::shared_ptr<const ApplyAstNode>;


class StatementAstNode : public AstNode {
public:
	explicit StatementAstNode(AstNodeType node_type)
		: AstNode{node_type}
	{
	}
};

using StatementAstNodePtr = std::shared_ptr<StatementAstNode>;
using StatementAstNodeCPtr = std::shared_ptr<const StatementAstNode>;


class SequenceAstNode : public StatementAstNode {
public:
	SequenceAstNode()
		: StatementAstNode{AstNodeType::kSequence}
	{
	}

	std::vector<StatementAstNodePtr> items;
};

using SequenceAstNodePtr = std::shared_ptr<SequenceAstNode>;
using SequenceAstNodeCPtr = std::shared_ptr<const SequenceAstNode>;


class InputAstNode : public StatementAstNode {
public:
	InputAstNode(TextAstNodePtr prompt, VariableAstNodePtr variable)
		: StatementAstNode{AstNodeType::kInput}
		, prompt{std::move(prompt)}
		, variable{std::move(variable)}
	{
	}

	TextAstNodePtr prompt;
	VariableAstNodePtr variable;
};

using InputAstNodePtr = std::shared_ptr<InputAstNode>;
using InputAstNodeCPtr = std::shared_ptr<const InputAstNode>;


class PrintAstNode : public StatementAstNode {
public:
	explicit PrintAstNode(ExpressionAstNodePtr expression)
		: StatementAstNode{AstNodeType::kPrint}
		, expression(std::move(expression))
	{
	}

	ExpressionAstNodePtr expression;
};

using PrintAstNodePtr = std::shared_ptr<PrintAstNode>;
using PrintAstNodeCPtr = std::shared_ptr<const PrintAstNode>;


class LetAstNode : public StatementAstNode {
public:
	LetAstNode(VariableAstNodePtr variable, ExpressionAstNodePtr expression)
		: StatementAstNode{AstNodeType::kLet}
		, variable{std::move(variable)}
		, expression{std::move(expression)}
	{
	}

	VariableAstNodePtr variable;
	ExpressionAstNodePtr expression;
};

using LetAstNodePtr = std::shared_ptr<LetAstNode>;
using LetAstNodeCPtr = std::shared_ptr<const LetAstNode>;


class IfAstNode : public StatementAstNode {
public:
	IfAstNode(ExpressionAstNodePtr condition, StatementAstNodePtr then, StatementAstNodePtr otherwise = nullptr)
		: StatementAstNode{AstNodeType::kIf}
		, condition{std::move(condition)}
		, then{std::move(then)}
		, otherwise{std::move(otherwise)}
	{
	}

	ExpressionAstNodePtr condition;
	StatementAstNodePtr then;
	StatementAstNodePtr otherwise;
};

using IfAstNodePtr = std::shared_ptr<IfAstNode>;
using IfAstNodeCPtr = std::shared_ptr<const IfAstNode>;


class WhileAstNode : public StatementAstNode {
public:
	WhileAstNode(ExpressionAstNodePtr condition, StatementAstNodePtr body)
		: StatementAstNode{AstNodeType::kWhile}
		, condition{std::move(condition)}
		, body{std::move(body)}
	{
	}

	ExpressionAstNodePtr condition;
	StatementAstNodePtr body;
};

using WhileAstNodePtr = std::shared_ptr<WhileAstNode>;
using WhileAstNodeCPtr = std::shared_ptr<WhileAstNode>;


class ForAstNode : public StatementAstNode {
public:
	ForAstNode(
		VariableAstNodePtr variable,
		ExpressionAstNodePtr begin,
		ExpressionAstNodePtr end,
		NumberAstNodePtr step,
		StatementAstNodePtr body
	)
		: StatementAstNode{AstNodeType::kFor}
		, variable{std::move(variable)}
		, begin{std::move(begin)}
		, end{std::move(end)}
		, step{std::move(step)}
		, body{std::move(body)}
	{
	}

	VariableAstNodePtr variable;
	ExpressionAstNodePtr begin;
	ExpressionAstNodePtr end;
	NumberAstNodePtr step;
	StatementAstNodePtr body;
};

using ForAstNodePtr = std::shared_ptr<ForAstNode>;
using ForAstNodeCPtr = std::shared_ptr<const ForAstNode>;


class CallAstNode : public StatementAstNode {
public:
	CallAstNode(const SubroutineAstNodePtr& sp, const std::vector<ExpressionAstNodePtr>& as)
		: StatementAstNode{AstNodeType::kCall}
		, subroutine_call{std::make_shared<ApplyAstNode>(sp, as)}
	{
	}

	ApplyAstNodePtr subroutine_call;
};

using CallAstNodePtr = std::shared_ptr<CallAstNode>;
using CallAstNodeCPtr = std::shared_ptr<const CallAstNode>;


/// @brief Подпрограмма
///
/// Является функцией, если содержит команду @c LET со своим названием.
class SubroutineAstNode : public AstNode {
public:
	SubroutineAstNode(std::string_view name, std::vector<std::string> parameters)
		: AstNode{AstNodeType::kSubroutine}
		, name_{name}
		, parameters_{std::move(parameters)}
	{
	}

	[[nodiscard]] const std::string& GetName() const { return name_; }
	[[nodiscard]] const std::vector<std::string>& GetParameters() const { return parameters_; }

	std::vector<VariableAstNodePtr> local_variables;
	StatementAstNodePtr body;
	bool is_builtin = false;
	bool is_returning_value = false;

private:
	std::string name_;
	std::vector<std::string> parameters_;
};


class ProgramAstNode : public AstNode {
public:
	explicit ProgramAstNode(std::string_view fn)
		: AstNode{AstNodeType::kProgram}
		, filename{fn}
	{
	}

	std::string filename;
	std::vector<SubroutineAstNodePtr> subroutines;
};

using ProgramAstNodePtr = std::shared_ptr<ProgramAstNode>;
using ProgramAstNodeCPtr = std::shared_ptr<const ProgramAstNode>;

}  // namespace bsq
