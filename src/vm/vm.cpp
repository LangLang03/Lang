#include "vm/vm.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <istream>
#include <limits>
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

constexpr char kDigitZero = '0';
constexpr int kDecimalBase = 10;
constexpr int kHexAlphaOffset = 10;
constexpr int kHexHighNibbleShift = 4;
constexpr std::size_t kHexEncodedByteWidth = 2;
constexpr std::string_view kIntegerZeroText = "0";
constexpr std::string_view kInstructionTrue = "1";

enum class ValueKind {
    Null,
    Integer,
    String,
    Ref,
};

struct Value {
    ValueKind kind = ValueKind::Integer;
    Int integer = 0;
    std::string string;
};

Int parseInteger(const std::string& text);

Value integerValue(Int value) {
    return Value{ValueKind::Integer, value, {}};
}

Value stringValue(std::string value) {
    return Value{ValueKind::String, 0, std::move(value)};
}

Value nullValue() {
    return Value{ValueKind::Null, 0, {}};
}

Value refValue(std::string name) {
    return Value{ValueKind::Ref, 0, std::move(name)};
}

bool equalsSymbol(const std::string& value, std::string_view symbol) {
    return std::string_view{value} == symbol;
}

Int asInteger(const Value& value) {
    if (value.kind == ValueKind::Integer) {
        return value.integer;
    }
    if (value.kind == ValueKind::Null) {
        return 0;
    }
    if (value.kind == ValueKind::Ref) {
        throw std::runtime_error("cannot use reference as integer without dereference");
    }
    return parseInteger(value.string);
}

bool asBool(const Value& value) {
    if (value.kind == ValueKind::String) {
        return !value.string.empty();
    }
    return asInteger(value) != 0;
}

bool equalsValue(const Value& left, const Value& right) {
    if (left.kind == ValueKind::String || right.kind == ValueKind::String || left.kind == ValueKind::Ref || right.kind == ValueKind::Ref) {
        return left.string == right.string;
    }
    return asInteger(left) == asInteger(right);
}

Int parseInteger(const std::string& text) {
    Int value = 0;
    bool negative = false;
    std::size_t index = 0;
    if (!text.empty() && text.front() == '-') {
        negative = true;
        index = 1;
    }
    for (; index < text.size(); ++index) {
        value = value * kDecimalBase + static_cast<Int>(text[index] - kDigitZero);
    }
    if (negative) {
        value = -value;
    }
    return value;
}

std::string intToString(Int value) {
    if (value == 0) {
        return std::string{kIntegerZeroText};
    }
    bool negative = value < 0;
    if (negative) {
        value = -value;
    }
    std::string out;
    while (value > 0) {
        out.push_back(static_cast<char>(kDigitZero + value % kDecimalBase));
        value /= kDecimalBase;
    }
    if (negative) {
        out.push_back('-');
    }
    std::reverse(out.begin(), out.end());
    return out;
}

std::string valueToString(const Value& value) {
    if (value.kind == ValueKind::String) {
        return value.string;
    }
    if (value.kind == ValueKind::Null) {
        return std::string{source_literal::kNull};
    }
    if (value.kind == ValueKind::Ref) {
        return value.string;
    }
    return intToString(value.integer);
}

std::size_t toSize(Int value) {
    if (value < 0) {
        throw std::runtime_error("negative bytecode jump target");
    }
    return static_cast<std::size_t>(value);
}

Int int64Min() {
    return static_cast<Int>(std::numeric_limits<std::int64_t>::min());
}

Int int64Max() {
    return static_cast<Int>(std::numeric_limits<std::int64_t>::max());
}

Int clamp64(Int value) {
    if (value < int64Min()) {
        return int64Min();
    }
    if (value > int64Max()) {
        return int64Max();
    }
    return value;
}

Int wrap64(Int value) {
    const auto signed_bits = std::numeric_limits<std::int64_t>::digits;
    const auto unsigned_bits = std::numeric_limits<std::uint64_t>::digits;
    const Int unsigned_range = static_cast<Int>(1) << unsigned_bits;
    const Int sign_bit = static_cast<Int>(1) << signed_bits;
    Int wrapped = value % unsigned_range;
    if (wrapped < 0) {
        wrapped += unsigned_range;
    }
    if (wrapped >= sign_bit) {
        wrapped -= unsigned_range;
    }
    return wrapped;
}

Int applyOverflowPolicy(Int value, const std::string& policy) {
    if (equalsSymbol(policy, overflow_policy::kWrap)) {
        return wrap64(value);
    }
    if (equalsSymbol(policy, overflow_policy::kSaturate)) {
        return clamp64(value);
    }
    if (equalsSymbol(policy, overflow_policy::kTrap) && (value < int64Min() || value > int64Max())) {
        throw std::runtime_error("integer overflow");
    }
    return value;
}

std::pair<std::string, std::string> splitOpPolicy(const std::string& op) {
    const auto pos = op.find('_');
    if (pos == std::string::npos) {
        return {op, ""};
    }
    return {op.substr(0, pos), op.substr(pos + 1)};
}

int hexDigit(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return kHexAlphaOffset + ch - 'a';
    }
    if (ch >= 'A' && ch <= 'F') {
        return kHexAlphaOffset + ch - 'A';
    }
    throw std::runtime_error("bad hex string in bytecode");
}

std::string hexDecode(const std::string& text) {
    if (text.size() % kHexEncodedByteWidth != 0) {
        throw std::runtime_error("bad hex string length in bytecode");
    }
    std::string out;
    out.reserve(text.size() / kHexEncodedByteWidth);
    for (std::size_t i = 0; i < text.size(); i += kHexEncodedByteWidth) {
        out.push_back(static_cast<char>((hexDigit(text[i]) << kHexHighNibbleShift) | hexDigit(text[i + 1])));
    }
    return out;
}

const FunctionBytecode* findFunction(const BytecodeProgram& program, const std::string& name) {
    for (const auto& function : program.functions) {
        if (function.name == name) {
            return &function;
        }
    }
    return nullptr;
}

Value pop(std::vector<Value>& stack) {
    if (stack.empty()) {
        throw std::runtime_error("stack underflow");
    }
    auto value = stack.back();
    stack.pop_back();
    return value;
}

void pushBool(std::vector<Value>& stack, bool value) {
    stack.push_back(integerValue(value ? 1 : 0));
}

class Interpreter {
public:
    Interpreter(const BytecodeProgram& program, std::istream& input, std::ostream& output, Diagnostics& diagnostics, VmOptions options)
        : program_(program), input_(input), output_(output), diagnostics_(diagnostics), options_(options) {
        chains_[std::string{builtin::kRootAuthority}].insert("*");
    }

    bool run() {
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

private:
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

    Value execute(const FunctionBytecode& function, const std::vector<Value>& args, const std::string& caller) {
        if (args.size() != function.params.size()) {
            throw std::runtime_error("function '" + function.name + "' expected " + std::to_string(function.params.size()) +
                " arguments but got " + std::to_string(args.size()));
        }
        if (!function.callableFrom.empty() && !caller.empty() &&
            std::find(function.callableFrom.begin(), function.callableFrom.end(), caller) == function.callableFrom.end()) {
            throw std::runtime_error("function '" + function.name + "' is not callable from '" + caller + "'");
        }
        std::vector<Value> stack;
        std::unordered_map<std::string, std::string> localAddresses;
        const auto framePrefix = function.name + "#" + std::to_string(nextFrameId_++) + ":";
        auto ensureAddress = [&](const std::string& name) -> const std::string& {
            const auto [it, inserted] = localAddresses.emplace(name, framePrefix + name);
            if (inserted) {
                memory_.emplace(it->second, integerValue(0));
            }
            return it->second;
        };
        auto loadLocal = [&](const std::string& name) {
            return memory_[ensureAddress(name)];
        };
        auto storeLocal = [&](const std::string& name, Value value) {
            memory_[ensureAddress(name)] = std::move(value);
        };
        auto refLocal = [&](const std::string& name) {
            return refValue(ensureAddress(name));
        };
        auto loadRef = [&](const Value& ref) {
            if (ref.kind != ValueKind::Ref) {
                throw std::runtime_error("cannot dereference non-reference value");
            }
            return memory_[ref.string];
        };
        auto storeRef = [&](const Value& ref, Value value) {
            if (ref.kind != ValueKind::Ref) {
                throw std::runtime_error("cannot assign through non-reference value");
            }
            memory_[ref.string] = std::move(value);
        };
        for (const auto& local : function.locals) {
            ensureAddress(local);
        }
        for (std::size_t i = 0; i < function.params.size(); ++i) {
            storeLocal(function.params[i], args[i]);
        }

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
            const auto [baseOp, policy] = splitOpPolicy(op);

            if (op == opcodeName(Opcode::kVerify)) {
                const auto actual = functionFingerprint(function);
                if (actual != function.hash) {
                    throw std::runtime_error("Function identity verification failed for " + function.name);
                }
                ++pc;
            } else if (op == opcodeName(Opcode::kPushInteger)) {
                stack.push_back(integerValue(parseInteger(instruction.args.at(0))));
                ++pc;
            } else if (op == opcodeName(Opcode::kPushString)) {
                stack.push_back(stringValue(hexDecode(instruction.args.at(0))));
                ++pc;
            } else if (op == opcodeName(Opcode::kPushNull)) {
                stack.push_back(nullValue());
                ++pc;
            } else if (op == opcodeName(Opcode::kPushReference)) {
                stack.push_back(refLocal(instruction.args.at(0)));
                ++pc;
            } else if (op == opcodeName(Opcode::kDereference)) {
                const auto ref = pop(stack);
                stack.push_back(loadRef(ref));
                ++pc;
            } else if (op == opcodeName(Opcode::kFieldReference)) {
                auto ref = pop(stack);
                if (ref.kind != ValueKind::Ref) {
                    throw std::runtime_error("cannot access field through non-reference value");
                }
                ref.string += ".";
                ref.string += instruction.args.at(0);
                stack.push_back(std::move(ref));
                ++pc;
            } else if (op == opcodeName(Opcode::kLoad)) {
                stack.push_back(loadLocal(instruction.args.at(0)));
                ++pc;
            } else if (op == opcodeName(Opcode::kStore)) {
                storeLocal(instruction.args.at(0), pop(stack));
                ++pc;
            } else if (op == opcodeName(Opcode::kStoreDereference)) {
                const auto value = pop(stack);
                const auto ref = pop(stack);
                storeRef(ref, value);
                ++pc;
            } else if (op == opcodeName(Opcode::kStorePointer)) {
                storeLocal(instruction.args.at(0), stringValue(instruction.args.at(1)));
                ++pc;
            } else if (op == opcodeName(Opcode::kPop)) {
                pop(stack);
                ++pc;
            } else if (baseOp == opcodeName(Opcode::kAdd) || baseOp == opcodeName(Opcode::kSub) ||
                       baseOp == opcodeName(Opcode::kMul) || baseOp == opcodeName(Opcode::kDiv) ||
                       op == opcodeName(Opcode::kEqual) || op == opcodeName(Opcode::kNotEqual) ||
                       op == opcodeName(Opcode::kLess) || op == opcodeName(Opcode::kLessEqual) ||
                       op == opcodeName(Opcode::kGreater) || op == opcodeName(Opcode::kGreaterEqual) ||
                       op == opcodeName(Opcode::kAnd) || op == opcodeName(Opcode::kOr) ||
                       op == opcodeName(Opcode::kXor) || op == opcodeName(Opcode::kNand) ||
                       op == opcodeName(Opcode::kNor)) {
                const auto rightValue = pop(stack);
                const auto leftValue = pop(stack);
                const auto right = asInteger(rightValue);
                const auto left = asInteger(leftValue);
                if (baseOp == opcodeName(Opcode::kAdd)) {
                    if (leftValue.kind == ValueKind::String || rightValue.kind == ValueKind::String) {
                        stack.push_back(stringValue(leftValue.string + rightValue.string));
                    } else {
                        stack.push_back(integerValue(applyOverflowPolicy(left + right, policy)));
                    }
                } else if (baseOp == opcodeName(Opcode::kSub)) {
                    stack.push_back(integerValue(applyOverflowPolicy(left - right, policy)));
                } else if (baseOp == opcodeName(Opcode::kMul)) {
                    stack.push_back(integerValue(applyOverflowPolicy(left * right, policy)));
                } else if (baseOp == opcodeName(Opcode::kDiv)) {
                    if (right == 0) {
                        throw std::runtime_error("division by zero");
                    }
                    stack.push_back(integerValue(applyOverflowPolicy(left / right, policy)));
                } else if (op == opcodeName(Opcode::kEqual)) {
                    pushBool(stack, equalsValue(leftValue, rightValue));
                } else if (op == opcodeName(Opcode::kNotEqual)) {
                    pushBool(stack, !equalsValue(leftValue, rightValue));
                } else if (op == opcodeName(Opcode::kLess)) {
                    pushBool(stack, left < right);
                } else if (op == opcodeName(Opcode::kLessEqual)) {
                    pushBool(stack, left <= right);
                } else if (op == opcodeName(Opcode::kGreater)) {
                    pushBool(stack, left > right);
                } else if (op == opcodeName(Opcode::kGreaterEqual)) {
                    pushBool(stack, left >= right);
                } else if (op == opcodeName(Opcode::kAnd)) {
                    pushBool(stack, left != 0 && right != 0);
                } else if (op == opcodeName(Opcode::kOr)) {
                    pushBool(stack, left != 0 || right != 0);
                } else if (op == opcodeName(Opcode::kXor)) {
                    pushBool(stack, (left != 0) != (right != 0));
                } else if (op == opcodeName(Opcode::kNand)) {
                    pushBool(stack, !(left != 0 && right != 0));
                } else if (op == opcodeName(Opcode::kNor)) {
                    pushBool(stack, !(left != 0 || right != 0));
                }
                ++pc;
            } else if (op == opcodeName(Opcode::kNot)) {
                pushBool(stack, !asBool(pop(stack)));
                ++pc;
            } else if (op == opcodeName(Opcode::kJump)) {
                pc = toSize(parseInteger(instruction.args.at(0)));
            } else if (op == opcodeName(Opcode::kJumpIfZero)) {
                const auto target = toSize(parseInteger(instruction.args.at(0)));
                pc = !asBool(pop(stack)) ? target : pc + 1;
            } else if (op == opcodeName(Opcode::kCheckApproval)) {
                const auto& approval = instruction.args.at(0);
                if (equalsSymbol(approval, builtin::kAlwaysDeny)) {
                    throw std::runtime_error("Approval denied");
                }
                if (!equalsSymbol(approval, builtin::kAlwaysApprove)) {
                    const auto* approvalFunction = findFunction(program_, approval);
                    if (approvalFunction == nullptr || !equalsSymbol(approvalFunction->category, function_category::kApproval)) {
                        throw std::runtime_error("Unknown approval function '" + approval + "'");
                    }
                    const auto decision = execute(*approvalFunction, {stringValue("approve invocation of " + instruction.args.at(1))}, function.name);
                    if (!asBool(decision)) {
                        throw std::runtime_error("Approval denied");
                    }
                }
                ++pc;
            } else if (op == opcodeName(Opcode::kCheckLimits)) {
                const auto memoryLimit = parseInteger(instruction.args.at(0));
                const auto timeout = parseInteger(instruction.args.at(1));
                const auto predictedStack = parseInteger(instruction.args.at(2));
                if (memoryLimit <= 0 || timeout <= 0) {
                    throw std::runtime_error("invalid call resource limits");
                }
                if (predictedStack > static_cast<Int>(options_.maxSteps)) {
                    throw std::runtime_error("predicted stack depth exceeds VM limit");
                }
                ++pc;
            } else if (op == opcodeName(Opcode::kCheckRole)) {
                const auto* target = findFunction(program_, instruction.args.at(0));
                if (target == nullptr) {
                    throw std::runtime_error("unknown function '" + instruction.args.at(0) + "'");
                }
                const auto& chain = instruction.args.at(1);
                requireRole(*target, chain);
                ++pc;
            } else if (op == opcodeName(Opcode::kCheckRoleIndirect)) {
                const auto pointer = loadLocal(instruction.args.at(0));
                if (pointer.kind != ValueKind::String) {
                    throw std::runtime_error("function pointer '" + instruction.args.at(0) + "' is not assigned");
                }
                const auto* target = findFunction(program_, pointer.string);
                if (target == nullptr) {
                    throw std::runtime_error("unknown function '" + pointer.string + "'");
                }
                const auto& chain = instruction.args.at(1);
                requireRole(*target, chain);
                ++pc;
            } else if (op == opcodeName(Opcode::kNullCheck)) {
                const auto pointer = loadLocal(instruction.args.at(0));
                if (pointer.kind != ValueKind::String || pointer.string.empty()) {
                    throw std::runtime_error("null function pointer '" + instruction.args.at(0) + "'");
                }
                ++pc;
            } else if (op == opcodeName(Opcode::kCall)) {
                const auto* target = findFunction(program_, instruction.args.at(0));
                if (target == nullptr) {
                    throw std::runtime_error("unknown function '" + instruction.args.at(0) + "'");
                }
                const auto arity = toSize(parseInteger(instruction.args.at(1)));
                std::vector<Value> args;
                args.reserve(arity);
                for (std::size_t i = 0; i < arity; ++i) {
                    args.push_back(pop(stack));
                }
                std::reverse(args.begin(), args.end());
                const auto ret = execute(*target, args, function.name);
                if (target->returnable) {
                    stack.push_back(ret);
                }
                ++pc;
            } else if (op == opcodeName(Opcode::kCallIndirect)) {
                const auto pointer = loadLocal(instruction.args.at(0));
                if (pointer.kind != ValueKind::String) {
                    throw std::runtime_error("function pointer '" + instruction.args.at(0) + "' is not assigned");
                }
                const auto* target = findFunction(program_, pointer.string);
                if (target == nullptr) {
                    throw std::runtime_error("unknown function '" + pointer.string + "'");
                }
                const auto arity = toSize(parseInteger(instruction.args.at(1)));
                const bool discard = equalsSymbol(instruction.args.at(2), kInstructionTrue);
                std::vector<Value> args;
                args.reserve(arity);
                for (std::size_t i = 0; i < arity; ++i) {
                    args.push_back(pop(stack));
                }
                std::reverse(args.begin(), args.end());
                const auto ret = execute(*target, args, function.name);
                if (target->returnable && !discard) {
                    stack.push_back(ret);
                }
                ++pc;
            } else if (op == opcodeName(Opcode::kOutputInt)) {
                output_ << intToString(asInteger(pop(stack))) << '\n';
                ++pc;
            } else if (op == opcodeName(Opcode::kOutputChar)) {
                output_ << static_cast<char>(static_cast<unsigned char>(asInteger(pop(stack))));
                ++pc;
            } else if (op == opcodeName(Opcode::kPrintLine)) {
                output_ << valueToString(pop(stack)) << '\n';
                ++pc;
            } else if (op == opcodeName(Opcode::kInputInt)) {
                long long value = 0;
                input_ >> value;
                stack.push_back(integerValue(value));
                ++pc;
            } else if (op == opcodeName(Opcode::kInputChar)) {
                char ch = '\0';
                input_.get(ch);
                stack.push_back(integerValue(static_cast<unsigned char>(ch)));
                ++pc;
            } else if (op == opcodeName(Opcode::kVerifyEcc)) {
                ++pc;
            } else if (op == opcodeName(Opcode::kCopyMemory)) {
                (void)pop(stack);
                const auto src = pop(stack);
                const auto dst = pop(stack);
                if (src.kind != ValueKind::Ref || dst.kind != ValueKind::Ref) {
                    throw std::runtime_error("copymemory expects reference arguments");
                }
                storeRef(dst, loadRef(src));
                ++pc;
            } else if (op == opcodeName(Opcode::kCompareMemory)) {
                (void)pop(stack);
                const auto right = pop(stack);
                const auto left = pop(stack);
                if (left.kind != ValueKind::Ref || right.kind != ValueKind::Ref) {
                    throw std::runtime_error("comparememory expects reference arguments");
                }
                stack.push_back(integerValue(equalsValue(loadRef(left), loadRef(right)) ? 0 : 1));
                ++pc;
            } else if (op == opcodeName(Opcode::kRole)) {
                applyRoleInstruction(instruction);
                ++pc;
            } else if (op == opcodeName(Opcode::kOperatorInput)) {
                std::string answer;
                input_ >> answer;
                storeLocal(instruction.args.at(0), integerValue(equalsSymbol(answer, source_literal::kYes) ? 1 : 0));
                ++pc;
            } else if (op == opcodeName(Opcode::kReturn)) {
                if (function.returnable && !stack.empty()) {
                    return pop(stack);
                }
                return integerValue(0);
            } else if (op == opcodeName(Opcode::kHalt)) {
                return integerValue(0);
            } else {
                throw std::runtime_error("unknown opcode " + op);
            }
        }
        return integerValue(0);
    }

    const BytecodeProgram& program_;
    std::istream& input_;
    std::ostream& output_;
    Diagnostics& diagnostics_;
    VmOptions options_;
    std::size_t steps_ = 0;
    std::size_t nextFrameId_ = 0;
    std::unordered_map<std::string, Value> memory_;
    std::unordered_map<std::string, std::unordered_set<std::string>> chains_;
};

} // namespace

bool runBytecode(const BytecodeProgram& program, std::istream& input, std::ostream& output, Diagnostics& diagnostics, VmOptions options) {
    Interpreter interpreter(program, input, output, diagnostics, options);
    return interpreter.run();
}

} // namespace torture::vm
