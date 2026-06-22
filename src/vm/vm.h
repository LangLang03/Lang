#pragma once

#include "common/diagnostic.h"
#include "vm/bytecode.h"

#include <iosfwd>

namespace torture::vm {

namespace vm_limits {

inline constexpr std::size_t DefaultMaxSteps = 100000000000;

} // namespace vm_limits

struct VmOptions {
    bool requireEcc = true;
    std::size_t maxSteps = vm_limits::DefaultMaxSteps;
};

bool runBytecode(const BytecodeProgram& program, std::istream& input, std::ostream& output, Diagnostics& diagnostics, VmOptions options = {});

} // namespace torture::vm
