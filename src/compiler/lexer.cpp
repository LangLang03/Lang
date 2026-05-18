#include "compiler/lexer.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>

namespace torture::compiler {
namespace {

const std::unordered_set<std::string>& keywords() {
    static const std::unordered_set<std::string> value = {
        "actor", "alignof", "allowed", "alwaysapprove", "alwaysdeny", "and", "approval", "arguments", "assign", "assume", "axiom",
        "abstractfactory", "at", "attach", "authority", "authorize", "because", "bool", "break", "bus", "by", "callable", "capture",
        "caseinsensitive", "casesensitive", "catch", "chain", "char", "class", "clockcycle", "clone", "code",
        "compare", "compute", "computational", "conditional", "confirm", "continue", "convene", "copy", "declare", "delegate", "deny", "destroy", "deserialize",
        "decreases", "detach", "discard", "discarding", "do", "ecc", "else", "elseifs", "ensures", "exception", "expects", "extend", "false", "field",
        "authorized", "dependencyinjection", "fieldcount", "fields", "float", "for", "from", "fptr", "function", "factory", "gate", "getproperty", "getsize",
        "global", "grant", "grantor", "hash", "if", "ignore", "immutable", "implement", "inherits", "injecting", "injects", "in", "inputchar", "into", "instantiate", "invocation",
        "inputint", "int", "integrity", "invariant", "invoke", "io", "isolation", "judge", "judging", "justify", "justification", "ledgerdivergence", "ledgeridentity", "level",
        "limit", "linkage", "literal", "local", "locals", "max", "memory", "method", "methods", "move", "mutable", "nand", "nominate",
        "nor", "not", "null", "nullcheck", "of", "onchange", "onread", "onwrite", "operation", "operator", "operatorinput", "over",
        "or", "outputchar", "outputint", "overflow", "patterns", "predictstackdepth", "println", "private", "proceed", "proof", "prompt", "protected",
        "prove", "ptr", "public", "purpose", "readable", "ref", "reference", "register", "require", "requires", "response", "return",
        "returnable", "revoke", "role", "roles", "saturate", "scope", "seal", "security", "serialize",
        "setproperty", "signature", "size", "stack", "storage", "struct", "synchronized", "than", "theorem", "therefore", "thread",
        "strategy", "timeout", "to", "trace", "trap", "tribunal", "true", "trustee", "type", "uint", "uncallable", "unseal",
        "until", "use", "value", "verifyfunctionidentity", "verifymemoryintegrity", "via", "void", "volatile",
        "where", "while", "wire", "with", "wrap", "writable", "xor", "yield", "yieldgate",
    };
    return value;
}

bool isPrintableAscii(unsigned char value) {
    return value >= 0x20U && value <= 0x7eU;
}

bool isUtf8Continuation(unsigned char value) {
    return (value & 0xc0U) == 0x80U;
}

bool validUtf8Sequence(std::string_view text, std::size_t index, std::size_t& length) {
    const auto lead = static_cast<unsigned char>(text[index]);
    if (lead >= 0xc2U && lead <= 0xdfU) {
        length = 2;
        return index + 1 < text.size() && isUtf8Continuation(static_cast<unsigned char>(text[index + 1]));
    }
    if (lead == 0xe0U) {
        length = 3;
        return index + 2 < text.size() &&
            static_cast<unsigned char>(text[index + 1]) >= 0xa0U &&
            static_cast<unsigned char>(text[index + 1]) <= 0xbfU &&
            isUtf8Continuation(static_cast<unsigned char>(text[index + 2]));
    }
    if ((lead >= 0xe1U && lead <= 0xecU) || (lead >= 0xeeU && lead <= 0xefU)) {
        length = 3;
        return index + 2 < text.size() &&
            isUtf8Continuation(static_cast<unsigned char>(text[index + 1])) &&
            isUtf8Continuation(static_cast<unsigned char>(text[index + 2]));
    }
    if (lead == 0xedU) {
        length = 3;
        return index + 2 < text.size() &&
            static_cast<unsigned char>(text[index + 1]) >= 0x80U &&
            static_cast<unsigned char>(text[index + 1]) <= 0x9fU &&
            isUtf8Continuation(static_cast<unsigned char>(text[index + 2]));
    }
    if (lead == 0xf0U) {
        length = 4;
        return index + 3 < text.size() &&
            static_cast<unsigned char>(text[index + 1]) >= 0x90U &&
            static_cast<unsigned char>(text[index + 1]) <= 0xbfU &&
            isUtf8Continuation(static_cast<unsigned char>(text[index + 2])) &&
            isUtf8Continuation(static_cast<unsigned char>(text[index + 3]));
    }
    if (lead >= 0xf1U && lead <= 0xf3U) {
        length = 4;
        return index + 3 < text.size() &&
            isUtf8Continuation(static_cast<unsigned char>(text[index + 1])) &&
            isUtf8Continuation(static_cast<unsigned char>(text[index + 2])) &&
            isUtf8Continuation(static_cast<unsigned char>(text[index + 3]));
    }
    if (lead == 0xf4U) {
        length = 4;
        return index + 3 < text.size() &&
            static_cast<unsigned char>(text[index + 1]) >= 0x80U &&
            static_cast<unsigned char>(text[index + 1]) <= 0x8fU &&
            isUtf8Continuation(static_cast<unsigned char>(text[index + 2])) &&
            isUtf8Continuation(static_cast<unsigned char>(text[index + 3]));
    }
    length = 0;
    return false;
}

bool isIdentifierStart(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) != 0;
}

bool isIdentifierContinue(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) != 0;
}

bool isAllLowerId(std::string_view text) {
    if (text.empty() || !std::islower(static_cast<unsigned char>(text.front()))) {
        return false;
    }
    return std::all_of(text.begin(), text.end(), [](char ch) {
        return std::islower(static_cast<unsigned char>(ch)) || std::isdigit(static_cast<unsigned char>(ch));
    });
}

bool isAllUpperId(std::string_view text) {
    if (text.empty() || !std::isupper(static_cast<unsigned char>(text.front()))) {
        return false;
    }
    return std::all_of(text.begin(), text.end(), [](char ch) {
        return std::isupper(static_cast<unsigned char>(ch)) || std::isdigit(static_cast<unsigned char>(ch));
    });
}

bool isSingleSymbol(char ch) {
    switch (ch) {
    case '{':
    case '}':
    case '(':
    case ')':
    case '[':
    case ']':
    case ';':
    case ',':
    case '.':
    case '*':
    case '&':
    case '=':
    case '<':
    case '>':
    case '+':
    case '-':
    case '/':
    case ':':
    case '!':
    case '?':
        return true;
    default:
        return false;
    }
}

void pushToken(std::vector<Token>& tokens, TokenKind kind, std::string text, const SourceLocation& location) {
    tokens.push_back(Token{kind, std::move(text), location});
}

} // namespace

std::string_view tokenKindName(TokenKind kind) {
    switch (kind) {
    case TokenKind::End:
        return "end";
    case TokenKind::Identifier:
        return "identifier";
    case TokenKind::Keyword:
        return "keyword";
    case TokenKind::Number:
        return "number";
    case TokenKind::String:
        return "string";
    case TokenKind::Symbol:
        return "symbol";
    }
    return "unknown";
}

bool isKeyword(std::string_view text) {
    return keywords().contains(std::string(text));
}

LexResult lexSource(const SourceFile& source, Diagnostics& diagnostics) {
    LexResult result;
    const auto& text = source.text;
    std::size_t index = 0;
    std::size_t line = 1;
    std::size_t column = 1;

    auto location = [&]() {
        return SourceLocation{source.path, line, column};
    };

    auto advance = [&]() -> char {
        const char ch = text[index++];
        if (ch == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
        return ch;
    };

    std::size_t lineStart = 0;
    std::size_t checkLine = 1;
    while (lineStart < text.size()) {
        std::size_t lineEnd = text.find('\n', lineStart);
        if (lineEnd == std::string::npos) {
            lineEnd = text.size();
        }
        if (lineEnd > lineStart && text[lineEnd - 1] == ' ') {
            diagnostics.error(
                "lexer",
                "trailing-space",
                SourceLocation{source.path, checkLine, lineEnd - lineStart},
                "line must not end with a space");
            return result;
        }
        if (lineEnd == text.size()) {
            break;
        }
        lineStart = lineEnd + 1;
        ++checkLine;
    }

    while (index < text.size()) {
        const char ch = text[index];
        if (ch == '\n' || ch == ' ') {
            advance();
            continue;
        }

        if (ch == '(' && index + 1 < text.size() && text[index + 1] == '*') {
            const auto start = location();
            advance();
            advance();
            bool closed = false;
            while (index < text.size()) {
                if (text[index] != '\n' && !isPrintableAscii(static_cast<unsigned char>(text[index]))) {
                    diagnostics.error("lexer", "illegal-byte", location(), "only printable ASCII is allowed outside string literals");
                    return result;
                }
                if (text[index] == '{' || text[index] == '}') {
                    diagnostics.error("lexer", "comment-brace", location(), "comments must not contain braces");
                    return result;
                }
                if (text[index] == '(' && index + 1 < text.size() && text[index + 1] == '*') {
                    diagnostics.error("lexer", "nested-comment", location(), "comments must not be nested");
                    return result;
                }
                if (text[index] == '*' && index + 1 < text.size() && text[index + 1] == ')') {
                    advance();
                    advance();
                    closed = true;
                    break;
                }
                advance();
            }
            if (!closed) {
                diagnostics.error("lexer", "unterminated-comment", start, "unterminated comment");
                return result;
            }
            continue;
        }

        if (ch == '"') {
            const auto start = location();
            std::string value;
            advance();
            bool closed = false;
            while (index < text.size()) {
                const char current = text[index];
                if (current == '\n') {
                    diagnostics.error("lexer", "newline-in-string", location(), "strings cannot contain a raw newline");
                    return result;
                }
                const auto byte = static_cast<unsigned char>(current);
                if (current == '"') {
                    advance();
                    closed = true;
                    break;
                }
                if (current == '\\') {
                    advance();
                    if (index >= text.size()) {
                        break;
                    }
                    const char escaped = text[index];
                    if (escaped != '\\' && escaped != '"') {
                        diagnostics.error("lexer", "bad-escape", location(), "only \\\\ and \\\" escapes are supported");
                        return result;
                    }
                    value.push_back(escaped);
                    advance();
                    continue;
                }
                if (byte < 0x20U || byte == 0x7fU) {
                    diagnostics.error("lexer", "illegal-string-byte", location(), "strings cannot contain raw control bytes");
                    return result;
                }
                if (byte >= 0x80U) {
                    std::size_t length = 0;
                    if (!validUtf8Sequence(text, index, length)) {
                        diagnostics.error("lexer", "bad-utf8", location(), "string literal contains invalid UTF-8");
                        return result;
                    }
                    for (std::size_t i = 0; i < length; ++i) {
                        value.push_back(advance());
                    }
                    continue;
                }
                value.push_back(current);
                advance();
            }
            if (!closed) {
                diagnostics.error("lexer", "unterminated-string", start, "unterminated string literal");
                return result;
            }
            pushToken(result.tokens, TokenKind::String, value, start);
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(ch)) != 0) {
            const auto start = location();
            std::string number;
            while (index < text.size() && std::isdigit(static_cast<unsigned char>(text[index])) != 0) {
                number.push_back(advance());
            }
            if (number.size() > 1 && number.front() == '0') {
                diagnostics.error("lexer", "leading-zero", start, "integer literals cannot have leading zeros");
                return result;
            }
            pushToken(result.tokens, TokenKind::Number, number, start);
            continue;
        }

        if (isIdentifierStart(ch)) {
            const auto start = location();
            std::string word;
            while (index < text.size() && isIdentifierContinue(text[index])) {
                word.push_back(advance());
            }
            if (!isAllLowerId(word) && !isAllUpperId(word)) {
                diagnostics.error("lexer", "bad-identifier", start, "identifiers must be all lower-case or all upper-case");
                return result;
            }
            pushToken(result.tokens, isKeyword(word) ? TokenKind::Keyword : TokenKind::Identifier, word, start);
            continue;
        }

        if (ch == '_') {
            diagnostics.error("lexer", "bad-identifier", location(), "underscores are not allowed in identifiers");
            return result;
        }

        if (!isPrintableAscii(static_cast<unsigned char>(ch))) {
            diagnostics.error("lexer", "illegal-byte", location(), "only printable ASCII is allowed outside string literals");
            return result;
        }

        const auto start = location();
        if (index + 1 < text.size()) {
            const std::string two{text[index], text[index + 1]};
            if (two == "==" || two == "!=" || two == "<=" || two == ">=" || two == "&&" || two == "||" ||
                two == "+=" || two == "-=" || two == "*=" || two == "/=") {
                advance();
                advance();
                pushToken(result.tokens, TokenKind::Symbol, two, start);
                continue;
            }
        }
        if (isSingleSymbol(ch)) {
            pushToken(result.tokens, TokenKind::Symbol, std::string(1, advance()), start);
            continue;
        }

        diagnostics.error("lexer", "unexpected-character", location(), "unexpected character");
        return result;
    }

    result.tokens.push_back(Token{TokenKind::End, "", SourceLocation{source.path, line, column}});
    return result;
}

} // namespace torture::compiler
