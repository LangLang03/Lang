#pragma once

#include "common/diagnostic.h"

#include <string>
#include <string_view>
#include <vector>

namespace torture::compiler {

enum class TokenKind {
    End,
    Identifier,
    Keyword,
    Number,
    String,
    Symbol,
};

struct Token {
    TokenKind kind = TokenKind::End;
    std::string text;
    SourceLocation location;
};

std::string_view tokenKindName(TokenKind kind);

struct LexResult {
    std::vector<Token> tokens;
};

} // namespace torture::compiler
