#include "syntax_parser.hpp"

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <utility>


namespace {

bool sAreVariablesNamesEqual(std::string_view first, std::string_view second) {
	std::string_view a = first, b = second;
	if (a.ends_with('$') || a.ends_with('?')) {
		a.remove_suffix(1);
	}
	if (b.ends_with('$') || b.ends_with('?')) {
		b.remove_suffix(1);
	}
	return a == b;
}

bsq::Operation sToOperation(bsq::Token token) {
	switch (token) {
	case bsq::Token::kAdd: return bsq::Operation::kAdd;
	case bsq::Token::kSub: return bsq::Operation::kSub;
	case bsq::Token::kAmp: return bsq::Operation::kConc;
	case bsq::Token::kMul: return bsq::Operation::kMul;
	case bsq::Token::kDiv: return bsq::Operation::kDiv;
	case bsq::Token::kMod: return bsq::Operation::kMod;
	case bsq::Token::kPow: return bsq::Operation::kPow;
	case bsq::Token::kEq: return bsq:: Operation::kEq;
	case bsq::Token::kNe: return bsq:: Operation::kNe;
	case bsq::Token::kGt: return bsq:: Operation::kGt;
	case bsq::Token::kGe: return bsq:: Operation::kGe;
	case bsq::Token::kLt: return bsq:: Operation::kLt;
	case bsq::Token::kLe: return bsq:: Operation::kLe;
	case bsq::Token::kAnd: return bsq::Operation::kAnd;
	case bsq::Token::kOr: return bsq:: Operation::kOr;
	default:
		throw "unknown token";
	}
}

}  // namespace


namespace bsq {

class SyntaxParseError : public std::exception {
public:
	explicit SyntaxParseError(std::string message)
		: message_{std::move(message)}
	{
	}

	[[nodiscard]] const char* what() const noexcept override {
		return message_.c_str();
	}

private:
	std::string message_;
};


SyntaxParser::SyntaxParser(const std::filesystem::path& filename)
	: reader_{filename}
{
	builtin_subroutines_ = {
		BuiltinSubroutine{"SQR", {"a"}, true},

		BuiltinSubroutine{"MID$", {"a$", "b", "c"}, true},
		BuiltinSubroutine{"STR$", {"a"}, true},
	};

	program_ = MakeAstNode<ProgramAstNode>(filename.string());
}

ProgramAstNodePtr SyntaxParser::Parse() {
	try {
		ParseProgram_();
	}
	catch (SyntaxParseError& e) {
		std::cerr << "Синтаксическая ошибка: " << e.what() << std::endl;
		return nullptr;
	}

	if (unresolved_links_.empty()) {
		return program_;
	}

	for (auto& e : unresolved_links_) {
		std::cerr << "Синтаксическая ошибка: " << e.first << " — неразрешённая ссылка на подпрограмму" << std::endl;
	}

	return nullptr;
}

/// Program [NewLines] { Subroutine NewLines }
void SyntaxParser::ParseProgram_() {
	reader_ >> next_lexeme_;

	if (next_lexeme_.OfType(Token::kNewLine)) {
		ParseNewLines_();
	}

	while (!next_lexeme_.OfType(Token::kEof)) {
		ParseSubroutine_();
		ParseNewLines_();
	}

	VerifyAndEatNextToken_(Token::kEof);
}

/// Subroutine = 'SUB' IDENT ['(' [IdentList] ')'] Statements 'END' 'SUB'.
void SyntaxParser::ParseSubroutine_() {
	VerifyAndEatNextToken_(Token::kSubroutine);
	auto sub_name = next_lexeme_.value;
	VerifyAndEatNextToken_(Token::kIdentifier);

	auto exists = find_if(program_->subroutines.begin(), program_->subroutines.end(), [&sub_name](auto sp) {
		return sAreVariablesNamesEqual(sub_name, sp->GetName()); }
	);
	if (exists != program_->subroutines.end()) {
		throw SyntaxParseError(sub_name + " — подпрограмма с таким именем уже определена");
	}

	std::vector<std::string> parameters;
	if (next_lexeme_.OfType(Token::kLeftPar)) {
		VerifyAndEatNextToken_(Token::kLeftPar);
		if (next_lexeme_.OfType(Token::kIdentifier)) {
			auto identifier = next_lexeme_.value;
			VerifyAndEatNextToken_(Token::kIdentifier);
			parameters.push_back(identifier);
			while (next_lexeme_.OfType(Token::kComma)) {
				VerifyAndEatNextToken_(Token::kComma);
				identifier = next_lexeme_.value;
				VerifyAndEatNextToken_(Token::kIdentifier);
				parameters.push_back(identifier);
			}
		}
		VerifyAndEatNextToken_(Token::kRightPar);
	}

	current_subroutine_ = MakeAstNode<SubroutineAstNode>(sub_name, parameters);
	program_->subroutines.push_back(current_subroutine_);

	for (auto& ps : current_subroutine_->GetParameters()) {
		current_subroutine_->local_variables.push_back(MakeAstNode<VariableAstNode>(ps));
	}

	current_subroutine_->body = ParseStatements_();

	VerifyAndEatNextToken_(Token::kEnd);
	VerifyAndEatNextToken_(Token::kSubroutine);

	if (auto link = unresolved_links_.find(sub_name); link != unresolved_links_.end()) {
		for (auto& apply : link->second) {
			apply->SetCallee(current_subroutine_);
		}
		unresolved_links_.erase(link);
	}
}

/// Statements = NewLines { (Let | Input | Print | If | While | For | Call) NewLines }
StatementAstNodePtr SyntaxParser::ParseStatements_() {
	ParseNewLines_();

	auto sequence = MakeAstNode<SequenceAstNode>();
	while (true) {
		StatementAstNodePtr stat;
		bool is_break = false;
		switch (next_lexeme_.token) {
		case Token::kLet:
			stat = ParseLet_();
			break;
		case Token::kInput:
			stat = ParseInput_();
			break;
		case Token::kPrint:
			stat = ParsePrint_();
			break;
		case Token::kIf:
			stat = ParseIf_();
			break;
		case Token::kWhile:
			stat = ParseWhile_();
			break;
		case Token::kFor:
			stat = ParseFor_();
			break;
		case Token::kCall:
			stat = ParseCall_();
			break;
		default:
			is_break = true;
		}
		if (is_break) {
			break;
		}
		sequence->items.push_back(stat);
		ParseNewLines_();
	}

	return sequence;
}

/// Let = 'LET' IDENT '=' Expression
StatementAstNodePtr SyntaxParser::ParseLet_() {
	VerifyAndEatNextToken_(Token::kLet);
	auto variable_name = next_lexeme_.value;
	VerifyAndEatNextToken_(Token::kIdentifier);
	VerifyAndEatNextToken_(Token::kEq);
	auto expression = ParseExpression_();

	auto variable = CreateOrGetLocalVariable_(variable_name, false);

	if (variable_name == current_subroutine_->GetName()) {
		current_subroutine_->is_returning_value = true;
	}

	return MakeAstNode<LetAstNode>(variable, expression);
}

/// Input = 'INPUT' IDENT
StatementAstNodePtr SyntaxParser::ParseInput_() {
	VerifyAndEatNextToken_(Token::kInput);

	std::string prompt = "?";
	if (next_lexeme_.OfType(Token::kText)) {
		prompt = next_lexeme_.value;
		VerifyAndEatNextToken_(Token::kText);
		VerifyAndEatNextToken_(Token::kComma);
	}

	auto variable_name = next_lexeme_.value;
	VerifyAndEatNextToken_(Token::kIdentifier);

	auto variable = CreateOrGetLocalVariable_(variable_name, false);
	return MakeAstNode<InputAstNode>(MakeAstNode<TextAstNode>(prompt), variable);
}

/// Print = 'PRINT' Expression
StatementAstNodePtr SyntaxParser::ParsePrint_() {
	VerifyAndEatNextToken_(Token::kPrint);
	auto expression = ParseExpression_();
	return MakeAstNode<PrintAstNode>(expression);
}

/// If = 'IF' Expression 'THEN' Statements
///   {'ELSEIF' Expression 'THEN' Statements }
///   ['ELSE' Statements] 'END' 'IF'
StatementAstNodePtr SyntaxParser::ParseIf_() {
	VerifyAndEatNextToken_(Token::kIf);
	auto condition = ParseExpression_();
	VerifyAndEatNextToken_(Token::kThen);
	auto then = ParseStatements_();
	auto if_node = MakeAstNode<IfAstNode>(condition, then);

	auto it = if_node;
	while (next_lexeme_.OfType(Token::kElseIf)) {
		VerifyAndEatNextToken_(Token::kElseIf);
		auto chained_condition = ParseExpression_();
		VerifyAndEatNextToken_(Token::kThen);
		auto chained_then = ParseStatements_();
		auto chained_if_node = MakeAstNode<IfAstNode>(chained_condition, chained_then);
		it->otherwise = chained_if_node;
		it = chained_if_node;
	}

	if (next_lexeme_.OfType(Token::kElse)) {
		VerifyAndEatNextToken_(Token::kElse);
		it->otherwise = ParseStatements_();
	}

	VerifyAndEatNextToken_(Token::kEnd);
	VerifyAndEatNextToken_(Token::kIf);

	return if_node;
}

/// While = 'WHILE' Expression Statements 'END' 'WHILE'
StatementAstNodePtr SyntaxParser::ParseWhile_() {
	VerifyAndEatNextToken_(Token::kWhile);
	auto condition = ParseExpression_();
	auto body = ParseStatements_();
	VerifyAndEatNextToken_(Token::kEnd);
	VerifyAndEatNextToken_(Token::kWhile);
	return MakeAstNode<WhileAstNode>(condition, body);
}

/// For = 'FOR' IDENT '=' Expression 'TO' Expression ['STEP' NUMBER]
///    Statements 'END' 'FOR'
StatementAstNodePtr SyntaxParser::ParseFor_() {
	VerifyAndEatNextToken_(Token::kFor);
	auto parameter = next_lexeme_.value;
	VerifyAndEatNextToken_(Token::kIdentifier);
	VerifyAndEatNextToken_(Token::kEq);
	auto begin_node = ParseExpression_();
	VerifyAndEatNextToken_(Token::kTo);
	auto end_node = ParseExpression_();
	double step = 1;
	if (next_lexeme_.OfType(Token::kStep)) {
		VerifyAndEatNextToken_(Token::kStep);
		bool is_negative = false;
		if (next_lexeme_.OfType(Token::kSub)) {
			VerifyAndEatNextToken_(Token::kSub);
			is_negative = true;
		}
		auto step_value = next_lexeme_.value;
		VerifyAndEatNextToken_(Token::kNumber);
		step = std::stod(step_value);
		if (is_negative) {
			step = -step;
		}
	}
	auto step_node = MakeAstNode<NumberAstNode>(step);
	auto variable_node = CreateOrGetLocalVariable_(parameter, false);
	auto body_node = ParseStatements_();
	VerifyAndEatNextToken_(Token::kEnd);
	VerifyAndEatNextToken_(Token::kFor);

	return MakeAstNode<ForAstNode>(variable_node, begin_node, end_node, step_node, body_node);
}

/// Call = 'CALL' IDENT [ExpressionList]
StatementAstNodePtr SyntaxParser::ParseCall_() {
	VerifyAndEatNextToken_(Token::kCall);
	auto name = next_lexeme_.value;
	VerifyAndEatNextToken_(Token::kIdentifier);
	std::vector<ExpressionAstNodePtr> arguments;

	if (next_lexeme_.OfTypeIn({Token::kNumber, Token::kText, Token::kIdentifier, Token::kSub, Token::kNot, Token::kLeftPar})) {
		auto expression = ParseExpression_();
		arguments.push_back(expression);
		while (next_lexeme_.OfType(Token::kComma)) {
			VerifyAndEatNextToken_(Token::kComma);
			expression = ParseExpression_();
			arguments.push_back(expression);
		}
	}

	auto caller = MakeAstNode<CallAstNode>(nullptr, arguments);

	auto callee = SafeGetSubroutine_(name);
	if (callee == nullptr) {
		unresolved_links_[name].push_back(caller->subroutine_call);
	}

	caller->subroutine_call->SetCallee(callee);

	return caller;
}

/// Expression = Addition [('=' | '<>' | '>' | '>=' | '<' | '<=') Addition]
ExpressionAstNodePtr SyntaxParser::ParseExpression_() {
	auto result = ParseAddition_();
	if (next_lexeme_.OfTypeIn({Token::kEq, Token::kNe, Token::kGt, Token::kGe, Token::kLt, Token::kLe})) {
		auto operation = sToOperation(next_lexeme_.token);
		VerifyAndEatNextToken_(next_lexeme_.token);
		auto expression = ParseAddition_();
		result = MakeAstNode<BinaryExpressionAstNode>(operation, result, expression);
	}
	return result;
}

/// Addition = Multiplication {('+' | '-' | '&' | 'OR') Multiplication}
ExpressionAstNodePtr SyntaxParser::ParseAddition_() {
	auto result = ParseMultiplication_();
	while (next_lexeme_.OfTypeIn({Token::kAdd, Token::kSub, Token::kAmp, Token::kOr})) {
		auto operation = sToOperation(next_lexeme_.token);
		VerifyAndEatNextToken_(next_lexeme_.token);
		auto expression = ParseMultiplication_();
		result = MakeAstNode<BinaryExpressionAstNode>(operation, result, expression);
	}
	return result;
}

/// Multiplication = Power {('*' | '/' | '\' | 'AND') Power}
ExpressionAstNodePtr SyntaxParser::ParseMultiplication_() {
	auto result = ParsePower_();
	while (next_lexeme_.OfTypeIn({Token::kMul, Token::kDiv, Token::kMod, Token::kAnd})) {
		auto operation = sToOperation(next_lexeme_.token);
		VerifyAndEatNextToken_(next_lexeme_.token);
		auto expression = ParsePower_();
		result = MakeAstNode<BinaryExpressionAstNode>(operation, result, expression);
	}
	return result;
}

/// Power = Factor ['^' Power]
ExpressionAstNodePtr SyntaxParser::ParsePower_() {
	auto result = ParseFactor_();
	if (next_lexeme_.OfType(Token::kPow)) {
		VerifyAndEatNextToken_(Token::kPow);
		auto expression = ParseFactor_();
		result = MakeAstNode<BinaryExpressionAstNode>(Operation::kPow, result, expression);
	}
	return result;
}

/// Factor = NUMBER | TEXT | IDENT | '(' ExpressionAstNode ')'
///        | IDENT '(' [ExpressionList] ')'
ExpressionAstNodePtr SyntaxParser::ParseFactor_() {
	// TRUE & FALSE
	if (next_lexeme_.OfType(Token::kTrue)) {
		VerifyAndEatNextToken_(Token::kTrue);
		return MakeAstNode<BooleanAstNode>(true);
	} else if (next_lexeme_.OfType(Token::kFalse)) {
		VerifyAndEatNextToken_(Token::kFalse);
		return MakeAstNode<BooleanAstNode>(false);
	}

	// NUMBER
	if (next_lexeme_.OfType(Token::kNumber)) {
		auto value = next_lexeme_.value;
		VerifyAndEatNextToken_(Token::kNumber);
		return MakeAstNode<NumberAstNode>(std::stod(value));
	}

	// TEXT
	if (next_lexeme_.OfType(Token::kText)) {
		auto value = next_lexeme_.value;
		VerifyAndEatNextToken_(Token::kText);
		return MakeAstNode<TextAstNode>(value);
	}

	// ('-' | 'NOT') Factor
	if (next_lexeme_.OfTypeIn({Token::kSub, Token::kNot})) {
		Operation operation = Operation::kNone;
		if (next_lexeme_.OfType(Token::kSub)) {
			operation = Operation::kSub;
			VerifyAndEatNextToken_(Token::kSub);
		} else if (next_lexeme_.OfType(Token::kNot)) {
			operation = Operation::kNot;
			VerifyAndEatNextToken_(Token::kNot);
		}
		auto expression = ParseFactor_();
		return MakeAstNode<UnaryExpressionAstNode>(operation, expression);
	}

	// IDENT ['(' [ExpressionList] ')']
	if (next_lexeme_.OfType(Token::kIdentifier)) {
		auto name = next_lexeme_.value;
		VerifyAndEatNextToken_(Token::kIdentifier);
		if (next_lexeme_.OfType(Token::kLeftPar)) {
			std::vector<ExpressionAstNodePtr> arguments;
			VerifyAndEatNextToken_(Token::kLeftPar);
			if (next_lexeme_.OfTypeIn(
				{
					Token::kTrue, Token::kFalse, Token::kNumber, Token::kText,
					Token::kIdentifier, Token::kSub, Token::kNot, Token::kLeftPar,
				}
			)) {
				auto expression = ParseExpression_();
				arguments.push_back(expression);
				while (next_lexeme_.OfType(Token::kComma)) {
					VerifyAndEatNextToken_(Token::kComma);
					expression = ParseExpression_();
					arguments.push_back(expression);
				}
			}
			VerifyAndEatNextToken_(Token::kRightPar);

			auto applier = MakeAstNode<ApplyAstNode>(nullptr, arguments);
			applier->SetType(GetIdentifierType(name));

			auto callee = SafeGetSubroutine_(name);
			if (callee == nullptr) {
				unresolved_links_[name].push_back(applier);
			}

			applier->SetCallee(callee);

			return applier;
		}
		return CreateOrGetLocalVariable_(name, true);
	}

	// '(' ExpressionAstNode ')'
	if (next_lexeme_.OfType(Token::kLeftPar)) {
		VerifyAndEatNextToken_(Token::kLeftPar);
		auto expression = ParseExpression_();
		VerifyAndEatNextToken_(Token::kRightPar);
		return expression;
	}

	throw SyntaxParseError{"Ожидалось NUMBER, TEXT, '-', NOT, IDENT или '(', получено: " + next_lexeme_.value};
}

void SyntaxParser::ParseNewLines_() {
	VerifyAndEatNextToken_(Token::kNewLine);
	while (next_lexeme_.OfType(Token::kNewLine)) {
		VerifyAndEatNextToken_(Token::kNewLine);
	}
}

void SyntaxParser::VerifyAndEatNextToken_(Token token) {
	if (!next_lexeme_.OfType(token)) {
		throw SyntaxParseError{"Ожидалось: " + ToString(token) + ", получено: " + next_lexeme_.value};
	}

	reader_ >> next_lexeme_;
}

VariableAstNodePtr SyntaxParser::CreateOrGetLocalVariable_(std::string_view name, bool is_r_value) {
	auto& locals = current_subroutine_->local_variables;

	if (is_r_value && sAreVariablesNamesEqual(current_subroutine_->GetName(), name)) {
		throw SyntaxParseError("Имя подпрограммы используется как rvalue");
	}

	auto it = std::find_if(locals.begin(), locals.end(), [&name](auto vp) {
		return sAreVariablesNamesEqual(name, vp->GetName());
	});
	if (it != locals.end()) {
		return *it;
	}

	if (is_r_value) {
		throw SyntaxParseError(std::string{name} + " — переменная ещё не определена");
	}

	auto variable = MakeAstNode<VariableAstNode>(name);
	locals.push_back(variable);

	return variable;
}

SubroutineAstNodePtr SyntaxParser::SafeGetSubroutine_(std::string_view name) {
	for (auto subroutine : program_->subroutines) {
		if (sAreVariablesNamesEqual(subroutine->GetName(), name)) {
			return subroutine;
		}
	}

	for (auto& builtin : builtin_subroutines_) {
		if (std::get<0>(builtin) == name) {
			auto subroutine = MakeAstNode<SubroutineAstNode>(std::get<0>(builtin), std::get<1>(builtin));
			subroutine->is_builtin = true;
			subroutine->is_returning_value = std::get<2>(builtin);
			program_->subroutines.push_back(subroutine);
			return subroutine;
		}
	}

	return nullptr;
}

}  // namespace bsq
