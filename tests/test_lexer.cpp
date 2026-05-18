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
