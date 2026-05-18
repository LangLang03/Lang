#include "common/diagnostic.h"
#include "common/source.h"
#include "compiler/lexer.h"

#include <catch2/catch_test_macros.hpp>

using torture::Diagnostics;
using torture::SourceFile;

TEST_CASE("lexer emits strict tokens for a minimal program", "[lexer][smoke]") {
    SourceFile source{"<test>", "require ecc;\nfunction public callable returnable void main() {}\n"};
    Diagnostics diagnostics;

    auto result = torture::compiler::lexSource(source, diagnostics);

    REQUIRE_FALSE(diagnostics.hasErrors());
    REQUIRE(result.tokens.size() > 8);
    CHECK(result.tokens[0].text == "require");
    CHECK(result.tokens[1].text == "ecc");
    CHECK(result.tokens[2].text == ";");
}

TEST_CASE("lexer rejects mixed-case identifiers", "[lexer]") {
    SourceFile source{"<test>", "require ecc;\nfunction public callable returnable void myVar() {}\n"};
    Diagnostics diagnostics;

    torture::compiler::lexSource(source, diagnostics);

    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().front().code == "bad-identifier");
}

TEST_CASE("lexer rejects trailing spaces", "[lexer]") {
    SourceFile source{"<test>", "require ecc; \n"};
    Diagnostics diagnostics;

    torture::compiler::lexSource(source, diagnostics);

    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().front().code == "trailing-space");
}

TEST_CASE("lexer emits conditional and compound assignment symbols", "[lexer]") {
    SourceFile source{"<test>", "assign x += flag ? 1 : 2;\n"};
    Diagnostics diagnostics;

    auto result = torture::compiler::lexSource(source, diagnostics);

    REQUIRE_FALSE(diagnostics.hasErrors());
    CHECK(result.tokens[2].text == "+=");
    CHECK(result.tokens[4].text == "?");
    CHECK(result.tokens[6].text == ":");
}

TEST_CASE("lexer accepts UTF-8 string literals", "[lexer][utf8]") {
    SourceFile source{"<test>", "authorize invocation of println with arguments { value by value = literal \"成绩管理系统\" };\n"};
    Diagnostics diagnostics;

    auto result = torture::compiler::lexSource(source, diagnostics);

    REQUIRE_FALSE(diagnostics.hasErrors());
    bool found = false;
    for (const auto& token : result.tokens) {
        if (token.kind == torture::compiler::TokenKind::String && token.text == "成绩管理系统") {
            found = true;
        }
    }
    CHECK(found);
}
