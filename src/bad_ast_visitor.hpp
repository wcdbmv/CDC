#pragma once

#include "ast.hpp"


namespace bsq {

class BadAstVisitor {
public:
	virtual ~BadAstVisitor() = default;

protected:
	virtual void visit(ProgramAstNodePtr node) = 0;
	virtual void visit(SubroutineAstNodePtr node) = 0;

	virtual void visit(SequenceAstNodePtr node) = 0;
	virtual void visit(LetAstNodePtr node) = 0;
	virtual void visit(InputAstNodePtr node) = 0;
	virtual void visit(PrintAstNodePtr node) = 0;
	virtual void visit(IfAstNodePtr node) = 0;
	virtual void visit(WhileAstNodePtr node) = 0;
	virtual void visit(ForAstNodePtr node) = 0;
	virtual void visit(CallAstNodePtr node) = 0;

	virtual void visit(ApplyAstNodePtr node) = 0;
	virtual void visit(BinaryExpressionAstNodePtr node) = 0;
	virtual void visit(UnaryExpressionAstNodePtr node) = 0;
	virtual void visit(VariableAstNodePtr node) = 0;
	virtual void visit(TextAstNodePtr node) = 0;
	virtual void visit(NumberAstNodePtr node) = 0;
	virtual void visit(BooleanAstNodePtr node) = 0;

	virtual void visit(AstNodePtr node);
};

}  // namespace bsq
