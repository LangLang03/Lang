#include "compiler/bytecode_compiler.h"

#include <algorithm>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace torture::compiler {
namespace {

struct FunctionInfo {
    bool returnable = false;
    FunctionCategory category = FunctionCategory::Callable;
    std::size_t arity = 0;
};

constexpr int kDefaultMaxStackSize = 1024;
constexpr std::string_view kIntegerZero = "0";
constexpr std::string_view kIntegerOne = "1";
constexpr std::string_view kUnaryAddressOf = "&";
constexpr std::string_view kUnaryDereference = "*";
constexpr std::string_view kUnaryNot = "!";
constexpr std::string_view kBinaryAdd = "+";
constexpr std::string_view kBinarySubtract = "-";
constexpr std::string_view kBinaryMultiply = "*";
constexpr std::string_view kBinaryDivide = "/";
constexpr std::string_view kBinaryEqual = "==";
constexpr std::string_view kBinaryNotEqual = "!=";
constexpr std::string_view kBinaryLess = "<";
constexpr std::string_view kBinaryLessEqual = "<=";
constexpr std::string_view kBinaryGreater = ">";
constexpr std::string_view kBinaryGreaterEqual = ">=";
constexpr std::string_view kBinaryAnd = "&&";
constexpr std::string_view kBinaryOr = "||";
constexpr std::size_t kPendingContinueTarget = static_cast<std::size_t>(-1);

std::string categoryName(FunctionCategory category) {
    switch (category) {
    case FunctionCategory::Callable:
        return std::string{vm::function_category::kCallable};
    case FunctionCategory::Uncallable:
        return std::string{vm::function_category::kUncallable};
    case FunctionCategory::Approval:
        return std::string{vm::function_category::kApproval};
    }
    return std::string{vm::function_category::kCallable};
}

std::string hexEncode(const std::string& text) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto ch : text) {
        out << std::setw(2) << static_cast<int>(static_cast<unsigned char>(ch));
    }
    return out.str();
}

std::string normalized(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), ' '), text.end());
    return text;
}

bool isPointerLikeType(const std::string& type) {
    const auto text = normalized(type);
    return text.starts_with("ptr<") || text.starts_with("ref<");
}

std::string owned(std::string_view value) {
    return std::string{value};
}

bool isSymbol(const std::string& value, std::string_view symbol) {
    return std::string_view{value} == symbol;
}

bool isSymbol(std::string_view value, std::string_view symbol) {
    return value == symbol;
}

std::string authorityChainOrRoot(const std::string& chain) {
    return chain.empty() ? owned(vm::builtin::kRootAuthority) : chain;
}

vm::Opcode opcodeForOverflowPolicy(
    std::string_view policy,
    vm::Opcode trapOpcode,
    vm::Opcode wrapOpcode,
    vm::Opcode saturateOpcode,
    vm::Opcode extendOpcode,
    vm::Opcode defaultOpcode) {
    if (isSymbol(policy, vm::overflow_policy::kTrap)) {
        return trapOpcode;
    }
    if (isSymbol(policy, vm::overflow_policy::kWrap)) {
        return wrapOpcode;
    }
    if (isSymbol(policy, vm::overflow_policy::kSaturate)) {
        return saturateOpcode;
    }
    if (isSymbol(policy, vm::overflow_policy::kExtend)) {
        return extendOpcode;
    }
    return defaultOpcode;
}

std::optional<vm::Opcode> arithmeticOpcode(std::string_view sourceOperator, std::string_view policy) {
    if (isSymbol(sourceOperator, kBinaryAdd)) {
        return opcodeForOverflowPolicy(
            policy, vm::Opcode::kAddTrap, vm::Opcode::kAddWrap, vm::Opcode::kAddSaturate, vm::Opcode::kAddExtend, vm::Opcode::kAdd);
    }
    if (isSymbol(sourceOperator, kBinarySubtract)) {
        return opcodeForOverflowPolicy(
            policy, vm::Opcode::kSubTrap, vm::Opcode::kSubWrap, vm::Opcode::kSubSaturate, vm::Opcode::kSubExtend, vm::Opcode::kSub);
    }
    if (isSymbol(sourceOperator, kBinaryMultiply)) {
        return opcodeForOverflowPolicy(
            policy, vm::Opcode::kMulTrap, vm::Opcode::kMulWrap, vm::Opcode::kMulSaturate, vm::Opcode::kMulExtend, vm::Opcode::kMul);
    }
    if (isSymbol(sourceOperator, kBinaryDivide)) {
        return opcodeForOverflowPolicy(
            policy, vm::Opcode::kDivTrap, vm::Opcode::kDivWrap, vm::Opcode::kDivSaturate, vm::Opcode::kDivExtend, vm::Opcode::kDiv);
    }
    return std::nullopt;
}

class FunctionCompiler {
public:
    FunctionCompiler(
        const FunctionDecl& function,
        const std::unordered_map<std::string, FunctionInfo>& functionInfos,
        Diagnostics& diagnostics,
        std::string outputName = {})
        : function_(function), functionInfos_(functionInfos), diagnostics_(diagnostics) {
        out_.name = outputName.empty() ? function.name : std::move(outputName);
        out_.maxStackSize = function.maxStackSize > 0 ? function.maxStackSize : kDefaultMaxStackSize;
        out_.securityLevel = function.securityLevel;
        out_.returnable = function.returnable;
        out_.category = categoryName(function.category);
        out_.grantor = function.grantor;
        out_.allowedRoles = function.allowedRoles;
        out_.callableFrom = function.callableFrom;
        for (const auto& param : function.params) {
            out_.params.push_back(param.name);
            declareLocal(param.name);
            localTypes_[param.name] = normalized(param.type.text);
        }
    }

    std::optional<vm::FunctionBytecode> compile() {
        for (const auto& statement : function_.body) {
            compileStatement(*statement);
            if (diagnostics_.hasErrors()) {
                return std::nullopt;
            }
        }
        if (isSymbol(function_.name, vm::source_function::kMain)) {
            emit(vm::Opcode::kHalt);
        } else {
            emit(vm::Opcode::kReturn);
        }
        if (function_.codeSize > 0 && out_.code.size() > static_cast<std::size_t>(function_.codeSize)) {
            diagnostics_.error(
                "compiler",
                "code-size-exceeded",
                function_.location,
                "function '" + out_.name + "' emits " + std::to_string(out_.code.size()) +
                    " instructions, exceeding declared code size " + std::to_string(function_.codeSize));
            return std::nullopt;
        }
        vm::finalizeFunctionHash(out_);
        return out_;
    }

private:
    struct LoopContext {
        std::size_t continueTarget = 0;
        std::vector<std::size_t> breakJumps;
        std::vector<std::size_t> continueJumps;
    };

    struct LoopFormalState {
        std::vector<const FormalClause*> invariants;
        std::vector<std::pair<const FormalClause*, std::string>> decreases;
    };

    void declareLocal(const std::string& name) {
        if (locals_.insert(name).second) {
            out_.locals.push_back(name);
        }
    }

    std::size_t emit(vm::Opcode opcode, std::vector<std::string> args = {}) {
        out_.code.push_back(vm::makeInstruction(opcode, std::move(args)));
        return out_.code.size() - 1;
    }

    void patch(std::size_t index, std::size_t target) {
        out_.code[index].args = {std::to_string(target)};
    }

    void compileStatement(const Statement& statement) {
        switch (statement.kind) {
        case StatementKind::VerifyFunctionIdentity:
            emit(vm::Opcode::kVerify);
            break;
        case StatementKind::VarDecl:
            declareLocal(statement.varDecl.name);
            localTypes_[statement.varDecl.name] = normalized(statement.varDecl.type.text);
            if (statement.varDecl.initializer) {
                compileExpr(*statement.varDecl.initializer);
            } else {
                emit(vm::Opcode::kPushInteger, {owned(kIntegerZero)});
            }
            emit(vm::Opcode::kStore, {statement.varDecl.name});
            break;
        case StatementKind::Assign:
            compileAssign(statement);
            break;
        case StatementKind::AssignFromCall:
            compileAssignFromCall(statement);
            break;
        case StatementKind::Return:
            if (statement.expression) {
                compileExpr(*statement.expression);
            } else {
                emit(vm::Opcode::kPushInteger, {owned(kIntegerZero)});
            }
            emit(vm::Opcode::kReturn);
            break;
        case StatementKind::AuthorizeCall:
            compileAuthorize(statement.authorizeCall, &statement.apply);
            break;
        case StatementKind::FptrAssign:
            declareLocal(statement.fptrAssign.variable);
            emit(vm::Opcode::kStorePointer, {statement.fptrAssign.variable, statement.fptrAssign.target});
            break;
        case StatementKind::Proof:
            compileProof(statement.proof);
            break;
        case StatementKind::Gate:
            compileExpr(*statement.expression);
            emit(vm::Opcode::kPop);
            break;
        case StatementKind::OperatorInput:
            declareLocal(statement.identifier);
            emit(vm::Opcode::kOperatorInput, {statement.identifier});
            break;
        case StatementKind::Role:
            emit(vm::Opcode::kRole, {statement.role.op, statement.role.role, statement.role.principal, statement.role.chain});
            break;
        case StatementKind::If:
            compileExpr(*statement.expression);
            {
                const auto jumpElse = emit(vm::Opcode::kJumpIfZero, {owned(kIntegerZero)});
                for (const auto& child : statement.thenBody) {
                    compileStatement(*child);
                }
                const auto jumpEnd = emit(vm::Opcode::kJump, {owned(kIntegerZero)});
                patch(jumpElse, out_.code.size());
                for (const auto& child : statement.elseBody) {
                    compileStatement(*child);
                }
                patch(jumpEnd, out_.code.size());
            }
            break;
        case StatementKind::While:
            compileWhile(statement);
            break;
        case StatementKind::DoUntil:
            compileDoUntil(statement);
            break;
        case StatementKind::For:
            compileFor(statement);
            break;
        case StatementKind::Break:
            if (loopStack_.empty()) {
                diagnostics_.error("compiler", "break-outside-loop", statement.location, "break must appear inside a loop");
                break;
            }
            loopStack_.back().breakJumps.push_back(emit(vm::Opcode::kJump, {owned(kIntegerZero)}));
            break;
        case StatementKind::Continue:
            if (loopStack_.empty()) {
                diagnostics_.error("compiler", "continue-outside-loop", statement.location, "continue must appear inside a loop");
                break;
            }
            if (loopStack_.back().continueTarget == kPendingContinueTarget) {
                loopStack_.back().continueJumps.push_back(emit(vm::Opcode::kJump, {owned(kIntegerZero)}));
            } else {
                emit(vm::Opcode::kJump, {std::to_string(loopStack_.back().continueTarget)});
            }
            break;
        case StatementKind::ClockCycle:
        case StatementKind::YieldGate:
            break;
        case StatementKind::Instantiate:
            declareLocal(statement.instantiate.name);
            localTypes_[statement.instantiate.name] = normalized(statement.instantiate.className);
            emit(vm::Opcode::kPushInteger, {owned(kIntegerZero)});
            emit(vm::Opcode::kStore, {statement.instantiate.name});
            break;
        case StatementKind::Unknown:
            break;
        }
    }

    void compileAssign(const Statement& statement) {
        if (statement.assign.target && statement.assign.target->kind == ExprKind::Unary &&
            isSymbol(statement.assign.target->text, kUnaryDereference)) {
            if (!compileLvalueRef(*statement.assign.target)) {
                diagnostics_.error("compiler", "unsupported-lvalue", statement.location, "assignment target is not supported");
                return;
            }
            compileExpr(*statement.assign.value);
            emit(vm::Opcode::kStoreDereference);
            return;
        }
        const auto target = lvalueName(statement.assign.target.get());
        if (!target) {
            diagnostics_.error("compiler", "unsupported-lvalue", statement.location, "assignment target is not supported");
            return;
        }
        compileExpr(*statement.assign.value);
        declareLocal(*target);
        emit(vm::Opcode::kStore, {*target});
    }

    void compileAssignFromCall(const Statement& statement) {
        if (statement.assign.target && statement.assign.target->kind == ExprKind::Unary &&
            isSymbol(statement.assign.target->text, kUnaryDereference)) {
            if (!compileLvalueRef(*statement.assign.target)) {
                diagnostics_.error("compiler", "unsupported-lvalue", statement.location, "assignment target is not supported for call results");
                return;
            }
            compileAuthorize(statement.authorizeCall, &statement.apply);
            emit(vm::Opcode::kStoreDereference);
            return;
        }
        const auto target = lvalueName(statement.assign.target.get());
        if (!target) {
            diagnostics_.error("compiler", "unsupported-lvalue", statement.location, "assignment target is not supported for call results");
            return;
        }
        compileAuthorize(statement.authorizeCall, &statement.apply);
        declareLocal(*target);
        emit(vm::Opcode::kStore, {*target});
    }

    void compileAuthorize(const AuthorizeCall& call, const ApplyStatement* applyInfo = nullptr) {
        // FFI `apply external <bindname>` 走专用 Apply 字节码。
        if (call.target.starts_with("apply.")) {
            const auto bindName = call.target.substr(6);
            std::vector<std::string> args;
            args.push_back(bindName);
            if (applyInfo != nullptr) {
                args.push_back(applyInfo->architecture);
                args.push_back(applyInfo->system);
                args.push_back(applyInfo->libraryPath);
                args.push_back(applyInfo->symbol);
                args.push_back(applyInfo->receivingReturnInto);
                args.push_back(applyInfo->justification ? applyInfo->justification->text : std::string{});
                args.push_back(applyInfo->approvalJustification ? applyInfo->approvalJustification->text : std::string{});
                args.push_back(std::to_string(applyInfo->approvalTimeout));
            } else {
                args.push_back("");
                args.push_back("");
                args.push_back("");
                args.push_back("");
                args.push_back("");
                args.push_back("");
                args.push_back("");
                args.push_back("0");
            }
            args.push_back(std::to_string(call.securityLevel));
            args.push_back(std::to_string(call.memoryLimit));
            args.push_back(std::to_string(call.timeout));
            args.push_back(std::to_string(call.predictStackDepth));
            args.push_back(call.authorityChain);
            args.push_back(call.approvalFunction.value_or(""));
            args.push_back(call.discardingReturn ? owned(kIntegerOne) : owned(kIntegerZero));
            args.push_back(call.fromNamespace);
            for (const auto& arg : call.arguments) {
                compileExpr(*arg.value);
            }
            args.push_back(std::to_string(call.arguments.size()));
            emit(vm::Opcode::kApply, std::move(args));
            return;
        }
        const auto resolvedTarget = resolveCallTarget(call.target);
        const auto approval = call.approvalFunction.value_or("");
        emit(vm::Opcode::kCheckApproval, {approval, resolvedTarget});
        emit(vm::Opcode::kCheckLimits, {
            std::to_string(call.memoryLimit),
            std::to_string(call.timeout),
            std::to_string(call.predictStackDepth),
        });
        if (call.viaPointer) {
            if (call.hasNullCheck && call.nullCheck) {
                emit(vm::Opcode::kNullCheck, {call.target});
            }
            for (const auto& arg : call.arguments) {
                compileExpr(*arg.value);
            }
            emit(vm::Opcode::kCheckRoleIndirect, {call.target, authorityChainOrRoot(call.authorityChain)});
            emit(vm::Opcode::kCallIndirect, {
                call.target,
                std::to_string(call.arguments.size()),
                call.discardingReturn ? owned(kIntegerOne) : owned(kIntegerZero),
            });
            return;
        }
        if (isSymbol(resolvedTarget, vm::builtin::kOutputInt)) {
            if (call.arguments.empty()) {
                diagnostics_.error("compiler", "missing-output-arg", call.location, "outputint requires one argument");
                return;
            }
            compileExpr(*call.arguments.front().value);
            emit(vm::Opcode::kOutputInt);
            return;
        }
        if (isSymbol(resolvedTarget, vm::builtin::kOutputChar)) {
            if (call.arguments.empty()) {
                diagnostics_.error("compiler", "missing-output-arg", call.location, "outputchar requires one argument");
                return;
            }
            compileExpr(*call.arguments.front().value);
            emit(vm::Opcode::kOutputChar);
            return;
        }
        if (isSymbol(resolvedTarget, vm::builtin::kPrintLine)) {
            if (call.arguments.empty()) {
                diagnostics_.error("compiler", "missing-println-arg", call.location, "println requires one argument");
                return;
            }
            compileExpr(*call.arguments.front().value);
            emit(vm::Opcode::kPrintLine);
            return;
        }
        if (isSymbol(resolvedTarget, vm::builtin::kInputInt)) {
            emit(vm::Opcode::kInputInt);
            if (call.discardingReturn) {
                emit(vm::Opcode::kPop);
            }
            return;
        }
        if (isSymbol(resolvedTarget, vm::builtin::kInputChar)) {
            emit(vm::Opcode::kInputChar);
            if (call.discardingReturn) {
                emit(vm::Opcode::kPop);
            }
            return;
        }
        if (isSymbol(resolvedTarget, vm::builtin::kVerifyMemoryIntegrity)) {
            emit(vm::Opcode::kVerifyEcc);
            if (!call.discardingReturn) {
                emit(vm::Opcode::kPushInteger, {owned(kIntegerOne)});
            }
            return;
        }
        if (isSymbol(resolvedTarget, vm::builtin::kCopyMemory)) {
            for (const auto& arg : call.arguments) {
                compileExpr(*arg.value);
            }
            emit(vm::Opcode::kCopyMemory);
            return;
        }
        if (isSymbol(resolvedTarget, vm::builtin::kCompareMemory)) {
            for (const auto& arg : call.arguments) {
                compileExpr(*arg.value);
            }
            emit(vm::Opcode::kCompareMemory);
            if (call.discardingReturn) {
                emit(vm::Opcode::kPop);
            }
            return;
        }
        const auto found = functionInfos_.find(resolvedTarget);
        if (found == functionInfos_.end() && !resolvedTarget.starts_with("std::")) {
            diagnostics_.error("compiler", "unknown-call-target", call.location, "unknown function '" + resolvedTarget + "'");
            return;
        }
        if (found != functionInfos_.end() && call.arguments.size() != found->second.arity) {
            diagnostics_.error(
                "compiler",
                "wrong-argument-count",
                call.location,
                "function '" + resolvedTarget + "' expects " + std::to_string(found->second.arity) + " arguments but got " +
                    std::to_string(call.arguments.size()));
            return;
        }
        if (found == functionInfos_.end()) {
            // std:: 调用的 stub：不发 CheckRole/Call，发一个 PUSHI 0 当作占位，
            // 这样字节码不会因找不到目标而崩溃。运行期 VM 会再校验 std binding。
            for (const auto& arg : call.arguments) {
                compileExpr(*arg.value);
            }
            emit(vm::Opcode::kPop, {});
            if (!call.discardingReturn) {
                emit(vm::Opcode::kPushInteger, {owned(kIntegerZero)});
            }
            return;
        }
        for (const auto& arg : call.arguments) {
            compileExpr(*arg.value);
        }
        emit(vm::Opcode::kCheckRole, {resolvedTarget, authorityChainOrRoot(call.authorityChain)});
        emit(vm::Opcode::kCall, {resolvedTarget, std::to_string(call.arguments.size())});
        if (found->second.returnable && call.discardingReturn) {
            emit(vm::Opcode::kPop);
        }
        return;
    }

    void compileExpr(const Expr& expr) {
        switch (expr.kind) {
        case ExprKind::Integer:
            emit(vm::Opcode::kPushInteger, {expr.text});
            break;
        case ExprKind::Boolean:
            emit(vm::Opcode::kPushInteger, {isSymbol(expr.text, vm::source_literal::kTrue) ? owned(kIntegerOne) : owned(kIntegerZero)});
            break;
        case ExprKind::Identifier:
            declareLocal(expr.text);
            emit(vm::Opcode::kLoad, {expr.text});
            break;
        case ExprKind::Compute:
            {
                const auto previous = overflowPolicy_;
                overflowPolicy_ = expr.text;
                compileExpr(*expr.left);
                overflowPolicy_ = previous;
            }
            break;
        case ExprKind::Unary:
            compileUnary(expr);
            break;
        case ExprKind::Binary:
            compileBinary(expr);
            break;
        case ExprKind::Conditional:
            compileConditional(expr);
            break;
        case ExprKind::Gate:
            compileGate(expr);
            break;
        case ExprKind::AuthorizeCall:
            compileAuthorize(expr.authorizeCall);
            break;
        case ExprKind::String:
            emit(vm::Opcode::kPushString, {hexEncode(expr.text)});
            break;
        case ExprKind::Null:
            emit(vm::Opcode::kPushNull);
            break;
        case ExprKind::Field:
            if (fieldBaseIsPointer(expr)) {
                if (!compilePointerRef(expr)) {
                    diagnostics_.error("compiler", "unsupported-expression", expr.location, "field expression is not supported");
                    return;
                }
                emit(vm::Opcode::kDereference);
            } else if (const auto name = lvalueName(&expr)) {
                declareLocal(*name);
                emit(vm::Opcode::kLoad, {*name});
            } else {
                diagnostics_.error("compiler", "unsupported-expression", expr.location, "field expression is not supported");
            }
            break;
        }
    }

    void compileUnary(const Expr& expr) {
        if (isSymbol(expr.text, kUnaryNot) || isSymbol(expr.text, vm::gate_operation::kNot)) {
            compileExpr(*expr.left);
            emit(vm::Opcode::kNot);
            return;
        }
        if (isSymbol(expr.text, kUnaryAddressOf) || isSymbol(expr.text, kUnaryDereference)) {
            if (isSymbol(expr.text, kUnaryAddressOf)) {
                if (expr.left && compileLvalueRef(*expr.left)) {
                    return;
                }
            }
            if (isSymbol(expr.text, kUnaryDereference) && expr.left && compilePointerRef(*expr.left)) {
                emit(vm::Opcode::kDereference);
                return;
            }
            compileExpr(*expr.left);
            if (isSymbol(expr.text, kUnaryDereference)) {
                emit(vm::Opcode::kDereference);
            }
            return;
        }
        diagnostics_.error("compiler", "unsupported-unary", expr.location, "unsupported unary operator '" + expr.text + "'");
    }

    void compileBinary(const Expr& expr) {
        compileExpr(*expr.left);
        compileExpr(*expr.right);
        const auto op = expr.text;
        if (const auto opcode = arithmeticOpcode(op, overflowPolicy_)) {
            emit(*opcode);
        } else if (isSymbol(op, kBinaryEqual)) {
            emit(vm::Opcode::kEqual);
        } else if (isSymbol(op, kBinaryNotEqual)) {
            emit(vm::Opcode::kNotEqual);
        } else if (isSymbol(op, kBinaryLess)) {
            emit(vm::Opcode::kLess);
        } else if (isSymbol(op, kBinaryLessEqual)) {
            emit(vm::Opcode::kLessEqual);
        } else if (isSymbol(op, kBinaryGreater)) {
            emit(vm::Opcode::kGreater);
        } else if (isSymbol(op, kBinaryGreaterEqual)) {
            emit(vm::Opcode::kGreaterEqual);
        } else if (isSymbol(op, kBinaryAnd) || isSymbol(op, vm::gate_operation::kAnd)) {
            emit(vm::Opcode::kAnd);
        } else if (isSymbol(op, kBinaryOr) || isSymbol(op, vm::gate_operation::kOr)) {
            emit(vm::Opcode::kOr);
        } else {
            diagnostics_.error("compiler", "unsupported-binary", expr.location, "unsupported binary operator '" + op + "'");
        }
    }

    void compileWhile(const Statement& statement) {
        auto formalState = compileLoopEntryFormalClauses(statement);
        const auto loopStart = out_.code.size();
        compileExpr(*statement.expression);
        const auto jumpEnd = emit(vm::Opcode::kJumpIfZero, {owned(kIntegerZero)});
        loopStack_.push_back(LoopContext{kPendingContinueTarget, {}, {}});
        for (const auto& child : statement.thenBody) {
            compileStatement(*child);
        }
        const auto iterationCheckStart = out_.code.size();
        auto continues = std::move(loopStack_.back().continueJumps);
        for (const auto jump : continues) {
            patch(jump, iterationCheckStart);
        }
        compileLoopIterationFormalClauses(formalState);
        emit(vm::Opcode::kJump, {std::to_string(loopStart)});
        patch(jumpEnd, out_.code.size());
        const auto breaks = std::move(loopStack_.back().breakJumps);
        loopStack_.pop_back();
        for (const auto jump : breaks) {
            patch(jump, out_.code.size());
        }
    }

    void compileDoUntil(const Statement& statement) {
        auto formalState = compileLoopEntryFormalClauses(statement);
        const auto loopStart = out_.code.size();
        loopStack_.push_back(LoopContext{kPendingContinueTarget, {}, {}});
        for (const auto& child : statement.thenBody) {
            compileStatement(*child);
        }
        const auto conditionStart = out_.code.size();
        auto continues = std::move(loopStack_.back().continueJumps);
        for (const auto jump : continues) {
            patch(jump, conditionStart);
        }
        compileLoopIterationFormalClauses(formalState);
        compileExpr(*statement.expression);
        emit(vm::Opcode::kJumpIfZero, {std::to_string(loopStart)});
        const auto breaks = std::move(loopStack_.back().breakJumps);
        loopStack_.pop_back();
        for (const auto jump : breaks) {
            patch(jump, out_.code.size());
        }
    }

    void compileFor(const Statement& statement) {
        if (statement.initializer) {
            compileExpr(*statement.initializer);
            emit(vm::Opcode::kPop);
        }
        auto formalState = compileLoopEntryFormalClauses(statement);
        const auto loopStart = out_.code.size();
        std::optional<std::size_t> jumpEnd;
        if (statement.expression) {
            compileExpr(*statement.expression);
            jumpEnd = emit(vm::Opcode::kJumpIfZero, {owned(kIntegerZero)});
        }
        loopStack_.push_back(LoopContext{kPendingContinueTarget, {}, {}});
        for (const auto& child : statement.thenBody) {
            compileStatement(*child);
        }
        const auto incrementStart = out_.code.size();
        auto continues = std::move(loopStack_.back().continueJumps);
        for (const auto jump : continues) {
            patch(jump, incrementStart);
        }
        if (statement.increment) {
            compileExpr(*statement.increment);
            emit(vm::Opcode::kPop);
        }
        compileLoopIterationFormalClauses(formalState);
        emit(vm::Opcode::kJump, {std::to_string(loopStart)});
        if (jumpEnd) {
            patch(*jumpEnd, out_.code.size());
        }
        const auto breaks = std::move(loopStack_.back().breakJumps);
        loopStack_.pop_back();
        for (const auto jump : breaks) {
            patch(jump, out_.code.size());
        }
    }

    void compileProof(const ProofStmt& proof) {
        const auto label = proof.form + " " + proof.name;
        if (proof.form == "theorem" && proof.premise) {
            compileExpr(*proof.premise);
            const auto skipClaim = emit(vm::Opcode::kJumpIfZero, {owned(kIntegerZero)});
            compileAssertedExpr(*proof.claim, label);
            patch(skipClaim, out_.code.size());
            return;
        }
        compileAssertedExpr(*proof.claim, label);
    }

    void compileAssertedExpr(const Expr& expr, const std::string& label) {
        compileExpr(expr);
        emit(vm::Opcode::kAssert, {label});
    }

    LoopFormalState compileLoopEntryFormalClauses(const Statement& statement) {
        LoopFormalState state;
        for (const auto& clause : statement.formalClauses) {
            const auto label = loopClauseLabel(clause);
            if (isSymbol(clause.kind, "invariant")) {
                compileAssertedExpr(*clause.expression, label);
                state.invariants.push_back(&clause);
                continue;
            }
            if (isSymbol(clause.kind, "decreases")) {
                const auto previous = "__decreases" + std::to_string(tempIndex_++);
                declareLocal(previous);
                compileExpr(*clause.expression);
                emit(vm::Opcode::kStore, {previous});
                compileStoredNonNegative(previous, label);
                state.decreases.push_back({&clause, previous});
            }
        }
        return state;
    }

    void compileLoopIterationFormalClauses(const LoopFormalState& state) {
        for (const auto* invariant : state.invariants) {
            compileAssertedExpr(*invariant->expression, loopClauseLabel(*invariant));
        }
        for (const auto& [clause, previous] : state.decreases) {
            const auto current = "__decreases" + std::to_string(tempIndex_++);
            declareLocal(current);
            compileExpr(*clause->expression);
            emit(vm::Opcode::kStore, {current});
            compileStoredNonNegative(current, loopClauseLabel(*clause));
            emit(vm::Opcode::kLoad, {current});
            emit(vm::Opcode::kLoad, {previous});
            emit(vm::Opcode::kLess);
            emit(vm::Opcode::kAssert, {"strictly decreasing " + loopClauseLabel(*clause)});
            emit(vm::Opcode::kLoad, {current});
            emit(vm::Opcode::kStore, {previous});
        }
    }

    void compileStoredNonNegative(const std::string& local, const std::string& label) {
        emit(vm::Opcode::kLoad, {local});
        emit(vm::Opcode::kPushInteger, {owned(kIntegerZero)});
        emit(vm::Opcode::kGreaterEqual);
        emit(vm::Opcode::kAssert, {"non-negative " + label});
    }

    std::string loopClauseLabel(const FormalClause& clause) const {
        return clause.kind + "@" + std::to_string(clause.location.line) + ":" + std::to_string(clause.location.column);
    }

    void compileConditional(const Expr& expr) {
        compileExpr(*expr.left);
        const auto jumpFalse = emit(vm::Opcode::kJumpIfZero, {owned(kIntegerZero)});
        compileExpr(*expr.right);
        const auto jumpEnd = emit(vm::Opcode::kJump, {owned(kIntegerZero)});
        patch(jumpFalse, out_.code.size());
        compileExpr(*expr.third);
        patch(jumpEnd, out_.code.size());
    }

    void compileGate(const Expr& expr) {
        std::unordered_map<std::string, std::string> wires;
        auto loadWire = [&](const std::string& wire) {
            const auto found = wires.find(wire);
            if (found != wires.end()) {
                emit(vm::Opcode::kLoad, {found->second});
                return;
            }
            declareLocal(wire);
            emit(vm::Opcode::kLoad, {wire});
        };
        auto storeWire = [&](const std::string& wire) {
            auto& local = wires[wire];
            if (local.empty()) {
                local = "__gate" + std::to_string(tempIndex_++) + "_" + wire;
                declareLocal(local);
            }
            emit(vm::Opcode::kStore, {local});
        };

        for (const auto& op : expr.gateOps) {
            if (isSymbol(op.op, vm::gate_operation::kNot)) {
                loadWire(op.left);
                emit(vm::Opcode::kNot);
                storeWire(op.output);
                continue;
            }

            loadWire(op.left);
            loadWire(op.right);
            if (isSymbol(op.op, vm::gate_operation::kAnd)) {
                emit(vm::Opcode::kAnd);
            } else if (isSymbol(op.op, vm::gate_operation::kOr)) {
                emit(vm::Opcode::kOr);
            } else if (isSymbol(op.op, vm::gate_operation::kXor)) {
                emit(vm::Opcode::kXor);
            } else if (isSymbol(op.op, vm::gate_operation::kNand)) {
                emit(vm::Opcode::kNand);
            } else if (isSymbol(op.op, vm::gate_operation::kNor)) {
                emit(vm::Opcode::kNor);
            } else {
                diagnostics_.error("compiler", "unsupported-gate-op", op.location, "unsupported gate operation '" + op.op + "'");
                return;
            }
            storeWire(op.output);
        }

        const auto found = wires.find(expr.text);
        if (found != wires.end()) {
            emit(vm::Opcode::kLoad, {found->second});
        } else {
            declareLocal(expr.text);
            emit(vm::Opcode::kLoad, {expr.text});
        }
    }

    std::optional<std::string> lvalueName(const Expr* expr) const {
        if (expr == nullptr) {
            return std::nullopt;
        }
        if (expr->kind == ExprKind::Identifier) {
            return expr->text;
        }
        if (expr->kind == ExprKind::Field) {
            auto base = lvalueName(expr->left.get());
            if (!base) {
                return std::nullopt;
            }
            return *base + "." + expr->text;
        }
        if (expr->kind == ExprKind::Unary && isSymbol(expr->text, kUnaryAddressOf)) {
            return lvalueName(expr->left.get());
        }
        return std::nullopt;
    }

    bool fieldBaseIsPointer(const Expr& expr) const {
        if (expr.kind != ExprKind::Field || expr.left == nullptr || expr.left->kind != ExprKind::Identifier) {
            return false;
        }
        const auto found = localTypes_.find(expr.left->text);
        return found != localTypes_.end() && isPointerLikeType(found->second);
    }

    bool compilePointerRef(const Expr& expr) {
        if (expr.kind == ExprKind::Field && fieldBaseIsPointer(expr)) {
            compileExpr(*expr.left);
            emit(vm::Opcode::kFieldReference, {expr.text});
            return true;
        }
        compileExpr(expr);
        return true;
    }

    bool compileLvalueRef(const Expr& expr) {
        if (expr.kind == ExprKind::Unary && isSymbol(expr.text, kUnaryDereference) && expr.left) {
            return compilePointerRef(*expr.left);
        }
        if (const auto name = lvalueName(&expr)) {
            declareLocal(*name);
            emit(vm::Opcode::kPushReference, {*name});
            return true;
        }
        return false;
    }

    std::string resolveCallTarget(const std::string& target) const {
        const auto dot = target.find('.');
        if (dot == std::string::npos) {
            return target;
        }
        const auto base = target.substr(0, dot);
        const auto method = target.substr(dot + 1);
        if (const auto found = localTypes_.find(base); found != localTypes_.end()) {
            return found->second + "." + method;
        }
        return target;
    }

    const FunctionDecl& function_;
    const std::unordered_map<std::string, FunctionInfo>& functionInfos_;
    Diagnostics& diagnostics_;
    vm::FunctionBytecode out_;
    std::unordered_set<std::string> locals_;
    std::unordered_map<std::string, std::string> localTypes_;
    int tempIndex_ = 0;
    std::vector<LoopContext> loopStack_;
    std::string overflowPolicy_;
};

} // namespace

std::optional<vm::BytecodeProgram> compileToBytecode(const Program& program, Diagnostics& diagnostics) {
    vm::BytecodeProgram bytecode;
    bytecode.environmentFingerprint = vm::currentEnvironmentFingerprint();
    bytecode.producer = vm::environmentSummary();
    bool hasMain = false;
    std::unordered_map<std::string, FunctionInfo> functionInfos;
    functionInfos.emplace(owned(vm::builtin::kOutputInt), FunctionInfo{false, FunctionCategory::Callable, 1});
    functionInfos.emplace(owned(vm::builtin::kOutputChar), FunctionInfo{false, FunctionCategory::Callable, 1});
    functionInfos.emplace(owned(vm::builtin::kPrintLine), FunctionInfo{false, FunctionCategory::Callable, 1});
    functionInfos.emplace(owned(vm::builtin::kInputInt), FunctionInfo{true, FunctionCategory::Callable, 0});
    functionInfos.emplace(owned(vm::builtin::kInputChar), FunctionInfo{true, FunctionCategory::Callable, 0});
    functionInfos.emplace(owned(vm::builtin::kVerifyMemoryIntegrity), FunctionInfo{true, FunctionCategory::Callable, 0});
    functionInfos.emplace(owned(vm::builtin::kCopyMemory), FunctionInfo{false, FunctionCategory::Callable, 3});
    functionInfos.emplace(owned(vm::builtin::kCompareMemory), FunctionInfo{true, FunctionCategory::Callable, 3});
    functionInfos.emplace(owned(vm::builtin::kAlwaysApprove), FunctionInfo{true, FunctionCategory::Approval, 1});
    functionInfos.emplace(owned(vm::builtin::kAlwaysDeny), FunctionInfo{true, FunctionCategory::Approval, 1});
    for (const auto& function : program.functions) {
        functionInfos[function.name] = FunctionInfo{function.returnable, function.category, function.params.size()};
    }
    for (const auto& structure : program.structs) {
        for (const auto& method : structure.methods) {
            functionInfos[structure.name + "." + method.name] = FunctionInfo{method.returnable, method.category, method.params.size()};
        }
    }

    for (const auto& function : program.functions) {
        if (isSymbol(function.name, vm::source_function::kMain)) {
            hasMain = true;
        }
        FunctionCompiler compiler(function, functionInfos, diagnostics);
        auto compiled = compiler.compile();
        if (!compiled) {
            return std::nullopt;
        }
        bytecode.functions.push_back(std::move(*compiled));
    }
    for (const auto& structure : program.structs) {
        for (const auto& method : structure.methods) {
            FunctionCompiler compiler(method, functionInfos, diagnostics, structure.name + "." + method.name);
            auto compiled = compiler.compile();
            if (!compiled) {
                return std::nullopt;
            }
            bytecode.functions.push_back(std::move(*compiled));
        }
    }
    if (!hasMain) {
        diagnostics.error("compiler", "missing-main", SourceLocation{"<program>", 1, 1}, "program must define main");
        return std::nullopt;
    }
    // 把 FFI 外部绑定表复制到字节码。
    for (const auto& ext : program.externalDecls) {
        vm::BytecodeProgram::ExternalBinding binding;
        binding.name = ext.bindName;
        binding.arch = ext.arch;
        binding.sys = ext.sys;
        binding.libPath = ext.libPath;
        binding.symbol = ext.symbol;
        binding.sha512Chain = ext.sha512Chain;
        bytecode.externalBindings.push_back(std::move(binding));
    }
    return bytecode;
}

} // namespace torture::compiler
