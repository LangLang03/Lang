#pragma once

#include "common/diagnostic.h"
#include "common/source.h"

namespace torture::compiler {

bool checkIndentation(const SourceFile& source, Diagnostics& diagnostics);

} // namespace torture::compiler
