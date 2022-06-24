#include "lexeme_reader.hpp"

#include <cctype>
#include <string>


namespace bsq {

std::map<std::string_view, Token> LexemeReader::keywords_ = {
	{"SUB",    Token::kSubroutine},
	{"LET",    Token::kLet},
	{"DIM",    Token::kDim},
	{"PRINT",  Token::kPrint},
	{"INPUT",  Token::kInput},
	{"IF",     Token::kIf},
	{"THEN",   Token::kThen},
	{"ELSEIF", Token::kElseIf},
	{"ELSE",   Token::kElse},
	{"WHILE",  Token::kWhile},
	{"FOR",    Token::kFor},
	{"TO",     Token::kTo},
	{"STEP",   Token::kStep},
	{"CALL",   Token::kCall},
	{"END",    Token::kEnd},
	{"MOD",    Token::kMod},
	{"AND",    Token::kAnd},
	{"OR",     Token::kOr},
	{"NOT",    Token::kNot},
	{"TRUE",   Token::kTrue},
	{"FALSE",  Token::kFalse},
};

LexemeReader::LexemeReader(const std::filesystem::path& filename) {
	input_.open(filename);
	input_.unsetf(std::ios_base::skipws);
	input_ >> current_char_;
}

LexemeReader::~LexemeReader() {
	if (input_.is_open()) {
		input_.close();
	}
}

LexemeReader& LexemeReader::operator>>(Lexeme& lexeme) {
	ReadNext_(lexeme);
	return *this;
}

bool LexemeReader::ReadNext_(Lexeme& lexeme) {
	lexeme.token = Token::kNone;
	lexeme.value = "";

	while (current_char_ == ' ' || current_char_ == '\t' || current_char_ == '\r') {
		input_ >> current_char_;
	}

	if (input_.eof()) {
		lexeme.token = Token::kEof;
		lexeme.value = "EOF";
		return true;
	}

	if (std::isdigit(current_char_)) {
		return ReadNumber_(lexeme);
	}

	if (current_char_ == '"') {
		return ReadText_(lexeme);
	}

	if (std::isalpha(current_char_)) {
		return ReadIdentifier_(lexeme);
	}

	// Однострочный комментарий
	if (current_char_ == '\'') {
		while (current_char_ != '\n') {
			input_ >> current_char_;
		}
		return ReadNext_(lexeme);
	}

	if (current_char_ == '\n') {
		lexeme.token = Token::kNewLine;
		lexeme.value = "\n";
		input_ >> current_char_;
		return true;
	}

	if (current_char_ == '<') {
		lexeme.value = "<";
		input_ >> current_char_;
		if (current_char_ == '>') {
			lexeme.value.push_back('>');
			input_ >> current_char_;
			lexeme.token = Token::kNe;
		} else if (current_char_ == '=') {
			lexeme.value.push_back('=');
			input_ >> current_char_;
			lexeme.token = Token::kLe;
		} else {
			lexeme.token = Token::kLt;
		}
		return true;
	}

	if (current_char_ == '>') {
		lexeme.value = ">";
		input_ >> current_char_;
		if (current_char_ == '=') {
			lexeme.value.push_back('=');
			input_ >> current_char_;
			lexeme.token = Token::kGe;
		} else {
			lexeme.token = Token::kGt;
		}
		return true;
	}

	switch (current_char_) {
	case '(':
		lexeme.token = Token::kLeftPar;
		break;
	case ')':
		lexeme.token = Token::kRightPar;
		break;
	case ',':
		lexeme.token = Token::kComma;
		break;
	case '+':
		lexeme.token = Token::kAdd;
		break;
	case '-':
		lexeme.token = Token::kSub;
		break;
	case '*':
		lexeme.token = Token::kMul;
		break;
	case '/':
		lexeme.token = Token::kDiv;
		break;
	case '^':
		lexeme.token = Token::kPow;
		break;
	case '&':
		lexeme.token = Token::kAmp;
		break;
	case '=':
		lexeme.token = Token::kEq;
		break;
	};

	input_ >> current_char_;

	return lexeme.token != Token::kNone;
}

bool LexemeReader::ReadNumber_(Lexeme& lexeme) {
	while (std::isdigit(current_char_)) {
		lexeme.value.push_back(current_char_);
		input_ >> current_char_;
	}

	if (current_char_ == '.') {
		lexeme.value.push_back('.');
		input_ >> current_char_;
		while (std::isdigit(current_char_)) {
			lexeme.value.push_back(current_char_);
			input_ >> current_char_;
		}
	}
	lexeme.token = Token::kNumber;
	return true;
}

bool LexemeReader::ReadText_(Lexeme& lexeme) {
	input_ >> current_char_;
	while (current_char_ != '"') {
		lexeme.value.push_back(current_char_);
		input_ >> current_char_;
	}
	input_ >> current_char_;
	lexeme.token = Token::kText;
	return true;
}

bool LexemeReader::ReadIdentifier_(Lexeme& lexeme) {
	while (std::isalnum(current_char_)) {
		lexeme.value.push_back(current_char_);
		input_ >> current_char_;
	}

	if (current_char_ == '$' || current_char_ == '?') {
		lexeme.value.push_back(current_char_);
		input_ >> current_char_;
	}

	auto keyword_it = keywords_.find(lexeme.value);
	lexeme.token = keyword_it == keywords_.end() ? Token::kIdentifier : keyword_it->second;

	return true;
}

}  // namespace bsq
