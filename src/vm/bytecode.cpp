#include "vm/bytecode.h"

#include "common/sha256.h"
#include "vm/platform.h"

#include <sstream>
#include <string_view>

namespace torture::vm {
namespace {

constexpr std::string_view kReturnableYes = "yes";
constexpr std::string_view kReturnableNo = "no";

} // namespace

std::string instructionToString(const Instruction& instruction) {
    std::ostringstream out;
    out << opcodeName(instruction.op);
    for (const auto& arg : instruction.args) {
        out << ' ' << arg;
    }
    return out.str();
}

std::string functionFingerprint(const FunctionBytecode& function) {
    std::ostringstream out;
    out << function.name << '\n';
    out << function.maxStackSize << '\n';
    out << function.securityLevel << '\n';
    out << function.returnable << '\n';
    out << function.category << '\n';
    out << function.grantor << '\n';
    for (const auto& param : function.params) {
        out << "param " << param << '\n';
    }
    for (const auto& role : function.allowedRoles) {
        out << "role " << role << '\n';
    }
    for (const auto& caller : function.callableFrom) {
        out << "caller " << caller << '\n';
    }
    for (const auto& local : function.locals) {
        out << "local " << local << '\n';
    }
    for (const auto& instruction : function.code) {
        out << instructionToString(instruction) << '\n';
    }
    return torture::sha256Hex(out.str());
}

void finalizeFunctionHash(FunctionBytecode& function) {
    function.hash = functionFingerprint(function);
}

std::string inspectBytecode(const BytecodeProgram& program) {
    std::ostringstream out;
    out << "format: " << bytecode_format::kMagicText << " v" << bytecode_format::kVersion
        << " platform=" << kPlatformName << '\n';
    out << "environment: " << shortFingerprint(program.environmentFingerprint) << '\n';
    out << "entry: " << program.entry << '\n';
    out << "functions: " << program.functions.size() << '\n';
    for (const auto& function : program.functions) {
        out << "function " << function.name << " maxstack=" << function.maxStackSize
            << " security=" << function.securityLevel
            << " returnable=" << (function.returnable ? kReturnableYes : kReturnableNo)
            << " category=" << function.category
            << " hash=" << function.hash << '\n';
        for (const auto& param : function.params) {
            out << "  param " << param << '\n';
        }
        for (const auto& role : function.allowedRoles) {
            out << "  role " << role << '\n';
        }
        if (!function.grantor.empty()) {
            out << "  grantor " << function.grantor << '\n';
        }
        for (const auto& caller : function.callableFrom) {
            out << "  callable " << caller << '\n';
        }
        for (const auto& local : function.locals) {
            out << "  local " << local << '\n';
        }
        for (std::size_t i = 0; i < function.code.size(); ++i) {
            out << "  " << i << ": " << instructionToString(function.code[i]) << '\n';
        }
    }
    return out.str();
}

} // namespace torture::vm
