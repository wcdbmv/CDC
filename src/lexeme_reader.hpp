#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <string_view>

#include "lexeme.hpp"


namespace bsq {

class LexemeReader {
public:
	explicit LexemeReader(const std::filesystem::path& filename);
	~LexemeReader();

	LexemeReader& operator>>(Lexeme& lexeme);

private:
	std::ifstream input_;
	char current_char_ = '\0';

	static std::map<std::string_view, Token> keywords_;

	bool ReadNext_(Lexeme& lexeme);
	bool ReadNumber_(Lexeme& lexeme);
	bool ReadText_(Lexeme& lexeme);
	bool ReadIdentifier_(Lexeme& lexeme);
};

}  // namespace bsq
