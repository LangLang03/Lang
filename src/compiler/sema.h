#pragma once

#include "common/diagnostic.h"
#include "compiler/ast.h"

namespace torture::compiler {

bool checkProgramSemantics(const Program& program, Diagnostics& diagnostics);

} // namespace torture::compiler
