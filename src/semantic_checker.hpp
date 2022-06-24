#pragma once

#include <optional>
#include <string>

#include "ast.hpp"
#include "bad_ast_visitor.hpp"


namespace bsq {

class SemanticChecker : public BadAstVisitor {
public:
	std::optional<std::string> Check(AstNodePtr node);

private:
	void visit(ProgramAstNodePtr node) override;
	void visit(SubroutineAstNodePtr node) override;

	void visit(SequenceAstNodePtr node) override;
	void visit(LetAstNodePtr node) override;
	void visit(DimAstNodePtr node) override;
	void visit(InputAstNodePtr node) override;
	void visit(PrintAstNodePtr node) override;
	void visit(IfAstNodePtr node) override;
	void visit(WhileAstNodePtr node) override;
	void visit(ForAstNodePtr node) override;
	void visit(CallAstNodePtr node) override;

	void visit(ApplyAstNodePtr node) override;
	void visit(BinaryExpressionAstNodePtr node) override;
	void visit(UnaryExpressionAstNodePtr node) override;
	void visit(ItemAstNodePtr node) override;
	void visit(VariableAstNodePtr node) override;
	void visit(TextAstNodePtr node) override;
	void visit(NumberAstNodePtr node) override;
	void visit(BooleanAstNodePtr node) override;

	void visit(AstNodePtr node) override;
};

}  // namespace bsq
