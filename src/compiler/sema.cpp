#include "compiler/sema.h"

#include "vm/bytecode.h"

#include <array>
#include <algorithm>
#include <charconv>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace torture::compiler {
namespace {

struct FunctionInfo {
    bool returnable = false;
    FunctionCategory category = FunctionCategory::Callable;
    int securityLevel = 0;
    int maxStackSize = 0;
    int codeSize = 0;
    std::size_t arity = 0;
    std::string returnType;
    std::vector<std::string> paramTypes;
    SourceLocation location;
};

struct LocalInfo {
    bool immutable = false;
    std::string type;
    std::string access;
    std::string purpose;
    bool literal = false;
};

using StructFieldMap = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

std::string normalized(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), ' '), text.end());
    return text;
}

bool isOneOf(const std::string& value, std::initializer_list<const char*> options) {
    return std::find(options.begin(), options.end(), value) != options.end();
}

bool isValidType(const TypeName& type, const std::unordered_set<std::string>& structNames, bool allowVoid) {
    const auto text = normalized(type.text);
    if (allowVoid && text == vm::source_literal::kVoidType) {
        return true;
    }
    if (isOneOf(text, {"int:8", "int:16", "int:32", "int:64", "uint:8", "uint:16", "uint:32", "uint:64",
            "float:32", "float:64", "char:8", "char:32", "char:8[]", "char:32[]", "bool:1", "chain"})) {
        return true;
    }
    if (structNames.contains(text)) {
        return true;
    }
    if ((text.starts_with("ptr<") || text.starts_with("ref<")) && text.ends_with(">") && text.find(',') != std::string::npos) {
        return true;
    }
    if (text.starts_with("fptr<") && text.ends_with(">") && text.find("return:") != std::string::npos &&
        text.find("params:") != std::string::npos && text.find("security:") != std::string::npos &&
        text.find("maxstack:") != std::string::npos && text.find("codesize:") != std::string::npos) {
        return true;
    }
    return false;
}

std::optional<std::string> between(const std::string& text, const std::string& prefix, const std::string& suffix) {
    const auto begin = text.find(prefix);
    if (begin == std::string::npos) {
        return std::nullopt;
    }
    const auto valueBegin = begin + prefix.size();
    const auto end = text.find(suffix, valueBegin);
    if (end == std::string::npos) {
        return std::nullopt;
    }
    return text.substr(valueBegin, end - valueBegin);
}

int parseSmallInt(std::string text) {
    int value = 0;
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

std::vector<std::string> splitParams(std::string text) {
    std::vector<std::string> out;
    if (text.empty()) {
        return out;
    }
    std::size_t start = 0;
    while (start <= text.size()) {
        const auto comma = text.find(',', start);
        out.push_back(text.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    return out;
}

bool checkFptrMatches(
    const std::string& fptrType,
    const FunctionInfo& target,
    const SourceLocation& location,
    Diagnostics& diagnostics) {
    const auto text = normalized(fptrType);
    if (!text.starts_with("fptr<")) {
        diagnostics.error("sema", "fptr-variable-not-fptr", location, "function pointer assignment target is not an fptr");
        return false;
    }
    const auto returnType = between(text, "return:", ",params:");
    const auto params = between(text, "params:(", "),security:");
    const auto security = between(text, "security:", ",maxstack:");
    const auto maxStack = between(text, "maxstack:", ",codesize:");
    const auto codeSize = between(text, "codesize:", ">");
    if (!returnType || !params || !security || !maxStack || !codeSize) {
        diagnostics.error("sema", "malformed-fptr-type", location, "malformed fptr type");
        return false;
    }
    bool ok = true;
    if (*returnType != normalized(target.returnType)) {
        diagnostics.error("sema", "fptr-return-mismatch", location, "fptr return type does not match target function");
        ok = false;
    }
    if (splitParams(*params) != target.paramTypes) {
        diagnostics.error("sema", "fptr-param-mismatch", location, "fptr parameter types do not match target function");
        ok = false;
    }
    if (parseSmallInt(*security) != target.securityLevel) {
        diagnostics.error("sema", "fptr-security-mismatch", location, "fptr security level does not match target function");
        ok = false;
    }
    if (parseSmallInt(*maxStack) != target.maxStackSize) {
        diagnostics.error("sema", "fptr-maxstack-mismatch", location, "fptr max stack does not match target function");
        ok = false;
    }
    if (parseSmallInt(*codeSize) != target.codeSize) {
        diagnostics.error("sema", "fptr-codesize-mismatch", location, "fptr code size does not match target function");
        ok = false;
    }
    return ok;
}

bool isNumericType(const std::string& type) {
    return type.starts_with("int:") || type.starts_with("uint:") || type.starts_with("float:") || type.starts_with("char:") ||
        type == "bool:1";
}

bool isTypeCompatible(const std::string& expected, const std::string& actual) {
    const auto want = normalized(expected);
    const auto got = normalized(actual);
    if (want == got) {
        return true;
    }
    if (isNumericType(want) && isNumericType(got)) {
        return true;
    }
    if (want.starts_with("ptr<") && got.starts_with("ptr<")) {
        return true;
    }
    if (want.starts_with("ref<") && got.starts_with("ref<")) {
        return true;
    }
    return false;
}

std::optional<std::string> fptrReturnType(const std::string& fptrType) {
    const auto text = normalized(fptrType);
    if (!text.starts_with("fptr<")) {
        return std::nullopt;
    }
    return between(text, "return:", ",params:");
}

std::vector<std::string> fptrParamTypes(const std::string& fptrType) {
    const auto text = normalized(fptrType);
    if (!text.starts_with("fptr<")) {
        return {};
    }
    const auto params = between(text, "params:(", "),security:");
    if (!params) {
        return {};
    }
    return splitParams(*params);
}

std::optional<std::string> pointeeType(const std::string& pointerType) {
    const auto text = normalized(pointerType);
    if ((!text.starts_with("ptr<") && !text.starts_with("ref<")) || !text.ends_with(">")) {
        return std::nullopt;
    }
    int depth = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '<') {
            ++depth;
        } else if (text[i] == '>') {
            --depth;
        } else if (text[i] == ',' && depth == 1) {
            return text.substr(i + 1, text.size() - i - 2);
        }
    }
    return std::nullopt;
}

std::optional<std::string> inferExprType(
    const Expr* expr,
    const std::unordered_map<std::string, LocalInfo>& locals,
    const std::unordered_map<std::string, FunctionInfo>* functions = nullptr,
    const StructFieldMap* structFields = nullptr);

std::string resolveCallTarget(const std::string& target, const std::unordered_map<std::string, LocalInfo>& locals);

std::optional<std::string> inferCallReturnType(
    const AuthorizeCall& call,
    const std::unordered_map<std::string, LocalInfo>& locals,
    const std::unordered_map<std::string, FunctionInfo>* functions) {
    if (call.viaPointer) {
        const auto found = locals.find(call.target);
        if (found == locals.end()) {
            return std::nullopt;
        }
        return fptrReturnType(found->second.type);
    }
    if (functions == nullptr) {
        return std::nullopt;
    }
    const auto resolvedTarget = resolveCallTarget(call.target, locals);
    const auto found = functions->find(resolvedTarget);
    if (found == functions->end()) {
        return std::nullopt;
    }
    return found->second.returnType;
}

std::optional<std::string> inferExprType(
    const Expr* expr,
    const std::unordered_map<std::string, LocalInfo>& locals,
    const std::unordered_map<std::string, FunctionInfo>* functions,
    const StructFieldMap* structFields) {
    if (expr == nullptr) {
        return std::nullopt;
    }
    switch (expr->kind) {
    case ExprKind::Integer:
        return "int:64";
    case ExprKind::Boolean:
        return "bool:1";
    case ExprKind::String:
        return "char:8[]";
    case ExprKind::Null:
        return "null";
    case ExprKind::Identifier:
        if (const auto found = locals.find(expr->text); found != locals.end()) {
            return found->second.type;
        }
        return std::nullopt;
    case ExprKind::Field:
        if (const auto base = inferExprType(expr->left.get(), locals, functions, structFields)) {
            auto baseType = normalized(*base);
            if (const auto inner = pointeeType(baseType)) {
                baseType = *inner;
            }
            if (structFields != nullptr) {
                if (const auto structure = structFields->find(baseType); structure != structFields->end()) {
                    if (const auto field = structure->second.find(expr->text); field != structure->second.end()) {
                        return field->second;
                    }
                }
            }
            return "int:64";
        }
        return std::nullopt;
    case ExprKind::Unary:
        if (expr->text == "&") {
            if (const auto inner = inferExprType(expr->left.get(), locals, functions, structFields)) {
                return "ptr<readablewritable," + normalized(*inner) + ">";
            }
        }
        if (expr->text == "*" && expr->left) {
            if (const auto inner = inferExprType(expr->left.get(), locals, functions, structFields)) {
                if (const auto pointed = pointeeType(*inner)) {
                    return *pointed;
                }
                return inner;
            }
            return std::nullopt;
        }
        if (expr->text == "!" || expr->text == "not") {
            return "bool:1";
        }
        return std::nullopt;
    case ExprKind::Binary:
        if (expr->text == "==" || expr->text == "!=" || expr->text == "<" || expr->text == ">" ||
            expr->text == "<=" || expr->text == ">=" || expr->text == "&&" || expr->text == "||" ||
            expr->text == "and" || expr->text == "or") {
            return "bool:1";
        }
        return inferExprType(expr->left.get(), locals, functions, structFields).value_or("int:64");
    case ExprKind::Compute:
        return inferExprType(expr->left.get(), locals, functions, structFields);
    case ExprKind::Gate:
        return "bool:1";
    case ExprKind::AuthorizeCall:
        return inferCallReturnType(expr->authorizeCall, locals, functions);
    }
    return std::nullopt;
}

std::string resolveCallTarget(const std::string& target, const std::unordered_map<std::string, LocalInfo>& locals) {
    const auto dot = target.find('.');
    if (dot == std::string::npos) {
        return target;
    }
    const auto base = target.substr(0, dot);
    const auto method = target.substr(dot + 1);
    if (const auto found = locals.find(base); found != locals.end()) {
        return normalized(found->second.type) + "." + method;
    }
    return target;
}

bool isVerify(const StatementPtr& statement) {
    return statement && statement->kind == StatementKind::VerifyFunctionIdentity;
}

bool exprContainsCall(const Expr* expr) {
    if (expr == nullptr) {
        return false;
    }
    if (expr->kind == ExprKind::AuthorizeCall) {
        return true;
    }
    if (exprContainsCall(expr->left.get()) || exprContainsCall(expr->right.get())) {
        return true;
    }
    for (const auto& arg : expr->authorizeCall.arguments) {
        if (exprContainsCall(arg.value.get())) {
            return true;
        }
    }
    return false;
}

bool bodyContainsCall(const std::vector<StatementPtr>& body) {
    for (const auto& statement : body) {
        if (!statement) {
            continue;
        }
        if (statement->kind == StatementKind::AuthorizeCall || statement->kind == StatementKind::AssignFromCall) {
            return true;
        }
        if (exprContainsCall(statement->expression.get()) || exprContainsCall(statement->initializer.get()) ||
            exprContainsCall(statement->increment.get()) || exprContainsCall(statement->assign.value.get())) {
            return true;
        }
        if (statement->kind == StatementKind::VarDecl && exprContainsCall(statement->varDecl.initializer.get())) {
            return true;
        }
        if (bodyContainsCall(statement->thenBody) || bodyContainsCall(statement->elseBody)) {
            return true;
        }
    }
    return false;
}

std::size_t countLocalDeclarations(const std::vector<StatementPtr>& body) {
    std::size_t count = 0;
    for (const auto& statement : body) {
        if (!statement) {
            continue;
        }
        if (statement->kind == StatementKind::VarDecl || statement->kind == StatementKind::Instantiate) {
            ++count;
        }
        count += countLocalDeclarations(statement->thenBody);
        count += countLocalDeclarations(statement->elseBody);
    }
    return count;
}

bool hasAccess(const std::string& access, const std::string& required) {
    return normalized(access).find(required) != std::string::npos;
}

std::optional<std::string> lvalueRoot(const Expr* expr) {
    if (expr == nullptr) {
        return std::nullopt;
    }
    if (expr->kind == ExprKind::Identifier) {
        return expr->text;
    }
    if (expr->kind == ExprKind::Field) {
        return lvalueRoot(expr->left.get());
    }
    if (expr->kind == ExprKind::Unary && (expr->text == "*" || expr->text == "&")) {
        return lvalueRoot(expr->left.get());
    }
    return std::nullopt;
}

std::optional<std::string> exprPurpose(const Expr* expr, const std::unordered_map<std::string, LocalInfo>& locals) {
    const auto root = lvalueRoot(expr);
    if (!root) {
        return std::nullopt;
    }
    const auto found = locals.find(*root);
    if (found == locals.end()) {
        return std::nullopt;
    }
    return found->second.purpose;
}

bool isDeclaredLiteralExpr(const Expr* expr, const std::unordered_map<std::string, LocalInfo>& locals) {
    if (expr == nullptr) {
        return false;
    }
    if (expr->kind == ExprKind::String) {
        return expr->declaredLiteral;
    }
    if (expr->kind == ExprKind::Identifier) {
        const auto found = locals.find(expr->text);
        return found != locals.end() && found->second.literal;
    }
    return false;
}

bool isDeclaredLiteralExpr(const Expr* expr) {
    return expr != nullptr && expr->kind == ExprKind::String && expr->declaredLiteral && !expr->text.empty();
}

bool checkExprReads(const Expr* expr, Diagnostics& diagnostics, const std::unordered_map<std::string, LocalInfo>& locals) {
    if (expr == nullptr) {
        return true;
    }
    bool ok = true;
    if (expr->kind == ExprKind::Identifier) {
        const auto found = locals.find(expr->text);
        if (found == locals.end()) {
            diagnostics.error("sema", "unknown-local-read", expr->location, "read from undeclared variable '" + expr->text + "'");
            ok = false;
        } else if (!hasAccess(found->second.access, std::string{vm::source_literal::kReadable})) {
            diagnostics.error("sema", "read-not-allowed", expr->location, "variable '" + expr->text + "' is not readable");
            ok = false;
        }
    }
    if (expr->kind == ExprKind::Unary && expr->text == "&") {
        const auto root = lvalueRoot(expr->left.get());
        if (root && !locals.contains(*root)) {
            diagnostics.error("sema", "unknown-local-reference", expr->location, "reference to undeclared variable '" + *root + "'");
            return false;
        }
        return ok;
    }
    for (const auto& arg : expr->authorizeCall.arguments) {
        ok = checkExprReads(arg.value.get(), diagnostics, locals) && ok;
    }
    ok = checkExprReads(expr->left.get(), diagnostics, locals) && ok;
    ok = checkExprReads(expr->right.get(), diagnostics, locals) && ok;
    return ok;
}

bool checkImmutableAssignments(
    const FunctionDecl& function,
    const std::vector<StatementPtr>& body,
    Diagnostics& diagnostics,
    std::unordered_map<std::string, LocalInfo>& locals,
    const std::unordered_set<std::string>& structNames,
    const std::unordered_set<std::string>& classNames,
    const std::unordered_map<std::string, FunctionInfo>& functions) {
    bool ok = true;
    for (const auto& statement : body) {
        if (!statement) {
            continue;
        }
        if (statement->kind == StatementKind::VarDecl) {
            if (!isValidType(statement->varDecl.type, structNames, false)) {
                diagnostics.error(
                    "sema",
                    "invalid-type",
                    statement->location,
                    "invalid variable type '" + statement->varDecl.type.text + "'");
                ok = false;
            }
            if (locals.contains(statement->varDecl.name)) {
                diagnostics.error(
                    "sema",
                    "duplicate-local",
                    statement->location,
                    "duplicate local variable '" + statement->varDecl.name + "'");
                ok = false;
            }
            locals[statement->varDecl.name] = LocalInfo{
                statement->varDecl.immutable,
                statement->varDecl.type.text,
                statement->varDecl.access,
                statement->varDecl.purpose,
                isDeclaredLiteralExpr(statement->varDecl.initializer.get(), locals),
            };
            if (statement->varDecl.initializer) {
                ok = checkExprReads(statement->varDecl.initializer.get(), diagnostics, locals) && ok;
                const auto sourcePurpose = exprPurpose(statement->varDecl.initializer.get(), locals);
                if (sourcePurpose && *sourcePurpose != statement->varDecl.purpose &&
                    statement->varDecl.initializer->kind != ExprKind::Compute) {
                    diagnostics.error(
                        "sema",
                        "purpose-mismatch",
                        statement->location,
                        "assignment between different purposes requires compute");
                    ok = false;
                }
            }
        }
        if (statement->kind == StatementKind::Instantiate) {
            if (!structNames.contains(statement->instantiate.className)) {
                diagnostics.error(
                    "sema",
                    "unknown-class",
                    statement->location,
                    "instantiation target class '" + statement->instantiate.className + "' is not declared");
                ok = false;
            }
            if (classNames.contains(statement->instantiate.className) && statement->instantiate.authority.empty()) {
                diagnostics.error(
                    "sema",
                    "missing-instantiation-authority",
                    statement->location,
                    "object instantiation must declare who authorized it using authorized by <authority>");
                ok = false;
            }
            if (locals.contains(statement->instantiate.name)) {
                diagnostics.error(
                    "sema",
                    "duplicate-local",
                    statement->location,
                    "duplicate local variable '" + statement->instantiate.name + "'");
                ok = false;
            }
            if (!hasAccess(statement->instantiate.memoryAccess, std::string{vm::source_literal::kReadable})) {
                diagnostics.error(
                    "sema",
                    "class-memory-not-readable",
                    statement->location,
                    "instantiated object memory must be declared readable");
                ok = false;
            }
            if (!isDeclaredLiteralExpr(statement->instantiate.reason.get())) {
                diagnostics.error(
                    "sema",
                    "missing-instantiation-reason",
                    statement->location,
                    "object instantiation must explain why using because literal \"...\"");
                ok = false;
            }
            if (statement->instantiate.factory.empty() || statement->instantiate.strategy.empty() || statement->instantiate.dependency.empty()) {
                diagnostics.error(
                    "sema",
                    "incomplete-instantiation-patterns",
                    statement->location,
                    "object instantiation must name an abstract factory, strategy, and dependency injection target");
                ok = false;
            }
            locals[statement->instantiate.name] = LocalInfo{
                false,
                statement->instantiate.className,
                statement->instantiate.memoryAccess,
                "storage",
                false,
            };
        }
        if (statement->kind == StatementKind::Assign && statement->assign.target) {
            const auto root = lvalueRoot(statement->assign.target.get());
            const auto found = root ? locals.find(*root) : locals.end();
            if (found == locals.end()) {
                diagnostics.error(
                    "sema",
                    "unknown-local",
                    statement->location,
                    "assignment to undeclared variable");
                ok = false;
            } else if (found->second.immutable) {
                diagnostics.error(
                    "sema",
                    "assign-immutable",
                    statement->location,
                    "immutable variable '" + *root + "' cannot be assigned after declaration");
                ok = false;
            } else if (!hasAccess(found->second.access, "writable")) {
                diagnostics.error(
                    "sema",
                    "write-not-allowed",
                    statement->location,
                    "variable '" + *root + "' is not writable");
                ok = false;
            }
            ok = checkExprReads(statement->assign.value.get(), diagnostics, locals) && ok;
            if (found != locals.end() && statement->assign.value) {
                const auto sourcePurpose = exprPurpose(statement->assign.value.get(), locals);
                if (sourcePurpose && *sourcePurpose != found->second.purpose && statement->assign.value->kind != ExprKind::Compute) {
                    diagnostics.error(
                        "sema",
                        "purpose-mismatch",
                        statement->location,
                        "assignment between different purposes requires compute");
                    ok = false;
                }
            }
        }
        if (statement->kind == StatementKind::OperatorInput) {
            if (function.category != FunctionCategory::Approval) {
                diagnostics.error(
                    "sema",
                    "operatorinput-outside-approval",
                    statement->location,
                    "operatorinput may only appear inside approval functions");
                ok = false;
            }
            const auto found = locals.find(statement->identifier);
            if (found == locals.end()) {
                diagnostics.error(
                    "sema",
                    "operatorinput-unknown-target",
                    statement->location,
                    "operatorinput target '" + statement->identifier + "' is not declared");
                ok = false;
            } else if (found->second.type != "bool:1") {
                diagnostics.error(
                    "sema",
                    "operatorinput-non-bool",
                    statement->location,
                    "operatorinput target must be bool:1");
                ok = false;
            } else if (!hasAccess(found->second.access, "writable")) {
                diagnostics.error(
                    "sema",
                    "write-not-allowed",
                    statement->location,
                    "operatorinput target '" + statement->identifier + "' is not writable");
                ok = false;
            }
        }
        if (statement->kind == StatementKind::Return) {
            ok = checkExprReads(statement->expression.get(), diagnostics, locals) && ok;
        }
        if (statement->kind == StatementKind::If || statement->kind == StatementKind::While) {
            ok = checkExprReads(statement->expression.get(), diagnostics, locals) && ok;
        }
        if (statement->kind == StatementKind::For) {
            ok = checkExprReads(statement->initializer.get(), diagnostics, locals) && ok;
            ok = checkExprReads(statement->expression.get(), diagnostics, locals) && ok;
            ok = checkExprReads(statement->increment.get(), diagnostics, locals) && ok;
        }
        if (statement->kind == StatementKind::AuthorizeCall || statement->kind == StatementKind::AssignFromCall) {
            for (const auto& arg : statement->authorizeCall.arguments) {
                ok = checkExprReads(arg.value.get(), diagnostics, locals) && ok;
            }
        }
        if (statement->kind == StatementKind::FptrAssign) {
            const auto local = locals.find(statement->fptrAssign.variable);
            const auto target = functions.find(statement->fptrAssign.target);
            if (local == locals.end()) {
                diagnostics.error(
                    "sema",
                    "unknown-fptr-variable",
                    statement->location,
                    "unknown function pointer variable '" + statement->fptrAssign.variable + "'");
                ok = false;
            } else if (target != functions.end()) {
                ok = checkFptrMatches(local->second.type, target->second, statement->location, diagnostics) && ok;
            }
        }
        ok = checkImmutableAssignments(function, statement->thenBody, diagnostics, locals, structNames, classNames, functions) && ok;
        ok = checkImmutableAssignments(function, statement->elseBody, diagnostics, locals, structNames, classNames, functions) && ok;
    }
    return ok;
}

bool checkAuthorizeCall(
    const FunctionDecl& function,
    const AuthorizeCall& call,
    SourceLocation location,
    bool usesReturn,
    Diagnostics& diagnostics,
    const std::unordered_map<std::string, FunctionInfo>& functions,
    const std::unordered_map<std::string, LocalInfo>& locals,
    const StructFieldMap& structFields) {
    bool ok = true;
    if (function.category != FunctionCategory::Approval) {
        if (call.memoryLimit <= 0 || call.timeout <= 0 || call.predictStackDepth < 0) {
            diagnostics.error(
                "sema",
                "invalid-call-resource",
                location,
                "authorized calls must provide positive memory limit and timeout");
            ok = false;
        }
        if (!call.approvalFunction) {
            diagnostics.error(
                "sema",
                "missing-approval",
                location,
                "non-approval function calls must include with approval of <approval_function>");
            ok = false;
        }
        if (call.approvalFunction) {
            if (call.approvalTimeout <= 0) {
                diagnostics.error(
                    "sema",
                    "invalid-approval-timeout",
                    location,
                    "approval timeout must be positive");
                ok = false;
            }
            const auto approval = functions.find(*call.approvalFunction);
            if (approval == functions.end() || approval->second.category != FunctionCategory::Approval) {
                diagnostics.error(
                    "sema",
                    "bad-approval-target",
                    location,
                    "approval target '" + *call.approvalFunction + "' is not an approval function");
                ok = false;
            }
        }
    }

    if (call.viaPointer) {
        if (!call.hasNullCheck) {
            diagnostics.error("sema", "missing-nullcheck", location, "indirect calls must include a nullcheck clause");
            ok = false;
        }
        const auto pointer = locals.find(call.target);
        if (pointer == locals.end()) {
            diagnostics.error("sema", "unknown-fptr-variable", location, "unknown function pointer variable '" + call.target + "'");
            return false;
        }
        const auto returnType = fptrReturnType(pointer->second.type);
        const auto paramTypes = fptrParamTypes(pointer->second.type);
        if (!returnType) {
            diagnostics.error("sema", "fptr-variable-not-fptr", location, "indirect invocation target is not an fptr");
            return false;
        }
        if (call.arguments.size() != paramTypes.size()) {
            diagnostics.error("sema", "wrong-argument-count", location, "indirect call has wrong argument count");
            ok = false;
        }
        const auto count = std::min(call.arguments.size(), paramTypes.size());
        for (std::size_t i = 0; i < count; ++i) {
            const auto actual = inferExprType(call.arguments[i].value.get(), locals, &functions, &structFields);
            if (actual && !isTypeCompatible(paramTypes[i], *actual)) {
                diagnostics.error(
                    "sema",
                    "argument-type-mismatch",
                    call.arguments[i].location,
                    "argument type '" + *actual + "' is not compatible with parameter type '" + paramTypes[i] + "'");
                ok = false;
            }
        }
        const bool returnable = normalized(*returnType) != vm::source_literal::kVoidType;
        if (returnable && !call.discardingReturn && !usesReturn) {
            diagnostics.error(
                "sema",
                "undiscarded-return",
                location,
                "returnable indirect call must use or explicitly discard the return");
            ok = false;
        }
        if (usesReturn && !returnable) {
            diagnostics.error("sema", "assign-void-call", location, "cannot use the result of a void indirect call");
            ok = false;
        }
        return ok;
    }

    const auto resolvedTarget = resolveCallTarget(call.target, locals);
    const auto target = functions.find(resolvedTarget);
    if (target == functions.end()) {
        diagnostics.error("sema", "unknown-call-target", location, "unknown call target '" + resolvedTarget + "'");
        return false;
    }
    if (resolvedTarget == vm::builtin::kPrintLine) {
        if (call.arguments.empty() || !isDeclaredLiteralExpr(call.arguments.front().value.get(), locals)) {
            diagnostics.error(
                "sema",
                "println-requires-literal",
                location,
                "println requires its string argument to be declared with literal \"...\"");
            ok = false;
        }
    }
    if (call.arguments.size() != target->second.arity) {
        diagnostics.error("sema", "wrong-argument-count", location, "call to '" + call.target + "' has wrong argument count");
        ok = false;
    }
    const auto count = std::min(call.arguments.size(), target->second.paramTypes.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto actual = inferExprType(call.arguments[i].value.get(), locals, &functions, &structFields);
        if (actual && !isTypeCompatible(target->second.paramTypes[i], *actual)) {
            diagnostics.error(
                "sema",
                "argument-type-mismatch",
                call.arguments[i].location,
                "argument type '" + *actual + "' is not compatible with parameter type '" + target->second.paramTypes[i] + "'");
            ok = false;
        }
    }
    if (target->second.returnable && !call.discardingReturn && !usesReturn) {
        diagnostics.error(
            "sema",
            "undiscarded-return",
            location,
            "returnable call to '" + resolvedTarget + "' must use or explicitly discard the return");
        ok = false;
    }
    if (usesReturn && !target->second.returnable) {
        diagnostics.error("sema", "assign-void-call", location, "cannot use the result of void call '" + resolvedTarget + "'");
        ok = false;
    }
    if (call.securityLevel < target->second.securityLevel) {
        diagnostics.error("sema", "insufficient-security-level", location, "call security level is lower than target function requirement");
        ok = false;
    }
    return ok;
}

bool checkExpressionCalls(
    const FunctionDecl& function,
    const Expr* expr,
    Diagnostics& diagnostics,
    const std::unordered_map<std::string, FunctionInfo>& functions,
    const std::unordered_map<std::string, LocalInfo>& locals,
    const StructFieldMap& structFields) {
    if (expr == nullptr) {
        return true;
    }
    bool ok = true;
    if (expr->kind == ExprKind::AuthorizeCall) {
        ok = checkAuthorizeCall(function, expr->authorizeCall, expr->location, true, diagnostics, functions, locals, structFields) && ok;
        if (const auto type = inferCallReturnType(expr->authorizeCall, locals, &functions);
            !type || normalized(*type) == vm::source_literal::kVoidType) {
            diagnostics.error("sema", "void-call-expression", expr->location, "call expression must produce a return value");
            ok = false;
        }
        for (const auto& arg : expr->authorizeCall.arguments) {
            ok = checkExpressionCalls(function, arg.value.get(), diagnostics, functions, locals, structFields) && ok;
        }
    }
    ok = checkExpressionCalls(function, expr->left.get(), diagnostics, functions, locals, structFields) && ok;
    ok = checkExpressionCalls(function, expr->right.get(), diagnostics, functions, locals, structFields) && ok;
    return ok;
}

bool checkCalls(
    const FunctionDecl& function,
    const std::vector<StatementPtr>& body,
    Diagnostics& diagnostics,
    const std::unordered_map<std::string, FunctionInfo>& functions,
    const std::unordered_map<std::string, LocalInfo>& locals,
    const StructFieldMap& structFields) {
    bool ok = true;
    for (const auto& statement : body) {
        if (!statement) {
            continue;
        }
        const bool isCall = statement->kind == StatementKind::AuthorizeCall || statement->kind == StatementKind::AssignFromCall;
        if (isCall) {
            ok = checkAuthorizeCall(
                     function,
                     statement->authorizeCall,
                     statement->location,
                     statement->kind == StatementKind::AssignFromCall,
                     diagnostics,
                     functions,
                     locals,
                     structFields) &&
                ok;
            for (const auto& arg : statement->authorizeCall.arguments) {
                ok = checkExpressionCalls(function, arg.value.get(), diagnostics, functions, locals, structFields) && ok;
            }
        }
        ok = checkExpressionCalls(function, statement->expression.get(), diagnostics, functions, locals, structFields) && ok;
        ok = checkExpressionCalls(function, statement->initializer.get(), diagnostics, functions, locals, structFields) && ok;
        ok = checkExpressionCalls(function, statement->increment.get(), diagnostics, functions, locals, structFields) && ok;
        ok = checkExpressionCalls(function, statement->assign.value.get(), diagnostics, functions, locals, structFields) && ok;
        if (statement->kind == StatementKind::VarDecl) {
            ok = checkExpressionCalls(function, statement->varDecl.initializer.get(), diagnostics, functions, locals, structFields) && ok;
        }
        if (statement->kind == StatementKind::FptrAssign) {
            const auto target = functions.find(statement->fptrAssign.target);
            if (target == functions.end()) {
                diagnostics.error(
                    "sema",
                    "unknown-fptr-target",
                    statement->location,
                    "unknown function pointer assignment target '" + statement->fptrAssign.target + "'");
                ok = false;
            }
        }
        ok = checkCalls(function, statement->thenBody, diagnostics, functions, locals, structFields) && ok;
        ok = checkCalls(function, statement->elseBody, diagnostics, functions, locals, structFields) && ok;
    }
    return ok;
}

} // namespace

bool checkProgramSemantics(const Program& program, Diagnostics& diagnostics) {
    bool ok = true;
    if (!program.requireEcc) {
        diagnostics.error("sema", "missing-ecc", SourceLocation{"<program>", 1, 1}, "source must start with require ecc;");
        ok = false;
    }

    std::unordered_set<std::string> functionNames;
    std::unordered_set<std::string> structNames;
    std::unordered_set<std::string> classNames;
    StructFieldMap structFields;
    for (const auto& structure : program.structs) {
        structNames.insert(structure.name);
        if (structure.isClass) {
            classNames.insert(structure.name);
        }
        auto& fields = structFields[structure.name];
        for (const auto& field : structure.fields) {
            fields[field.name] = normalized(field.type.text);
        }
    }
    std::unordered_map<std::string, FunctionInfo> functions;
    auto addBuiltin = [&](std::string name, bool returnable, FunctionCategory category, std::string returnType, std::vector<std::string> paramTypes) {
        functionNames.insert(name);
        const auto arity = paramTypes.size();
        functions.emplace(std::move(name), FunctionInfo{returnable, category, 0, 0, 0, arity, std::move(returnType), std::move(paramTypes), SourceLocation{"<builtin>", 1, 1}});
    };
    addBuiltin(std::string{vm::builtin::kOutputInt}, false, FunctionCategory::Callable, std::string{vm::source_literal::kVoidType}, {"int:64"});
    addBuiltin(std::string{vm::builtin::kOutputChar}, false, FunctionCategory::Callable, std::string{vm::source_literal::kVoidType}, {"char:8"});
    addBuiltin(std::string{vm::builtin::kPrintLine}, false, FunctionCategory::Callable, std::string{vm::source_literal::kVoidType}, {"char:8[]"});
    addBuiltin(std::string{vm::builtin::kInputInt}, true, FunctionCategory::Callable, "int:64", {});
    addBuiltin(std::string{vm::builtin::kInputChar}, true, FunctionCategory::Callable, "char:8", {});
    addBuiltin(std::string{vm::builtin::kAlwaysApprove}, true, FunctionCategory::Approval, "bool:1", {"char:8[]"});
    addBuiltin(std::string{vm::builtin::kAlwaysDeny}, true, FunctionCategory::Approval, "bool:1", {"char:8[]"});
    addBuiltin(std::string{vm::builtin::kVerifyMemoryIntegrity}, true, FunctionCategory::Callable, "bool:1", {});
    addBuiltin(std::string{vm::builtin::kCopyMemory}, false, FunctionCategory::Callable, std::string{vm::source_literal::kVoidType}, {"ptr<readablewritable,int:64>", "ptr<readable,int:64>", "int:64"});
    addBuiltin(std::string{vm::builtin::kCompareMemory}, true, FunctionCategory::Callable, "int:32", {"ptr<readable,int:64>", "ptr<readable,int:64>", "int:64"});

    for (const auto& function : program.functions) {
        if (!isValidType(function.returnType, structNames, function.returnType.text == vm::source_literal::kVoidType)) {
            diagnostics.error("sema", "invalid-return-type", function.location, "invalid return type '" + function.returnType.text + "'");
            ok = false;
        }
        for (const auto& param : function.params) {
            if (!isValidType(param.type, structNames, false)) {
                diagnostics.error("sema", "invalid-param-type", param.location, "invalid parameter type '" + param.type.text + "'");
                ok = false;
            }
        }
        if (!functionNames.insert(function.name).second) {
            diagnostics.error("sema", "duplicate-function", function.location, "duplicate function '" + function.name + "'");
            ok = false;
        }
        std::vector<std::string> paramTypes;
        for (const auto& param : function.params) {
            paramTypes.push_back(normalized(param.type.text));
        }
        functions[function.name] = FunctionInfo{
            function.returnable,
            function.category,
            function.securityLevel,
            function.maxStackSize,
            function.codeSize,
            function.params.size(),
            normalized(function.returnType.text),
            std::move(paramTypes),
            function.location,
        };
    }
    for (const auto& structure : program.structs) {
        std::unordered_set<std::string> fieldNames;
        if (structure.isClass) {
            if (structure.definitionAuthority.empty()) {
                diagnostics.error(
                    "sema",
                    "missing-class-authority",
                    structure.location,
                    "class definitions must declare who authorized them using authorized by <authority>");
                ok = false;
            }
            if (structure.declaredFieldCount < 0 || structure.declaredMethodCount < 0) {
                diagnostics.error(
                    "sema",
                    "missing-class-counts",
                    structure.location,
                    "class definitions must declare exact fields and methods counts");
                ok = false;
            }
            if (structure.declaredFieldCount >= 0 && structure.fields.size() != static_cast<std::size_t>(structure.declaredFieldCount)) {
                diagnostics.error(
                    "sema",
                    "field-count-mismatch",
                    structure.location,
                    "class declared " + std::to_string(structure.declaredFieldCount) + " fields but defines " +
                        std::to_string(structure.fields.size()));
                ok = false;
            }
            if (structure.declaredMethodCount >= 0 && structure.methods.size() != static_cast<std::size_t>(structure.declaredMethodCount)) {
                diagnostics.error(
                    "sema",
                    "method-count-mismatch",
                    structure.location,
                    "class declared " + std::to_string(structure.declaredMethodCount) + " methods but defines " +
                        std::to_string(structure.methods.size()));
                ok = false;
            }
            if (!isDeclaredLiteralExpr(structure.definitionReason.get())) {
                diagnostics.error(
                    "sema",
                    "missing-class-reason",
                    structure.location,
                    "class definitions must explain why they exist using because literal \"...\"");
                ok = false;
            }
            if (!hasAccess(structure.memoryAccess, std::string{vm::source_literal::kReadable})) {
                diagnostics.error(
                    "sema",
                    "class-memory-not-readable",
                    structure.location,
                    "class memory must explicitly be readable");
                ok = false;
            }
            if (structure.baseClass.empty() || !isDeclaredLiteralExpr(structure.inheritanceReason.get())) {
                diagnostics.error(
                    "sema",
                    "missing-inheritance-reason",
                    structure.location,
                    "class inheritance must name a base and explain why using because literal \"...\"");
                ok = false;
            }
            if (!structure.baseClass.empty() && structure.baseClass != vm::source_literal::kRootObject &&
                !structNames.contains(structure.baseClass)) {
                diagnostics.error(
                    "sema",
                    "unknown-base-class",
                    structure.location,
                    "base class '" + structure.baseClass + "' is not declared");
                ok = false;
            }
            const std::unordered_set<std::string> patterns(structure.patterns.begin(), structure.patterns.end());
            for (const auto* required : {
                     vm::class_pattern::kAbstractFactory.data(),
                     vm::class_pattern::kStrategy.data(),
                     vm::class_pattern::kDependencyInjection.data(),
                 }) {
                if (!patterns.contains(required)) {
                    diagnostics.error(
                        "sema",
                        "missing-class-pattern",
                        structure.location,
                        "class must declare design pattern '" + std::string(required) + "'");
                    ok = false;
                }
            }
            if (structure.dependencies.empty()) {
                diagnostics.error(
                    "sema",
                    "missing-class-dependency",
                    structure.location,
                    "class must declare at least one injected dependency");
                ok = false;
            }
        } else {
            if ((structure.declaredFieldCount >= 0 || structure.declaredMethodCount >= 0) && structure.definitionAuthority.empty()) {
                diagnostics.error(
                    "sema",
                    "missing-struct-authority",
                    structure.location,
                    "formal struct definitions must declare who authorized them using authorized by <authority>");
                ok = false;
            }
            if (structure.declaredFieldCount >= 0 && structure.fields.size() != static_cast<std::size_t>(structure.declaredFieldCount)) {
                diagnostics.error(
                    "sema",
                    "field-count-mismatch",
                    structure.location,
                    "struct declared " + std::to_string(structure.declaredFieldCount) + " fields but defines " +
                        std::to_string(structure.fields.size()));
                ok = false;
            }
            if (structure.declaredMethodCount >= 0 && structure.methods.size() != static_cast<std::size_t>(structure.declaredMethodCount)) {
                diagnostics.error(
                    "sema",
                    "method-count-mismatch",
                    structure.location,
                    "struct declared " + std::to_string(structure.declaredMethodCount) + " methods but defines " +
                        std::to_string(structure.methods.size()));
                ok = false;
            }
        }
        for (const auto& field : structure.fields) {
            if (!fieldNames.insert(field.name).second) {
                diagnostics.error("sema", "duplicate-field", field.location, "duplicate field '" + field.name + "'");
                ok = false;
            }
            if (!isValidType(field.type, structNames, false)) {
                diagnostics.error("sema", "invalid-field-type", field.location, "invalid field type '" + field.type.text + "'");
                ok = false;
            }
        }
        for (const auto& method : structure.methods) {
            std::vector<std::string> paramTypes;
            for (const auto& param : method.params) {
                paramTypes.push_back(normalized(param.type.text));
            }
            functions[structure.name + "." + method.name] = FunctionInfo{
                method.returnable,
                method.category,
                method.securityLevel,
                method.maxStackSize,
                method.codeSize,
                method.params.size(),
                normalized(method.returnType.text),
                std::move(paramTypes),
                method.location,
            };
        }
    }

    for (const auto& function : program.functions) {
        if (function.body.empty() || !isVerify(function.body.front())) {
            diagnostics.error(
                "sema",
                "missing-verifyfunctionidentity",
                function.location,
                "first executable statement in every function must be verifyfunctionidentity();");
            ok = false;
        }
        if (function.category == FunctionCategory::Approval && bodyContainsCall(function.body)) {
            diagnostics.error(
                "sema",
                "approval-calls",
                function.location,
                "approval functions cannot authorize or call other functions");
            ok = false;
        }
        if (function.category == FunctionCategory::Approval) {
            if (!function.returnable || function.returnType.text != "bool:1" || function.params.size() != 1 ||
                function.params.front().type.text != "char:8[]") {
                diagnostics.error(
                    "sema",
                    "bad-approval-signature",
                    function.location,
                    "approval functions must return bool:1 and take one char:8[] parameter");
                ok = false;
            }
        }
        if (function.declaredLocalCount >= 0) {
            if (function.definitionAuthority.empty()) {
                diagnostics.error(
                    "sema",
                    "missing-function-authority",
                    function.location,
                    "function must declare who authorized its definition using authorized by <authority>");
                ok = false;
            }
            const auto actualLocals = countLocalDeclarations(function.body);
            if (actualLocals != static_cast<std::size_t>(function.declaredLocalCount)) {
                diagnostics.error(
                    "sema",
                    "local-count-mismatch",
                    function.location,
                    "function declared " + std::to_string(function.declaredLocalCount) + " local variables but defines " +
                        std::to_string(actualLocals));
                ok = false;
            }
        }
        std::unordered_map<std::string, LocalInfo> locals;
        for (const auto& param : function.params) {
            locals[param.name] = LocalInfo{false, param.type.text, param.access, std::string{vm::source_literal::kComputational}, false};
        }
        ok = checkImmutableAssignments(function, function.body, diagnostics, locals, structNames, classNames, functions) && ok;
        ok = checkCalls(function, function.body, diagnostics, functions, locals, structFields) && ok;
    }

    static constexpr std::array requiredMethods = {
        "init", "destroy", "copy", "move", "serialize", "deserialize", "tostring", "fromstring", "hash", "compare",
        "equals", "getproperty", "setproperty", "invoke", "getsize", "alignof", "defaultvalue", "validate", "mutate",
        "clone",
    };
    for (const auto& structure : program.structs) {
        std::unordered_set<std::string> methods;
        for (const auto& method : structure.methods) {
            if (!methods.insert(method.name).second) {
                diagnostics.error("sema", "duplicate-method", method.location, "duplicate method '" + method.name + "'");
                ok = false;
            }
            if (!isValidType(method.returnType, structNames, method.returnType.text == vm::source_literal::kVoidType)) {
                diagnostics.error("sema", "invalid-return-type", method.location, "invalid method return type '" + method.returnType.text + "'");
                ok = false;
            }
            for (const auto& param : method.params) {
                if (!isValidType(param.type, structNames, false)) {
                    diagnostics.error("sema", "invalid-param-type", param.location, "invalid method parameter type '" + param.type.text + "'");
                    ok = false;
                }
            }
            if (method.body.empty() || !isVerify(method.body.front())) {
                diagnostics.error(
                    "sema",
                    "missing-method-verifyfunctionidentity",
                    method.location,
                    "first executable statement in every struct method must be verifyfunctionidentity();");
                ok = false;
            }
            if (method.category == FunctionCategory::Approval && bodyContainsCall(method.body)) {
                diagnostics.error("sema", "approval-calls", method.location, "approval methods cannot authorize or call other functions");
                ok = false;
            }
            if (method.declaredLocalCount >= 0) {
                if (method.definitionAuthority.empty()) {
                    diagnostics.error(
                        "sema",
                        "missing-function-authority",
                        method.location,
                        "method must declare who authorized its definition using authorized by <authority>");
                    ok = false;
                }
                const auto actualLocals = countLocalDeclarations(method.body);
                if (actualLocals != static_cast<std::size_t>(method.declaredLocalCount)) {
                    diagnostics.error(
                        "sema",
                        "local-count-mismatch",
                        method.location,
                        "method declared " + std::to_string(method.declaredLocalCount) + " local variables but defines " +
                            std::to_string(actualLocals));
                    ok = false;
                }
            } else if (structure.isClass) {
                diagnostics.error(
                    "sema",
                    "missing-local-count",
                    method.location,
                    "class methods must declare exact local variable count using locals <count>");
                ok = false;
            }
            std::unordered_map<std::string, LocalInfo> locals;
            for (const auto& param : method.params) {
                locals[param.name] = LocalInfo{false, param.type.text, param.access, std::string{vm::source_literal::kComputational}, false};
            }
            ok = checkImmutableAssignments(method, method.body, diagnostics, locals, structNames, classNames, functions) && ok;
            ok = checkCalls(method, method.body, diagnostics, functions, locals, structFields) && ok;
        }
        if (structure.isClass) {
            for (const auto* required : {
                     vm::class_method::kInit.data(),
                     vm::class_method::kDestroy.data(),
                     vm::class_method::kPrint.data(),
                 }) {
                if (!methods.contains(required)) {
                    diagnostics.error(
                        "sema",
                        "missing-class-method",
                        structure.location,
                        "class '" + structure.name + "' must implement method '" + std::string(required) + "'");
                    ok = false;
                }
            }
        } else {
            for (const auto* required : requiredMethods) {
                if (!methods.contains(required)) {
                    diagnostics.error(
                        "sema",
                        "missing-struct-method",
                        structure.location,
                        "struct '" + structure.name + "' must implement method '" + std::string(required) + "'");
                    ok = false;
                }
            }
        }
    }

    return ok && !diagnostics.hasErrors();
}

} // namespace torture::compiler
