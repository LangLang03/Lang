#include "vm/vm.h"
#include "vm/memory.h"
#include "vm/value.h"

#include "common/sha512.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
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

// 把带 policy 的算术 opcode 直接映射到 policy 名（编译期 switch，无字符串查表）
std::string_view overflowPolicyFor(Opcode op) {
    switch (op) {
    case Opcode::kAddTrap: case Opcode::kSubTrap: case Opcode::kMulTrap: case Opcode::kDivTrap:
        return overflow_policy::kTrap;
    case Opcode::kAddWrap: case Opcode::kSubWrap: case Opcode::kMulWrap: case Opcode::kDivWrap:
        return overflow_policy::kWrap;
    case Opcode::kAddSaturate: case Opcode::kSubSaturate: case Opcode::kMulSaturate: case Opcode::kDivSaturate:
        return overflow_policy::kSaturate;
    case Opcode::kAddExtend: case Opcode::kSubExtend: case Opcode::kMulExtend: case Opcode::kDivExtend:
        return overflow_policy::kExtend;
    default:
        return {};
    }
}

} // namespace

// VM 解释器类，负责执行字节码程序
class Interpreter {
public:
    Interpreter(const BytecodeProgram& program, std::istream& input, std::ostream& output, Diagnostics& diagnostics, VmOptions options)
        : program_(program), input_(input), output_(output), diagnostics_(diagnostics), options_(options) {
        chains_[std::string{builtin::kRootAuthority}].insert("*");
        // 预建函数索引：O(1) 查找，避免每次 CALL/CHECKROLE 线性扫所有函数
        function_index_.reserve(program_.functions.size());
        for (const auto& function : program_.functions) {
            function_index_.emplace(function.name, &function);
        }
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
        const auto* entry = lookupFunction(program_.entry);
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
        return true;
    }

    Value execute(const FunctionBytecode& function, const std::vector<Value>& args, const std::string& caller);

private:
    const FunctionBytecode* lookupFunction(const std::string& name) const {
        const auto it = function_index_.find(name);
        return it == function_index_.end() ? nullptr : it->second;
    }

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
    std::unordered_map<std::string, const FunctionBytecode*> function_index_;
};

// 解释器执行方法：单 switch 分派，避免每条指令的字符串比较
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

    const auto& code = function.code;
    std::size_t pc = 0;
    while (pc < code.size()) {
        if (++steps_ > options_.maxSteps) {
            throw std::runtime_error("VM step limit exceeded");
        }
        if (stack.size() > static_cast<std::size_t>(function.maxStackSize)) {
            throw std::runtime_error("max stack size exceeded in function " + function.name);
        }
        const auto& instruction = code[pc];
        const auto& argsRef = instruction.args;

        switch (instruction.op) {
        // ============== 栈操作 ==============
        case Opcode::kPushInteger: {
            stack.push_back(integerValue(parseInteger(argsRef.at(0))));
            ++pc;
            break;
        }
        case Opcode::kPushString: {
            stack.push_back(stringValue(hexDecode(argsRef.at(0))));
            ++pc;
            break;
        }
        case Opcode::kPushNull: {
            stack.push_back(nullValue());
            ++pc;
            break;
        }
        case Opcode::kPushReference: {
            stack.push_back(memory_.ref(argsRef.at(0), framePrefix));
            ++pc;
            break;
        }
        case Opcode::kDereference: {
            const auto ref = pop(stack);
            stack.push_back(memory_.loadRef(ref));
            ++pc;
            break;
        }
        case Opcode::kFieldReference: {
            auto ref = pop(stack);
            if (ref.kind != ValueKind::Ref) {
                throw std::runtime_error("cannot access field through non-reference value");
            }
            ref.string += ".";
            ref.string += argsRef.at(0);
            stack.push_back(std::move(ref));
            ++pc;
            break;
        }
        case Opcode::kLoad: {
            stack.push_back(memory_.load(argsRef.at(0), framePrefix));
            ++pc;
            break;
        }
        case Opcode::kStore: {
            memory_.store(argsRef.at(0), framePrefix, pop(stack));
            ++pc;
            break;
        }
        case Opcode::kStoreDereference: {
            const auto value = pop(stack);
            const auto ref = pop(stack);
            memory_.storeRef(ref, value);
            ++pc;
            break;
        }
        case Opcode::kStorePointer: {
            memory_.store(argsRef.at(0), framePrefix, stringValue(argsRef.at(1)));
            ++pc;
            break;
        }
        case Opcode::kPop: {
            pop(stack);
            ++pc;
            break;
        }

        // ============== 算术与逻辑 ==============
        case Opcode::kAdd: {
            const auto rightValue = pop(stack);
            const auto leftValue = pop(stack);
            if (leftValue.kind == ValueKind::String || rightValue.kind == ValueKind::String) {
                stack.push_back(stringValue(leftValue.string + rightValue.string));
            } else {
                stack.push_back(integerValue(applyOverflowPolicy(asInteger(leftValue) + asInteger(rightValue), std::string(overflowPolicyFor(instruction.op)))));
            }
            ++pc;
            break;
        }
        case Opcode::kAddTrap:
        case Opcode::kAddWrap:
        case Opcode::kAddSaturate:
        case Opcode::kAddExtend: {
            const auto policy = overflowPolicyFor(instruction.op);
            const auto rightValue = pop(stack);
            const auto leftValue = pop(stack);
            if (leftValue.kind == ValueKind::String || rightValue.kind == ValueKind::String) {
                stack.push_back(stringValue(leftValue.string + rightValue.string));
            } else {
                stack.push_back(integerValue(applyOverflowPolicy(asInteger(leftValue) + asInteger(rightValue), std::string(policy))));
            }
            ++pc;
            break;
        }
        case Opcode::kSub: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            stack.push_back(integerValue(applyOverflowPolicy(left - right, std::string(overflowPolicyFor(instruction.op)))));
            ++pc;
            break;
        }
        case Opcode::kSubTrap:
        case Opcode::kSubWrap:
        case Opcode::kSubSaturate:
        case Opcode::kSubExtend: {
            const auto policy = overflowPolicyFor(instruction.op);
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            stack.push_back(integerValue(applyOverflowPolicy(left - right, std::string(policy))));
            ++pc;
            break;
        }
        case Opcode::kMul: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            stack.push_back(integerValue(applyOverflowPolicy(left * right, std::string(overflowPolicyFor(instruction.op)))));
            ++pc;
            break;
        }
        case Opcode::kMulTrap:
        case Opcode::kMulWrap:
        case Opcode::kMulSaturate:
        case Opcode::kMulExtend: {
            const auto policy = overflowPolicyFor(instruction.op);
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            stack.push_back(integerValue(applyOverflowPolicy(left * right, std::string(policy))));
            ++pc;
            break;
        }
        case Opcode::kDiv: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            if (right == 0) {
                throw std::runtime_error("division by zero");
            }
            stack.push_back(integerValue(applyOverflowPolicy(left / right, std::string(overflowPolicyFor(instruction.op)))));
            ++pc;
            break;
        }
        case Opcode::kDivTrap:
        case Opcode::kDivWrap:
        case Opcode::kDivSaturate:
        case Opcode::kDivExtend: {
            const auto policy = overflowPolicyFor(instruction.op);
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            if (right == 0) {
                throw std::runtime_error("division by zero");
            }
            stack.push_back(integerValue(applyOverflowPolicy(left / right, std::string(policy))));
            ++pc;
            break;
        }
        case Opcode::kEqual: {
            const auto rightValue = pop(stack);
            const auto leftValue = pop(stack);
            pushBool(stack, equalsValue(leftValue, rightValue));
            ++pc;
            break;
        }
        case Opcode::kNotEqual: {
            const auto rightValue = pop(stack);
            const auto leftValue = pop(stack);
            pushBool(stack, !equalsValue(leftValue, rightValue));
            ++pc;
            break;
        }
        case Opcode::kLess: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            pushBool(stack, left < right);
            ++pc;
            break;
        }
        case Opcode::kLessEqual: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            pushBool(stack, left <= right);
            ++pc;
            break;
        }
        case Opcode::kGreater: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            pushBool(stack, left > right);
            ++pc;
            break;
        }
        case Opcode::kGreaterEqual: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            pushBool(stack, left >= right);
            ++pc;
            break;
        }
        case Opcode::kAnd: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            pushBool(stack, left != 0 && right != 0);
            ++pc;
            break;
        }
        case Opcode::kOr: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            pushBool(stack, left != 0 || right != 0);
            ++pc;
            break;
        }
        case Opcode::kXor: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            pushBool(stack, (left != 0) != (right != 0));
            ++pc;
            break;
        }
        case Opcode::kNand: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            pushBool(stack, !(left != 0 && right != 0));
            ++pc;
            break;
        }
        case Opcode::kNor: {
            const auto right = asInteger(pop(stack));
            const auto left = asInteger(pop(stack));
            pushBool(stack, !(left != 0 || right != 0));
            ++pc;
            break;
        }
        case Opcode::kNot: {
            pushBool(stack, !asBool(pop(stack)));
            ++pc;
            break;
        }

        // ============== 控制流 ==============
        case Opcode::kJump: {
            pc = toSize(parseInteger(argsRef.at(0)));
            break;
        }
        case Opcode::kJumpIfZero: {
            const auto target = toSize(parseInteger(argsRef.at(0)));
            pc = !asBool(pop(stack)) ? target : pc + 1;
            break;
        }
        case Opcode::kCall: {
            const auto* target = lookupFunction(argsRef.at(0));
            if (target == nullptr) {
                throw std::runtime_error("unknown function '" + argsRef.at(0) + "'");
            }
            const auto arity = toSize(parseInteger(argsRef.at(1)));
            std::vector<Value> callArgs;
            callArgs.reserve(arity);
            for (std::size_t i = 0; i < arity; ++i) {
                callArgs.push_back(pop(stack));
            }
            std::reverse(callArgs.begin(), callArgs.end());
            const auto ret = execute(*target, callArgs, function.name);
            if (target->returnable) {
                stack.push_back(ret);
            }
            ++pc;
            break;
        }
        case Opcode::kCallIndirect: {
            const auto pointer = memory_.load(argsRef.at(0), framePrefix);
            if (pointer.kind != ValueKind::String) {
                throw std::runtime_error("function pointer '" + argsRef.at(0) + "' is not assigned");
            }
            const auto* target = lookupFunction(pointer.string);
            if (target == nullptr) {
                throw std::runtime_error("unknown function '" + pointer.string + "'");
            }
            const auto arity = toSize(parseInteger(argsRef.at(1)));
            const bool discard = equalsSymbol(argsRef.at(2), kInstructionTrue);
            std::vector<Value> callArgs;
            callArgs.reserve(arity);
            for (std::size_t i = 0; i < arity; ++i) {
                callArgs.push_back(pop(stack));
            }
            std::reverse(callArgs.begin(), callArgs.end());
            const auto ret = execute(*target, callArgs, function.name);
            if (target->returnable && !discard) {
                stack.push_back(ret);
            }
            ++pc;
            break;
        }
        case Opcode::kReturn: {
            Value returnValue;
            if (function.returnable && !stack.empty()) {
                returnValue = pop(stack);
            } else {
                returnValue = integerValue(0);
            }
            return returnValue;
        }
        case Opcode::kHalt: {
            return integerValue(0);
        }
        case Opcode::kApply: {
            // FFI `apply external <bindname>` 运行期校验：arch/sys、lib 路径、SHA-512 链。
            // 指令参数顺序（与 bytecode_compiler 严格对应）：
            //   0:bindname 1:arch 2:sys 3:libPath 4:symbol
            //   5:receivingReturnInto 6:justification 7:approvalJustification 8:approvalTimeout
            //   9:securityLevel 10:memoryLimit 11:timeout 12:predictStackDepth
            //   13:authorityChain 14:approvalFunction 15:discardingReturn 16:fromNamespace
            //   17:argumentCount
            const auto& argsR = instruction.args;
            const auto bindName = argsR.at(0);
            const auto runtimeArch = argsR.at(1);
            const auto runtimeSys = argsR.at(2);
            const auto libPath = argsR.at(3);
            const auto symbol = argsR.at(4);
            (void)argsR.at(5);  // receivingReturnInto
            (void)argsR.at(6);  // justification
            (void)argsR.at(7);  // approvalJustification
            (void)argsR.at(8);  // approvalTimeout
            (void)argsR.at(9);  // securityLevel
            (void)argsR.at(10); // memoryLimit
            (void)argsR.at(11); // timeout
            (void)argsR.at(12); // predictStackDepth
            (void)argsR.at(13); // authorityChain
            (void)argsR.at(14); // approvalFunction
            (void)argsR.at(15); // discardingReturn
            (void)argsR.at(16); // fromNamespace
            const auto argCount = toSize(parseInteger(argsR.at(17)));
            // 弹出参数（保持 LIFO 顺序）
            for (std::size_t i = 0; i < argCount; ++i) {
                (void)pop(stack);
            }
            // 1. 找到 binding
            const BytecodeProgram::ExternalBinding* binding = nullptr;
            for (const auto& b : program_.externalBindings) {
                if (b.name == bindName) {
                    binding = &b;
                    break;
                }
            }
            if (binding == nullptr) {
                throw std::runtime_error("ffi-runtime-binding-missing: no external binding '" + bindName + "'");
            }
            // 2. arch/sys 与当前平台匹配
            const std::string currentArch = std::string{kPlatformName}.substr(0, std::string{kPlatformName}.find('_'));
            std::string currentSys = "unknown";
            const auto pname = std::string{kPlatformName};
            if (pname.starts_with("linux_")) currentSys = "linux";
            else if (pname.starts_with("windows_")) currentSys = "windows";
            else if (pname.starts_with("android_")) currentSys = "android";
            else if (pname.starts_with("macos_")) currentSys = "macos";
            else if (pname.starts_with("freebsd_")) currentSys = "freebsd";
            else if (pname.starts_with("openbsd_")) currentSys = "openbsd";
            else if (pname.starts_with("uefi_")) currentSys = "uefi";
            if (binding->arch != currentArch) {
                throw std::runtime_error("ffi-runtime-platform-mismatch: binding '" + bindName + "' declared arch '" + binding->arch + "' but running on arch '" + currentArch + "'");
            }
            if (binding->sys != currentSys) {
                throw std::runtime_error("ffi-runtime-platform-mismatch: binding '" + bindName + "' declared sys '" + binding->sys + "' but running on sys '" + currentSys + "'");
            }
            if (runtimeArch != binding->arch || runtimeSys != binding->sys) {
                throw std::runtime_error("ffi-runtime-platform-mismatch: apply call restated arch/sys do not match external declaration");
            }
            if (runtimeArch != currentArch) {
                throw std::runtime_error("ffi-runtime-platform-mismatch: apply call arch '" + runtimeArch + "' does not match current platform arch '" + currentArch + "'");
            }
            if (runtimeSys != currentSys) {
                throw std::runtime_error("ffi-runtime-platform-mismatch: apply call sys '" + runtimeSys + "' does not match current platform sys '" + currentSys + "'");
            }
            // 3. 重新计算 SHA-512 链
            if (binding->sha512Chain.size() != 3) {
                throw std::runtime_error("ffi-runtime-sha512-mismatch: binding '" + bindName + "' does not have a 3-segment sha512 chain");
            }
            const auto h1 = torture::common::sha512Hex(binding->sha512Chain[0]);
            if (h1 != binding->sha512Chain[1]) {
                throw std::runtime_error("ffi-runtime-sha512-mismatch: binding '" + bindName + "' sha512 chain broken at segment 2");
            }
            const auto h2 = torture::common::sha512Hex(binding->sha512Chain[1]);
            if (h2 != binding->sha512Chain[2]) {
                throw std::runtime_error("ffi-runtime-sha512-mismatch: binding '" + bindName + "' sha512 chain broken at segment 3");
            }
            // 4. 检查 lib 路径是绝对路径且文件存在
            if (libPath.empty() || libPath.front() != '/') {
                throw std::runtime_error("ffi-runtime-library-missing: apply call lib path '" + libPath + "' is not an absolute path");
            }
            if (!std::filesystem::exists(std::filesystem::path{libPath})) {
                throw std::runtime_error("ffi-runtime-library-missing: library file '" + libPath + "' does not exist on disk");
            }
            (void)symbol;
            ++pc;
            break;
        }

        // ============== IO ==============
        case Opcode::kOutputInt: {
            output_ << intToString(asInteger(pop(stack))) << '\n';
            ++pc;
            break;
        }
        case Opcode::kOutputChar: {
            output_ << static_cast<char>(static_cast<unsigned char>(asInteger(pop(stack))));
            ++pc;
            break;
        }
        case Opcode::kPrintLine: {
            output_ << valueToString(pop(stack)) << '\n';
            ++pc;
            break;
        }
        case Opcode::kInputInt: {
            long long value = 0;
            input_ >> value;
            stack.push_back(integerValue(value));
            ++pc;
            break;
        }
        case Opcode::kInputChar: {
            char ch = '\0';
            input_.get(ch);
            stack.push_back(integerValue(static_cast<unsigned char>(ch)));
            ++pc;
            break;
        }
        case Opcode::kOperatorInput: {
            std::string answer;
            input_ >> answer;
            memory_.store(argsRef.at(0), framePrefix, integerValue(equalsSymbol(answer, source_literal::kYes) ? 1 : 0));
            ++pc;
            break;
        }

        // ============== 安全/角色 ==============
        case Opcode::kVerify: {
            const auto actual = functionFingerprint(function);
            if (actual != function.hash) {
                throw std::runtime_error("Function identity verification failed for " + function.name);
            }
            ++pc;
            break;
        }
        case Opcode::kCheckApproval: {
            const auto& approval = argsRef.at(0);
            if (equalsSymbol(approval, builtin::kAlwaysDeny)) {
                throw std::runtime_error("Approval denied");
            }
            if (!equalsSymbol(approval, builtin::kAlwaysApprove)) {
                const auto* approvalFunction = lookupFunction(approval);
                if (approvalFunction == nullptr || !equalsSymbol(approvalFunction->category, function_category::kApproval)) {
                    throw std::runtime_error("Unknown approval function '" + approval + "'");
                }
                const auto decision = execute(*approvalFunction, {stringValue("approve invocation of " + argsRef.at(1))}, function.name);
                if (!asBool(decision)) {
                    throw std::runtime_error("Approval denied");
                }
            }
            ++pc;
            break;
        }
        case Opcode::kCheckLimits: {
            const auto memoryLimit = parseInteger(argsRef.at(0));
            const auto timeout = parseInteger(argsRef.at(1));
            const auto predictedStack = parseInteger(argsRef.at(2));
            if (memoryLimit <= 0 || timeout <= 0) {
                throw std::runtime_error("invalid call resource limits");
            }
            if (predictedStack > static_cast<Int>(options_.maxSteps)) {
                throw std::runtime_error("predicted stack depth exceeds VM limit");
            }
            ++pc;
            break;
        }
        case Opcode::kCheckRole: {
            const auto* target = lookupFunction(argsRef.at(0));
            if (target == nullptr) {
                throw std::runtime_error("unknown function '" + argsRef.at(0) + "'");
            }
            requireRole(*target, argsRef.at(1));
            ++pc;
            break;
        }
        case Opcode::kCheckRoleIndirect: {
            const auto pointer = memory_.load(argsRef.at(0), framePrefix);
            if (pointer.kind != ValueKind::String) {
                throw std::runtime_error("function pointer '" + argsRef.at(0) + "' is not assigned");
            }
            const auto* target = lookupFunction(pointer.string);
            if (target == nullptr) {
                throw std::runtime_error("unknown function '" + pointer.string + "'");
            }
            requireRole(*target, argsRef.at(1));
            ++pc;
            break;
        }
        case Opcode::kNullCheck: {
            const auto& name = argsRef.at(0);
            const auto pointer = memory_.load(name, framePrefix);
            if (pointer.kind != ValueKind::String || pointer.string.empty()) {
                throw std::runtime_error("null function pointer '" + name + "'");
            }
            ++pc;
            break;
        }
        case Opcode::kAssert: {
            if (!asBool(pop(stack))) {
                const auto label = argsRef.empty() ? std::string{"anonymous obligation"} : argsRef.front();
                throw std::runtime_error("proof obligation failed: " + label);
            }
            ++pc;
            break;
        }
        case Opcode::kRole: {
            applyRoleInstruction(instruction);
            ++pc;
            break;
        }
        case Opcode::kVerifyEcc: {
            ++pc;
            break;
        }
        case Opcode::kCopyMemory: {
            (void)pop(stack);
            const auto src = pop(stack);
            const auto dst = pop(stack);
            if (src.kind != ValueKind::Ref || dst.kind != ValueKind::Ref) {
                throw std::runtime_error("copymemory expects reference arguments");
            }
            memory_.storeRef(dst, memory_.loadRef(src));
            ++pc;
            break;
        }
        case Opcode::kCompareMemory: {
            (void)pop(stack);
            const auto right = pop(stack);
            const auto left = pop(stack);
            if (left.kind != ValueKind::Ref || right.kind != ValueKind::Ref) {
                throw std::runtime_error("comparememory expects reference arguments");
            }
            stack.push_back(integerValue(equalsValue(memory_.loadRef(left), memory_.loadRef(right)) ? 0 : 1));
            ++pc;
            break;
        }
        } // switch
    }
    return integerValue(0);
}

bool runBytecode(const BytecodeProgram& program, std::istream& input, std::ostream& output, Diagnostics& diagnostics, VmOptions options) {
    Interpreter interpreter(program, input, output, diagnostics, options);
    return interpreter.run();
}

} // namespace torture::vm
