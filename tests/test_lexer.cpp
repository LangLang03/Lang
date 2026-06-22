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

TEST_CASE("lexer recognizes std/ffi/widthliteral/description keywords", "[lexer][std][ffi]") {
    // 全部新增 std/FFI/位宽/描述子句族关键字都应被识别为关键字。
    for (const char* keyword : {
        "std", "external", "apply", "arch", "sys", "lib", "symbol", "as", "signature", "sha512",
        "bit8", "bit16", "bit32", "bit64", "bit128",
        "of", "width", "from", "namespace",
        "declaredescription", "parameterdescription", "returndescription",
        "variabledescription", "typedefinitiondescription", "literaldescription",
        "stddecl", "stdbinding", "stdpurposestatement", "stdinfrastructurenote",
        // 11 archid
        "x86", "x64", "arm32", "arm64", "riscv32", "riscv64", "mips32", "mips64",
        "sparc32", "sparc64", "powerpc64",
        // 7 sysid
        "linux", "windows", "macos", "android", "freebsd", "openbsd", "uefi",
    }) {
        CHECK(torture::compiler::isKeyword(keyword));
    }
}

TEST_CASE("lexer still treats ordinary identifiers as non-keywords", "[lexer][std][ffi]") {
    CHECK_FALSE(torture::compiler::isKeyword("add"));
    CHECK_FALSE(torture::compiler::isKeyword("bindname"));
    CHECK_FALSE(torture::compiler::isKeyword("studentname"));
}

TEST_CASE("lexer accepts bit32 keyword without colliding with int", "[lexer][widthliteral]") {
    // 位宽关键字单独存在，且与 int 不冲突。
    CHECK(torture::compiler::isKeyword("bit32"));
    CHECK(torture::compiler::isKeyword("int"));
    CHECK(torture::compiler::isKeyword("uint"));
}
