#pragma once

#include <filesystem>
#include <list>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "ast.hpp"
#include "lexeme.hpp"
#include "lexeme_reader.hpp"


namespace bsq {

class SyntaxParser {
public:
	explicit SyntaxParser(const std::filesystem::path& filename);

	ProgramAstNodePtr Parse();

private:
	void ParseProgram_();
	void ParseSubroutine_();
	StatementAstNodePtr ParseStatements_();
	StatementAstNodePtr ParseInput_();
	StatementAstNodePtr ParsePrint_();
	StatementAstNodePtr ParseLet_();
	StatementAstNodePtr ParseIf_();
	StatementAstNodePtr ParseWhile_();
	StatementAstNodePtr ParseFor_();
	StatementAstNodePtr ParseCall_();
	ExpressionAstNodePtr ParseExpression_();
	ExpressionAstNodePtr ParseAddition_();
	ExpressionAstNodePtr ParseMultiplication_();
	ExpressionAstNodePtr ParsePower_();
	ExpressionAstNodePtr ParseFactor_();

	void ParseNewLines_();

	void VerifyAndEatNextToken_(Token token);

	/// Создаёт локальную переменную или возвращает уже существующую
	VariableAstNodePtr CreateOrGetLocalVariable_(std::string_view name, bool is_r_value);

	/// Находит подпрограмму и проверяет типы аргументов и параметров
	SubroutineAstNodePtr SafeGetSubroutine_(std::string_view name);

private:
	ProgramAstNodePtr program_;  ///< Корень дерева

	SubroutineAstNodePtr current_subroutine_;

	LexemeReader reader_;
	Lexeme next_lexeme_;

	/// Неразрешённые ссылки: имя подпрограммы -> список объектов ApplyAstNode, на которые она ссылается
	std::map<std::string, std::list<ApplyAstNodePtr>> unresolved_links_;

	using BuiltinSubroutine = std::tuple<std::string, std::vector<std::string>, bool>;
	std::vector<BuiltinSubroutine> builtin_subroutines_;
};

}  // namespace bsq
