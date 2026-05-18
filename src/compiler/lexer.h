#pragma once

#include "common/diagnostic.h"
#include "common/source.h"
#include "compiler/token.h"

#include <string_view>

namespace torture::compiler {

bool isKeyword(std::string_view text);
LexResult lexSource(const SourceFile& source, Diagnostics& diagnostics);

} // namespace torture::compiler
