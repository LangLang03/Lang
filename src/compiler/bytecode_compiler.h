#pragma once

#include "common/diagnostic.h"
#include "compiler/ast.h"
#include "vm/bytecode.h"

#include <optional>

namespace torture::compiler {

std::optional<vm::BytecodeProgram> compileToBytecode(const Program& program, Diagnostics& diagnostics);

} // namespace torture::compiler
