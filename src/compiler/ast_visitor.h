// AST 访问者接口定义头文件
#pragma once

#include "compiler/ast.h"

namespace torture::compiler {

class AstVisitor {
public:
    virtual ~AstVisitor() = default;

    virtual void visit(const Program&) {}
    virtual void visit(const FunctionDecl&) {}
    virtual void visit(const StructDecl&) {}
    virtual void visit(const VarDecl&) {}
    virtual void visit(const Param&) {}
    virtual void visit(const TypeName&) {}
    virtual void visit(const Statement&) {}
    virtual void visit(const Expr&) {}
    virtual void visit(const FunctionCallArg&) {}
    virtual void visit(const IoClause&) {}
    virtual void visit(const AuthorizeCall&) {}
    virtual void visit(const GateOp&) {}
    virtual void visit(const AssignStmt&) {}
    virtual void visit(const FptrAssignStmt&) {}
    virtual void visit(const ProofStmt&) {}
    virtual void visit(const FormalClause&) {}
    virtual void visit(const JudgmentClause&) {}
    virtual void visit(const RoleStmt&) {}
    virtual void visit(const InstantiateStmt&) {}
};

} // namespace torture::compiler
