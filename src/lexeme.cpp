#include "lexeme.hpp"


namespace bsq {

std::string ToString(Token token) {
	switch (token) {
	case Token::kNone: return "None";
	case Token::kNumber: return "Number";
	case Token::kText: return "Text";
	case Token::kTrue: return "TRUE";
	case Token::kFalse: return "FALSE";
	case Token::kIdentifier: return "IDENT";
	case Token::kSubroutine: return "SUB";
	case Token::kInput: return "INPUT";
	case Token::kPrint: return "PRINT";
	case Token::kLet: return "LET";
	case Token::kDim: return "DIM";
	case Token::kIf: return "IF";
	case Token::kThen: return "THEN";
	case Token::kElseIf: return "ELSEIF";
	case Token::kElse: return "ELSE";
	case Token::kWhile: return "WHILE";
	case Token::kFor: return "FOR";
	case Token::kTo: return "TO";
	case Token::kStep: return "STEP";
	case Token::kCall: return "CALL";
	case Token::kEnd: return "END";
	case Token::kNewLine: return "New Line";
	case Token::kEq: return "=";
	case Token::kNe: return "<>";
	case Token::kLt: return "<";
	case Token::kLe: return "<=";
	case Token::kGt: return ">";
	case Token::kGe: return ">=";
	case Token::kLeftPar: return "(";
	case Token::kRightPar: return ")";
	case Token::kComma: return ",";
	case Token::kAdd: return "+";
	case Token::kSub: return "-";
	case Token::kAmp: return "&";
	case Token::kOr: return "OR";
	case Token::kMul: return "*";
	case Token::kDiv: return "/";
	case Token::kMod: return "MOD";
	case Token::kAnd: return "AND";
	case Token::kPow: return "^";
	case Token::kNot: return "NOT";
	case Token::kEof: return "Eof";
	}
}

bool Lexeme::OfType(Token exp) const {
	return exp == token;
}

bool Lexeme::OfTypeIn(const std::vector<Token>& tokens) const {
	for (auto tok : tokens) {
		if (token == tok) {
			return true;
		}
	}
	return false;
}

}  // namespace bsq
