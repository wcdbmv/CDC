#pragma once

#include <string>
#include <string_view>
#include <vector>


namespace bsq {

enum class Token {
	kNone,

	kNumber,
	kText,
	kIdentifier,
	kTrue,
	kFalse,

	kSubroutine,
	kInput,
	kPrint,
	kLet,
	kDim,
	kIf,
	kThen,
	kElseIf,
	kElse,
	kWhile,
	kFor,
	kTo,
	kStep,
	kCall,
	kEnd,

	kNewLine,

	kEq,
	kNe,
	kLt,
	kLe,
	kGt,
	kGe,

	kLeftPar,
	kRightPar,
	kComma,

	kAdd,
	kSub,
	kAmp,
	kOr,
	kMul,
	kDiv,
	kMod,
	kAnd,
	kPow,
	kNot,

	kEof,
};

std::string ToString(Token token);


struct Lexeme {
	Token token = Token::kNone;
	std::string value;

	[[nodiscard]] bool OfType(Token exp) const;
	[[nodiscard]] bool OfTypeIn(const std::vector<Token>& exps) const;
};

}  // namespace bsq
