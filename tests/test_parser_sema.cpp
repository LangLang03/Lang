#include "common/diagnostic.h"
#include "common/source.h"
#include "compiler/indentation.h"
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/sema.h"

#include <catch2/catch_test_macros.hpp>

#include <string_view>

namespace {

constexpr auto validProgram = R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local int:32 x = 41;
    assign x = compute (x + 1) with overflow trap;
    authorize invocation of outputint at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "declared io operation" with justification "print" with arguments { value by value = x } discarding return predictstackdepth 4 with authority chain root with approval of alwaysapprove with approval justification "ok" with approval timeout 1000;
}
)";

bool hasDiagnosticCode(const torture::Diagnostics& diagnostics, std::string_view code) {
    for (const auto& diagnostic : diagnostics.all()) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

TEST_CASE("parser and sema accept the first vertical smoke program", "[parser][sema][smoke]") {
    torture::SourceFile source{"<test>", validProgram};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    REQUIRE_FALSE(diagnostics.hasErrors());
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK(parsed->functions.size() == 1);
    CHECK(parsed->functions.front().name == "main");
    CHECK(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    CHECK_FALSE(diagnostics.hasErrors());
}

TEST_CASE("indentation smoke catches bad indent widths", "[indentation][smoke]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
   proceed verifyfunctionidentity();
}
)"};
    torture::Diagnostics diagnostics;

    CHECK_FALSE(torture::compiler::checkIndentation(source, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().front().code == "bad-indent-width");
}

TEST_CASE("sema requires verifyfunctionidentity first", "[sema]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed declare mutable readable writable purpose computational scope local int:32 x = 0;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().back().code == "missing-verifyfunctionidentity");
}

TEST_CASE("sema accepts structs only when all required methods exist", "[sema][struct][smoke]") {
    const char* names[] = {
        "init", "destroy", "copy", "move", "serialize", "deserialize", "tostring", "fromstring", "hash", "compare",
        "equals", "getproperty", "setproperty", "invoke", "getsize", "alignof", "defaultvalue", "validate", "mutate",
        "clone",
    };
    std::string text = "require ecc;\nstruct thing = struct { } implement {\n";
    for (const auto* name : names) {
        text += "method public callable returnable void ";
        text += name;
        text += "() code size 128 max stack size 64 { proceed verifyfunctionidentity(); }\n";
    }
    text += "}\n";

    torture::SourceFile source{"<test>", text};
    torture::Diagnostics diagnostics;
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    REQUIRE_FALSE(diagnostics.hasErrors());
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());

    CHECK(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    CHECK_FALSE(diagnostics.hasErrors());
}

TEST_CASE("sema rejects operatorinput outside approval functions", "[sema][approval]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local bool:1 decision = false;
    operatorinput prompt "approve" timeout 1000 into decision with io operation because literal "declared operator input";
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().back().code == "operatorinput-outside-approval");
}

TEST_CASE("parser rejects impure event blocks", "[parser][event]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local int:32 x { onread assign x = 1; } = 0;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    CHECK_FALSE(parsed.has_value());
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().back().code == "impure-event-block");
}

TEST_CASE("sema rejects mismatched function pointer assignments", "[sema][fptr]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable void target(readable int:32 x) code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
}
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose linkage scope local fptr<return:void, params:(int:64), security:1, maxstack:64, codesize:128> fp;
    authorize fptr assignment of fp to target with capture { } with attest "bad" with authority chain root;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().back().code == "fptr-param-mismatch");
}

TEST_CASE("sema enforces readable and writable variable access", "[sema][access]") {
    torture::SourceFile readonlySource{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable purpose computational scope local int:32 x = 0;
    assign x = 1;
}
)"};
    torture::Diagnostics readonlyDiagnostics;
    REQUIRE(torture::compiler::checkIndentation(readonlySource, readonlyDiagnostics));
    auto readonlyLexed = torture::compiler::lexSource(readonlySource, readonlyDiagnostics);
    auto readonlyParsed = torture::compiler::parseTokens(readonlyLexed.tokens, readonlyDiagnostics);
    REQUIRE(readonlyParsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*readonlyParsed, readonlyDiagnostics));
    REQUIRE(readonlyDiagnostics.hasErrors());
    CHECK(readonlyDiagnostics.all().back().code == "write-not-allowed");

    torture::SourceFile writeonlySource{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable writable purpose computational scope local int:32 x = 1;
    authorize invocation of outputint at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "declared io operation" with justification "print" with arguments { value by value = x } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "ok" with approval timeout 1000;
}
)"};
    torture::Diagnostics writeonlyDiagnostics;
    REQUIRE(torture::compiler::checkIndentation(writeonlySource, writeonlyDiagnostics));
    auto writeonlyLexed = torture::compiler::lexSource(writeonlySource, writeonlyDiagnostics);
    auto writeonlyParsed = torture::compiler::parseTokens(writeonlyLexed.tokens, writeonlyDiagnostics);
    REQUIRE(writeonlyParsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*writeonlyParsed, writeonlyDiagnostics));
    REQUIRE(writeonlyDiagnostics.hasErrors());
    CHECK(writeonlyDiagnostics.all().back().code == "read-not-allowed");
}

TEST_CASE("sema rejects direct assignment across purposes", "[sema][purpose]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local int:32 src = 1;
    declare mutable readable writable purpose storage scope local int:32 dst = 0;
    assign dst = src;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().back().code == "purpose-mismatch");
}

TEST_CASE("sema requires println strings to be declared as literals", "[sema][literal]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "declared io operation" with justification "print" with arguments { value by value = "HelloWorld" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "ok" with approval timeout 1000;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().back().code == "println-requires-literal");
}

TEST_CASE("sema requires IO operations to declare themselves", "[sema][io]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local int:32 x = 7;
    authorize invocation of outputint at security level 1 with memory limit 128 with timeout 1000 with justification "print" with arguments { value by value = x } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "ok" with approval timeout 1000;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().back().code == "missing-io-declaration");
}

TEST_CASE("sema requires class memory to be readable", "[sema][class]") {
    torture::SourceFile source{"<test>", R"(require ecc;
class printer fields 0 methods 3 authorized by root because literal "needed for excessive object ceremony" memory writable inherits object because literal "root inheritance is administratively mandated" patterns abstractfactory strategy dependencyinjection injects console {
} implement {
    method public callable returnable void init() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void destroy() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void print() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().back().code == "class-memory-not-readable");
}

TEST_CASE("sema enforces declared local variable counts", "[sema][locals]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local int:32 x = 0;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().back().code == "local-count-mismatch");
}

TEST_CASE("sema enforces class method counts", "[sema][class]") {
    torture::SourceFile source{"<test>", R"(require ecc;
class printer fields 0 methods 2 authorized by root because literal "needed for excessive object ceremony" memory readable writable inherits object because literal "root inheritance is administratively mandated" patterns abstractfactory strategy dependencyinjection injects console {
} implement {
    method public callable returnable void init() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void destroy() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void print() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().front().code == "method-count-mismatch");
}

TEST_CASE("sema requires class instantiation authority", "[sema][class][authority]") {
    torture::SourceFile source{"<test>", R"(require ecc;
class printer fields 0 methods 3 authorized by root because literal "needed for excessive object ceremony" memory readable writable inherits object because literal "root inheritance is administratively mandated" patterns abstractfactory strategy dependencyinjection injects console {
} implement {
    method public callable returnable void init() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void destroy() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void print() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
}
function public callable returnable void main() code size 128 max stack size 64 locals 1 authorized by root requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    instantiate printer p memory readable writable from printerfactory using strategy characterstrategy injecting console because literal "creation requires a written permission trail";
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(diagnostics.all().back().code == "missing-instantiation-authority");
}

TEST_CASE("sema accepts comparisons with matching bit widths and pointer permissions", "[sema][types][compare]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 256 max stack size 128 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local int:32 signedvalue = 1;
    declare mutable readable writable purpose computational scope local uint:32 unsignedvalue = 2;
    declare mutable readable writable purpose computational scope local char:8 letter = 65;
    declare mutable readable writable purpose computational scope local char:8 other = 66;
    declare mutable readable writable purpose computational scope local bool:1 enabled = true;
    declare mutable readable writable purpose computational scope local bool:1 disabled = false;
    declare mutable readable writable purpose computational scope local ptr<readable, int:32> left;
    declare mutable readable writable purpose computational scope local ptr<readable, int:32> right;
    declare mutable readable writable purpose computational scope local bool:1 numericok = signedvalue < unsignedvalue;
    declare mutable readable writable purpose computational scope local bool:1 charok = letter != other;
    declare mutable readable writable purpose computational scope local bool:1 boolok = enabled == disabled;
    declare mutable readable writable purpose computational scope local bool:1 ptrok = left == right;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    CHECK_FALSE(diagnostics.hasErrors());
}

TEST_CASE("sema rejects types without required bit widths", "[sema][types]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local int missing = 0;
    declare mutable readable writable purpose computational scope local ptr<readable, int> pointer;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(hasDiagnosticCode(diagnostics, "invalid-type"));
}

TEST_CASE("sema rejects numeric comparisons with mismatched bit widths", "[sema][types][compare]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local int:32 narrow = 1;
    declare mutable readable writable purpose computational scope local int:64 wide = 1;
    declare mutable readable writable purpose computational scope local bool:1 same = narrow == wide;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(hasDiagnosticCode(diagnostics, "comparison-width-mismatch"));
}

TEST_CASE("sema rejects boolean order comparisons", "[sema][types][compare]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local bool:1 left = true;
    declare mutable readable writable purpose computational scope local bool:1 right = false;
    declare mutable readable writable purpose computational scope local bool:1 ordered = left < right;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(hasDiagnosticCode(diagnostics, "bool-order-comparison"));
}

TEST_CASE("sema rejects pointer comparisons with different access permissions", "[sema][types][compare][ptr]") {
    torture::SourceFile source{"<test>", R"(require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local ptr<readable, int:32> readableptr;
    declare mutable readable writable purpose computational scope local ptr<writable, int:32> writableptr;
    declare mutable readable writable purpose computational scope local bool:1 same = readableptr == writableptr;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(hasDiagnosticCode(diagnostics, "pointer-comparison-type-mismatch"));
}

TEST_CASE("sema rejects direct struct operator comparisons", "[sema][types][compare][struct]") {
    torture::SourceFile source{"<test>", R"(require ecc;
class gradebook fields 0 methods 3 authorized by root because literal "type ceremony for comparison checks" memory readable writable inherits object because literal "root inheritance is administratively mandated" patterns abstractfactory strategy dependencyinjection injects console {
} implement {
    method public callable returnable void init() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void destroy() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void print() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
}
function public callable returnable void main() code size 256 max stack size 128 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose storage scope local gradebook left;
    declare mutable readable writable purpose storage scope local gradebook right;
    declare mutable readable writable purpose computational scope local bool:1 same = left == right;
}
)"};
    torture::Diagnostics diagnostics;

    REQUIRE(torture::compiler::checkIndentation(source, diagnostics));
    const auto lexed = torture::compiler::lexSource(source, diagnostics);
    auto parsed = torture::compiler::parseTokens(lexed.tokens, diagnostics);
    REQUIRE(parsed.has_value());
    CHECK_FALSE(torture::compiler::checkProgramSemantics(*parsed, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    CHECK(hasDiagnosticCode(diagnostics, "struct-comparison-requires-method"));
}
