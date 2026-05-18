#include "compiler/ast.h"

#include <sstream>

namespace torture::compiler {
namespace {

AuthorizeCall cloneCall(const AuthorizeCall& call) {
    AuthorizeCall copy;
    copy.target = call.target;
    copy.discardingReturn = call.discardingReturn;
    copy.viaPointer = call.viaPointer;
    copy.hasNullCheck = call.hasNullCheck;
    copy.nullCheck = call.nullCheck;
    copy.securityLevel = call.securityLevel;
    copy.memoryLimit = call.memoryLimit;
    copy.timeout = call.timeout;
    copy.predictStackDepth = call.predictStackDepth;
    copy.approvalTimeout = call.approvalTimeout;
    copy.authorityChain = call.authorityChain;
    copy.approvalFunction = call.approvalFunction;
    copy.location = call.location;
    copy.arguments.reserve(call.arguments.size());
    for (const auto& arg : call.arguments) {
        FunctionCallArg cloned;
        cloned.name = arg.name;
        cloned.byReference = arg.byReference;
        cloned.location = arg.location;
        if (arg.value) {
            cloned.value = cloneExpr(*arg.value);
        }
        copy.arguments.push_back(std::move(cloned));
    }
    return copy;
}

} // namespace

ExprPtr makeExpr(ExprKind kind, SourceLocation location, std::string text) {
    auto expr = std::make_unique<Expr>();
    expr->kind = kind;
    expr->location = std::move(location);
    expr->text = std::move(text);
    return expr;
}

ExprPtr cloneExpr(const Expr& expr) {
    auto copy = makeExpr(expr.kind, expr.location, expr.text);
    copy->declaredLiteral = expr.declaredLiteral;
    copy->gateOps = expr.gateOps;
    copy->authorizeCall = cloneCall(expr.authorizeCall);
    if (expr.left) {
        copy->left = cloneExpr(*expr.left);
    }
    if (expr.right) {
        copy->right = cloneExpr(*expr.right);
    }
    return copy;
}

StatementPtr makeStatement(StatementKind kind, SourceLocation location) {
    auto statement = std::make_unique<Statement>();
    statement->kind = kind;
    statement->location = std::move(location);
    return statement;
}

std::string summarizeProgram(const Program& program) {
    std::ostringstream out;
    out << "require ecc: " << (program.requireEcc ? "yes" : "no") << '\n';
    out << "functions: " << program.functions.size() << '\n';
    for (const auto& function : program.functions) {
        out << "  function " << function.name << " params=" << function.params.size()
            << " statements=" << function.body.size() << '\n';
    }
    out << "structs: " << program.structs.size() << '\n';
    for (const auto& structure : program.structs) {
        out << "  " << (structure.isClass ? "class " : "struct ") << structure.name << " fields=" << structure.fields.size()
            << " methods=" << structure.methods.size() << '\n';
    }
    return out.str();
}

} // namespace torture::compiler
