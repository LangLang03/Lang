#pragma once

#include "common/diagnostic.h"
#include "vm/bytecode_format.h"
#include "vm/environment.h"
#include "vm/language_symbols.h"
#include "vm/opcode.h"

#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace torture::vm {

struct FunctionBytecode {
    std::string name;
    int maxStackSize = 0;
    int securityLevel = 0;
    bool returnable = false;
    std::string category = std::string{function_category::kCallable};
    std::string hash;
    std::string grantor;
    std::vector<std::string> params;
    std::vector<std::string> allowedRoles;
    std::vector<std::string> callableFrom;
    std::vector<std::string> locals;
    std::vector<Instruction> code;
};

struct BytecodeProgram {
    std::uint8_t platformId = kPlatformId;
    std::string environmentFingerprint = currentEnvironmentFingerprint();
    std::string producer = environmentSummary();
    std::string entry = std::string{source_function::kMain};
    std::vector<FunctionBytecode> functions;
};

std::string instructionToString(const Instruction& instruction);
std::string functionFingerprint(const FunctionBytecode& function);
void finalizeFunctionHash(FunctionBytecode& function);

void writeBytecode(std::ostream& out, const BytecodeProgram& program);
std::optional<BytecodeProgram> readBytecode(std::istream& in, const std::string& name, torture::Diagnostics& diagnostics);
std::string inspectBytecode(const BytecodeProgram& program);

} // namespace torture::vm
