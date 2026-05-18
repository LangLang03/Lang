#pragma once

#include "common/diagnostic.h"
#include "compiler/ast.h"
#include "compiler/token.h"

#include <optional>
#include <span>

namespace torture::compiler {

std::optional<Program> parseTokens(std::span<const Token> tokens, Diagnostics& diagnostics);

} // namespace torture::compiler
