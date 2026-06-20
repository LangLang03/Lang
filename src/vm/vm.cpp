#include "vm/vm.h"
#include "vm/instruction_handler.h"
#include "vm/memory.h"
#include "vm/value.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace torture::vm {
namespace {

using Int = __int128_t;

constexpr std::string_view kInstructionTrue = "1";

const FunctionBytecode* findFunction(const BytecodeProgram& program, const std::string& name) {
    for (const auto& function : program.functions) {
        if (function.name == name) {
            return &function;
        }
    }
    return nullptr;
}

} // namespace

// 前向声明处理器类，用于友元声明
class StackHandler;
class ArithmeticHandler;
class ControlFlowHandler;
class IoHandler;
class SecurityHandler;

// VM 解释器类，负责执行字节码程序
class Interpreter {
public:
    Interpreter(const BytecodeProgram& program, std::istream& input, std::ostream& output, Diagnostics& diagnostics, VmOptions options)
        : program_(program), input_(input), output_(output), diagnostics_(diagnostics), options_(options) {
        chains_[std::string{builtin::kRootAuthority}].insert("*");
    }

    bool run() {
        if (program_.platformId != kPlatformId) {
            diagnostics_.error(
                std::string{bytecode_format::kDiagnosticDomain},
                std::string{bytecode_diagnostic::kPlatformMismatch},
                SourceLocation{"<bytecode>", 1, 1},
                "bytecode platform id " + std::to_string(static_cast<unsigned>(program_.platformId)) +
                    " does not match current platform " + std::to_string(static_cast<unsigned>(kPlatformId)) +
                    " ('" + std::string{kPlatformName} + "')");
            return false;
        }
        if (!matchesCurrentEnvironment(program_.environmentFingerprint)) {
            diagnostics_.error(
                "vm",
                std::string{bytecode_diagnostic::kEnvironmentMismatch},
                SourceLocation{"<bytecode>", 1, 1},
                "bytecode environment " + shortFingerprint(program_.environmentFingerprint) +
                    " does not match current compiler environment " + shortFingerprint(currentEnvironmentFingerprint()));
            return false;
        }
        const auto* entry = findFunction(program_, program_.entry);
        if (entry == nullptr) {
            diagnostics_.error("vm", "missing-entry", SourceLocation{"<bytecode>", 1, 1}, "entry function '" + program_.entry + "' not found");
            return false;
        }
        try {
            execute(*entry, {}, "");
        } catch (const std::exception& error) {
            diagnostics_.error("vm", "runtime-error", SourceLocation{"<bytecode>", 1, 1}, error.what());
            return false;
        }
        return !diagnostics_.hasErrors();
    }

    Value execute(const FunctionBytecode& function, const std::vector<Value>& args, const std::string& caller);

private:
    // 声明处理器类为友元，允许访问私有成员
    friend class StackHandler;
    friend class ArithmeticHandler;
    friend class ControlFlowHandler;
    friend class IoHandler;
    friend class SecurityHandler;

    bool chainHasRole(const std::string& chain, const std::string& role) const {
        if (equalsSymbol(chain, builtin::kRootAuthority) || chain == role) {
            return true;
        }
        const auto found = chains_.find(chain);
        if (found == chains_.end()) {
            return false;
        }
        return found->second.contains("*") || found->second.contains(role);
    }

    void requireRole(const FunctionBytecode& target, const std::string& chain) const {
        if (!target.grantor.empty() && !equalsSymbol(chain, builtin::kRootAuthority) && !chainHasRole(chain, target.grantor)) {
            throw std::runtime_error("Authority chain '" + chain + "' lacks required grantor '" + target.grantor + "'");
        }
        if (target.allowedRoles.empty() || equalsSymbol(chain, builtin::kRootAuthority)) {
            return;
        }
        for (const auto& role : target.allowedRoles) {
            if (chainHasRole(chain, role)) {
                return;
            }
        }
        throw std::runtime_error("Authority chain '" + chain + "' lacks a role allowed to call '" + target.name + "'");
    }

    void applyRoleInstruction(const Instruction& instruction) {
        const auto& op = instruction.args.at(0);
        const auto& role = instruction.args.at(1);
        const auto& principal = instruction.args.at(2);
        const auto& chain = instruction.args.at(3);
        if (equalsSymbol(op, role_operation::kGrant) || equalsSymbol(op, role_operation::kDelegate)) {
            if (!chainHasRole(chain, role) && !equalsSymbol(chain, builtin::kRootAuthority)) {
                throw std::runtime_error("Authority chain '" + chain + "' cannot grant role '" + role + "'");
            }
            chains_[principal].insert(role);
            return;
        }
        if (equalsSymbol(op, role_operation::kRevoke)) {
            chains_[principal].erase(role);
            return;
        }
        if (equalsSymbol(op, role_operation::kAssume)) {
            chains_[chain].insert(role);
            return;
        }
        if (equalsSymbol(op, role_operation::kTrace)) {
            (void)chains_[chain];
            return;
        }
    }

    const BytecodeProgram& program_;
    std::istream& input_;
    std::ostream& output_;
    Diagnostics& diagnostics_;
    VmOptions options_;
    std::size_t steps_ = 0;
    std::size_t nextFrameId_ = 0;
    Memory memory_;
    std::unordered_map<std::string, std::unordered_set<std::string>> chains_;
};

// 栈操作处理器：处理压栈、出栈、加载、存储等指令
class StackHandler : public InstructionHandler {
public:
    bool canHandle(const std::string& op) const override {
        return op == opcodeName(Opcode::kPushInteger) ||
               op == opcodeName(Opcode::kPushString) ||
               op == opcodeName(Opcode::kPushNull) ||
               op == opcodeName(Opcode::kPushReference) ||
               op == opcodeName(Opcode::kDereference) ||
               op == opcodeName(Opcode::kFieldReference) ||
               op == opcodeName(Opcode::kLoad) ||
               op == opcodeName(Opcode::kStore) ||
               op == opcodeName(Opcode::kStoreDereference) ||
               op == opcodeName(Opcode::kStorePointer) ||
               op == opcodeName(Opcode::kPop);
    }

    HandlerResult execute(ExecutionContext& ctx) override {
        const auto& op = ctx.instruction.op;
        if (op == opcodeName(Opcode::kPushInteger)) {
            ctx.stack.push_back(integerValue(parseInteger(ctx.instruction.args.at(0))));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kPushString)) {
            ctx.stack.push_back(stringValue(hexDecode(ctx.instruction.args.at(0))));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kPushNull)) {
            ctx.stack.push_back(nullValue());
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kPushReference)) {
            ctx.stack.push_back(ctx.memory.ref(ctx.instruction.args.at(0), ctx.framePrefix));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kDereference)) {
            const auto ref = pop(ctx.stack);
            ctx.stack.push_back(ctx.memory.loadRef(ref));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kFieldReference)) {
            auto ref = pop(ctx.stack);
            if (ref.kind != ValueKind::Ref) {
                throw std::runtime_error("cannot access field through non-reference value");
            }
            ref.string += ".";
            ref.string += ctx.instruction.args.at(0);
            ctx.stack.push_back(std::move(ref));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kLoad)) {
            ctx.stack.push_back(ctx.memory.load(ctx.instruction.args.at(0), ctx.framePrefix));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kStore)) {
            ctx.memory.store(ctx.instruction.args.at(0), ctx.framePrefix, pop(ctx.stack));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kStoreDereference)) {
            const auto value = pop(ctx.stack);
            const auto ref = pop(ctx.stack);
            ctx.memory.storeRef(ref, value);
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kStorePointer)) {
            ctx.memory.store(ctx.instruction.args.at(0), ctx.framePrefix, stringValue(ctx.instruction.args.at(1)));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kPop)) {
            pop(ctx.stack);
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        return HandlerResult::NotMatched;
    }
};

// 算术与逻辑处理器：处理加减乘除及其溢出策略、比较和逻辑运算
class ArithmeticHandler : public InstructionHandler {
public:
    bool canHandle(const std::string& op) const override {
        const auto [baseOp, policy] = splitOpPolicy(op);
        (void)policy;
        return baseOp == opcodeName(Opcode::kAdd) || baseOp == opcodeName(Opcode::kSub) ||
               baseOp == opcodeName(Opcode::kMul) || baseOp == opcodeName(Opcode::kDiv) ||
               op == opcodeName(Opcode::kEqual) || op == opcodeName(Opcode::kNotEqual) ||
               op == opcodeName(Opcode::kLess) || op == opcodeName(Opcode::kLessEqual) ||
               op == opcodeName(Opcode::kGreater) || op == opcodeName(Opcode::kGreaterEqual) ||
               op == opcodeName(Opcode::kAnd) || op == opcodeName(Opcode::kOr) ||
               op == opcodeName(Opcode::kXor) || op == opcodeName(Opcode::kNand) ||
               op == opcodeName(Opcode::kNor) || op == opcodeName(Opcode::kNot);
    }

    HandlerResult execute(ExecutionContext& ctx) override {
        const auto& op = ctx.instruction.op;
        // NOT 指令是一元操作，单独处理
        if (op == opcodeName(Opcode::kNot)) {
            pushBool(ctx.stack, !asBool(pop(ctx.stack)));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        // 二元算术和逻辑操作
        const auto [baseOp, policy] = splitOpPolicy(op);
        const auto rightValue = pop(ctx.stack);
        const auto leftValue = pop(ctx.stack);
        const auto right = asInteger(rightValue);
        const auto left = asInteger(leftValue);
        if (baseOp == opcodeName(Opcode::kAdd)) {
            if (leftValue.kind == ValueKind::String || rightValue.kind == ValueKind::String) {
                ctx.stack.push_back(stringValue(leftValue.string + rightValue.string));
            } else {
                ctx.stack.push_back(integerValue(applyOverflowPolicy(left + right, policy)));
            }
        } else if (baseOp == opcodeName(Opcode::kSub)) {
            ctx.stack.push_back(integerValue(applyOverflowPolicy(left - right, policy)));
        } else if (baseOp == opcodeName(Opcode::kMul)) {
            ctx.stack.push_back(integerValue(applyOverflowPolicy(left * right, policy)));
        } else if (baseOp == opcodeName(Opcode::kDiv)) {
            if (right == 0) {
                throw std::runtime_error("division by zero");
            }
            ctx.stack.push_back(integerValue(applyOverflowPolicy(left / right, policy)));
        } else if (op == opcodeName(Opcode::kEqual)) {
            pushBool(ctx.stack, equalsValue(leftValue, rightValue));
        } else if (op == opcodeName(Opcode::kNotEqual)) {
            pushBool(ctx.stack, !equalsValue(leftValue, rightValue));
        } else if (op == opcodeName(Opcode::kLess)) {
            pushBool(ctx.stack, left < right);
        } else if (op == opcodeName(Opcode::kLessEqual)) {
            pushBool(ctx.stack, left <= right);
        } else if (op == opcodeName(Opcode::kGreater)) {
            pushBool(ctx.stack, left > right);
        } else if (op == opcodeName(Opcode::kGreaterEqual)) {
            pushBool(ctx.stack, left >= right);
        } else if (op == opcodeName(Opcode::kAnd)) {
            pushBool(ctx.stack, left != 0 && right != 0);
        } else if (op == opcodeName(Opcode::kOr)) {
            pushBool(ctx.stack, left != 0 || right != 0);
        } else if (op == opcodeName(Opcode::kXor)) {
            pushBool(ctx.stack, (left != 0) != (right != 0));
        } else if (op == opcodeName(Opcode::kNand)) {
            pushBool(ctx.stack, !(left != 0 && right != 0));
        } else if (op == opcodeName(Opcode::kNor)) {
            pushBool(ctx.stack, !(left != 0 || right != 0));
        }
        ++ctx.pc;
        return HandlerResult::Continue;
    }
};

// 控制流处理器：处理跳转、调用、返回等指令
class ControlFlowHandler : public InstructionHandler {
public:
    bool canHandle(const std::string& op) const override {
        return op == opcodeName(Opcode::kJump) ||
               op == opcodeName(Opcode::kJumpIfZero) ||
               op == opcodeName(Opcode::kCall) ||
               op == opcodeName(Opcode::kCallIndirect) ||
               op == opcodeName(Opcode::kReturn) ||
               op == opcodeName(Opcode::kHalt);
    }

    HandlerResult execute(ExecutionContext& ctx) override {
        const auto& op = ctx.instruction.op;
        if (op == opcodeName(Opcode::kJump)) {
            ctx.pc = toSize(parseInteger(ctx.instruction.args.at(0)));
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kJumpIfZero)) {
            const auto target = toSize(parseInteger(ctx.instruction.args.at(0)));
            ctx.pc = !asBool(pop(ctx.stack)) ? target : ctx.pc + 1;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kCall)) {
            const auto* target = findFunction(ctx.interpreter.program_, ctx.instruction.args.at(0));
            if (target == nullptr) {
                throw std::runtime_error("unknown function '" + ctx.instruction.args.at(0) + "'");
            }
            const auto arity = toSize(parseInteger(ctx.instruction.args.at(1)));
            std::vector<Value> args;
            args.reserve(arity);
            for (std::size_t i = 0; i < arity; ++i) {
                args.push_back(pop(ctx.stack));
            }
            std::reverse(args.begin(), args.end());
            const auto ret = ctx.interpreter.execute(*target, args, ctx.function.name);
            if (target->returnable) {
                ctx.stack.push_back(ret);
            }
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kCallIndirect)) {
            const auto pointer = ctx.memory.load(ctx.instruction.args.at(0), ctx.framePrefix);
            if (pointer.kind != ValueKind::String) {
                throw std::runtime_error("function pointer '" + ctx.instruction.args.at(0) + "' is not assigned");
            }
            const auto* target = findFunction(ctx.interpreter.program_, pointer.string);
            if (target == nullptr) {
                throw std::runtime_error("unknown function '" + pointer.string + "'");
            }
            const auto arity = toSize(parseInteger(ctx.instruction.args.at(1)));
            const bool discard = equalsSymbol(ctx.instruction.args.at(2), kInstructionTrue);
            std::vector<Value> args;
            args.reserve(arity);
            for (std::size_t i = 0; i < arity; ++i) {
                args.push_back(pop(ctx.stack));
            }
            std::reverse(args.begin(), args.end());
            const auto ret = ctx.interpreter.execute(*target, args, ctx.function.name);
            if (target->returnable && !discard) {
                ctx.stack.push_back(ret);
            }
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kReturn)) {
            if (ctx.function.returnable && !ctx.stack.empty()) {
                ctx.returnValue = pop(ctx.stack);
            } else {
                ctx.returnValue = integerValue(0);
            }
            return HandlerResult::Return;
        }
        if (op == opcodeName(Opcode::kHalt)) {
            ctx.returnValue = integerValue(0);
            return HandlerResult::Return;
        }
        return HandlerResult::NotMatched;
    }
};

// 输入输出处理器：处理输出、输入等指令
class IoHandler : public InstructionHandler {
public:
    bool canHandle(const std::string& op) const override {
        return op == opcodeName(Opcode::kOutputInt) ||
               op == opcodeName(Opcode::kOutputChar) ||
               op == opcodeName(Opcode::kPrintLine) ||
               op == opcodeName(Opcode::kInputInt) ||
               op == opcodeName(Opcode::kInputChar) ||
               op == opcodeName(Opcode::kOperatorInput);
    }

    HandlerResult execute(ExecutionContext& ctx) override {
        const auto& op = ctx.instruction.op;
        if (op == opcodeName(Opcode::kOutputInt)) {
            ctx.interpreter.output_ << intToString(asInteger(pop(ctx.stack))) << '\n';
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kOutputChar)) {
            ctx.interpreter.output_ << static_cast<char>(static_cast<unsigned char>(asInteger(pop(ctx.stack))));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kPrintLine)) {
            ctx.interpreter.output_ << valueToString(pop(ctx.stack)) << '\n';
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kInputInt)) {
            long long value = 0;
            ctx.interpreter.input_ >> value;
            ctx.stack.push_back(integerValue(value));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kInputChar)) {
            char ch = '\0';
            ctx.interpreter.input_.get(ch);
            ctx.stack.push_back(integerValue(static_cast<unsigned char>(ch)));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kOperatorInput)) {
            std::string answer;
            ctx.interpreter.input_ >> answer;
            ctx.memory.store(ctx.instruction.args.at(0), ctx.framePrefix, integerValue(equalsSymbol(answer, source_literal::kYes) ? 1 : 0));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        return HandlerResult::NotMatched;
    }
};

// 安全处理器：处理验证、权限检查、断言等指令
class SecurityHandler : public InstructionHandler {
public:
    bool canHandle(const std::string& op) const override {
        return op == opcodeName(Opcode::kVerify) ||
               op == opcodeName(Opcode::kCheckApproval) ||
               op == opcodeName(Opcode::kCheckLimits) ||
               op == opcodeName(Opcode::kCheckRole) ||
               op == opcodeName(Opcode::kCheckRoleIndirect) ||
               op == opcodeName(Opcode::kNullCheck) ||
               op == opcodeName(Opcode::kAssert) ||
               op == opcodeName(Opcode::kRole) ||
               op == opcodeName(Opcode::kVerifyEcc) ||
               op == opcodeName(Opcode::kCopyMemory) ||
               op == opcodeName(Opcode::kCompareMemory);
    }

    HandlerResult execute(ExecutionContext& ctx) override {
        const auto& op = ctx.instruction.op;
        if (op == opcodeName(Opcode::kVerify)) {
            const auto actual = functionFingerprint(ctx.function);
            if (actual != ctx.function.hash) {
                throw std::runtime_error("Function identity verification failed for " + ctx.function.name);
            }
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kCheckApproval)) {
            const auto& approval = ctx.instruction.args.at(0);
            if (equalsSymbol(approval, builtin::kAlwaysDeny)) {
                throw std::runtime_error("Approval denied");
            }
            if (!equalsSymbol(approval, builtin::kAlwaysApprove)) {
                const auto* approvalFunction = findFunction(ctx.interpreter.program_, approval);
                if (approvalFunction == nullptr || !equalsSymbol(approvalFunction->category, function_category::kApproval)) {
                    throw std::runtime_error("Unknown approval function '" + approval + "'");
                }
                const auto decision = ctx.interpreter.execute(*approvalFunction, {stringValue("approve invocation of " + ctx.instruction.args.at(1))}, ctx.function.name);
                if (!asBool(decision)) {
                    throw std::runtime_error("Approval denied");
                }
            }
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kCheckLimits)) {
            const auto memoryLimit = parseInteger(ctx.instruction.args.at(0));
            const auto timeout = parseInteger(ctx.instruction.args.at(1));
            const auto predictedStack = parseInteger(ctx.instruction.args.at(2));
            if (memoryLimit <= 0 || timeout <= 0) {
                throw std::runtime_error("invalid call resource limits");
            }
            if (predictedStack > static_cast<Int>(ctx.interpreter.options_.maxSteps)) {
                throw std::runtime_error("predicted stack depth exceeds VM limit");
            }
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kCheckRole)) {
            const auto* target = findFunction(ctx.interpreter.program_, ctx.instruction.args.at(0));
            if (target == nullptr) {
                throw std::runtime_error("unknown function '" + ctx.instruction.args.at(0) + "'");
            }
            const auto& chain = ctx.instruction.args.at(1);
            ctx.interpreter.requireRole(*target, chain);
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kCheckRoleIndirect)) {
            const auto pointer = ctx.memory.load(ctx.instruction.args.at(0), ctx.framePrefix);
            if (pointer.kind != ValueKind::String) {
                throw std::runtime_error("function pointer '" + ctx.instruction.args.at(0) + "' is not assigned");
            }
            const auto* target = findFunction(ctx.interpreter.program_, pointer.string);
            if (target == nullptr) {
                throw std::runtime_error("unknown function '" + pointer.string + "'");
            }
            const auto& chain = ctx.instruction.args.at(1);
            ctx.interpreter.requireRole(*target, chain);
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kNullCheck)) {
            const auto pointer = ctx.memory.load(ctx.instruction.args.at(0), ctx.framePrefix);
            if (pointer.kind != ValueKind::String || pointer.string.empty()) {
                throw std::runtime_error("null function pointer '" + ctx.instruction.args.at(0) + "'");
            }
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kAssert)) {
            if (!asBool(pop(ctx.stack))) {
                const auto label = ctx.instruction.args.empty() ? std::string{"anonymous obligation"} : ctx.instruction.args.front();
                throw std::runtime_error("proof obligation failed: " + label);
            }
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kRole)) {
            ctx.interpreter.applyRoleInstruction(ctx.instruction);
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kVerifyEcc)) {
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kCopyMemory)) {
            (void)pop(ctx.stack);
            const auto src = pop(ctx.stack);
            const auto dst = pop(ctx.stack);
            if (src.kind != ValueKind::Ref || dst.kind != ValueKind::Ref) {
                throw std::runtime_error("copymemory expects reference arguments");
            }
            ctx.memory.storeRef(dst, ctx.memory.loadRef(src));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        if (op == opcodeName(Opcode::kCompareMemory)) {
            (void)pop(ctx.stack);
            const auto right = pop(ctx.stack);
            const auto left = pop(ctx.stack);
            if (left.kind != ValueKind::Ref || right.kind != ValueKind::Ref) {
                throw std::runtime_error("comparememory expects reference arguments");
            }
            ctx.stack.push_back(integerValue(equalsValue(ctx.memory.loadRef(left), ctx.memory.loadRef(right)) ? 0 : 1));
            ++ctx.pc;
            return HandlerResult::Continue;
        }
        return HandlerResult::NotMatched;
    }
};

// 解释器执行方法：使用处理器分派执行指令
Value Interpreter::execute(const FunctionBytecode& function, const std::vector<Value>& args, const std::string& caller) {
    if (args.size() != function.params.size()) {
        throw std::runtime_error("function '" + function.name + "' expected " + std::to_string(function.params.size()) +
            " arguments but got " + std::to_string(args.size()));
    }
    if (!function.callableFrom.empty() && !caller.empty() &&
        std::find(function.callableFrom.begin(), function.callableFrom.end(), caller) == function.callableFrom.end()) {
        throw std::runtime_error("function '" + function.name + "' is not callable from '" + caller + "'");
    }
    std::vector<Value> stack;
    const auto framePrefix = function.name + "#" + std::to_string(nextFrameId_++) + ":";
    for (const auto& local : function.locals) {
        memory_.ensureAddress(local, framePrefix);
    }
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        memory_.store(function.params[i], framePrefix, args[i]);
    }

    // 创建处理器实例
    StackHandler stackHandler;
    ArithmeticHandler arithHandler;
    ControlFlowHandler controlHandler;
    IoHandler ioHandler;
    SecurityHandler securityHandler;

    std::size_t pc = 0;
    while (pc < function.code.size()) {
        if (++steps_ > options_.maxSteps) {
            throw std::runtime_error("VM step limit exceeded");
        }
        if (stack.size() > static_cast<std::size_t>(function.maxStackSize)) {
            throw std::runtime_error("max stack size exceeded in function " + function.name);
        }
        const auto& instruction = function.code[pc];
        const auto& op = instruction.op;

        // 构建执行上下文，传递所有运行时状态给处理器
        ExecutionContext ctx{stack, memory_, framePrefix, instruction, function, caller, pc, *this, Value{}};

        // 按类别分派到对应处理器
        if (stackHandler.canHandle(op)) {
            const auto result = stackHandler.execute(ctx);
            if (result == HandlerResult::Return) {
                return ctx.returnValue;
            }
        } else if (arithHandler.canHandle(op)) {
            const auto result = arithHandler.execute(ctx);
            if (result == HandlerResult::Return) {
                return ctx.returnValue;
            }
        } else if (controlHandler.canHandle(op)) {
            const auto result = controlHandler.execute(ctx);
            if (result == HandlerResult::Return) {
                return ctx.returnValue;
            }
        } else if (ioHandler.canHandle(op)) {
            const auto result = ioHandler.execute(ctx);
            if (result == HandlerResult::Return) {
                return ctx.returnValue;
            }
        } else if (securityHandler.canHandle(op)) {
            const auto result = securityHandler.execute(ctx);
            if (result == HandlerResult::Return) {
                return ctx.returnValue;
            }
        } else {
            throw std::runtime_error("unknown opcode " + op);
        }
    }
    return integerValue(0);
}

bool runBytecode(const BytecodeProgram& program, std::istream& input, std::ostream& output, Diagnostics& diagnostics, VmOptions options) {
    Interpreter interpreter(program, input, output, diagnostics, options);
    return interpreter.run();
}

} // namespace torture::vm
