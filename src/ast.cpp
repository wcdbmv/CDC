#include "ast.hpp"


namespace bsq {

std::string ToString(Operation opc) {
	switch (opc) {
	case Operation::kNone: return "None";
	case Operation::kAdd: return "+";
	case Operation::kSub: return "-";
	case Operation::kMul: return "*";
	case Operation::kDiv: return "/";
	case Operation::kMod: return "\\";
	case Operation::kPow: return "^";
	case Operation::kEq: return "=";
	case Operation::kNe: return "<>";
	case Operation::kGt: return ">";
	case Operation::kGe: return ">=";
	case Operation::kLt: return "<";
	case Operation::kLe: return "<=";
	case Operation::kAnd: return "AND";
	case Operation::kOr: return "OR";
	case Operation::kNot: return "NOT";
	case Operation::kConc: return "&";
	default: return "UNDEFINED";
	}
}

DataType GetIdentifierType(std::string_view name) {
	if (name.ends_with('?')) {
		return DataType::kBoolean;
	}

	if (name.ends_with('$')) {
		return DataType::kTextual;
	}

	return DataType::kNumeric;
}

std::string ToString(DataType type) {
	switch (type) {
	case DataType::kVoid: return "VOID";
	case DataType::kBoolean: return "BOOLEAN";
	case DataType::kNumeric: return "NUMBER";
	case DataType::kTextual: return "TEXT";
	default: return "UNDEFINED";
	}
}

}  // namespace bsq
