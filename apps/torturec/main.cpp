#include "common/diagnostic.h"
#include "common/source.h"
#include "compiler/ast.h"
#include "compiler/bytecode_compiler.h"
#include "compiler/indentation.h"
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/sema.h"
#include "vm/bytecode.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

using torture::Diagnostics;
using torture::SourceFile;
using torture::compiler::Program;

void usage() {
    std::cerr << "usage:\n"
              << "  torturec lex <file>\n"
              << "  torturec parse <file>\n"
              << "  torturec check <file>\n"
              << "  torturec compile <file> [-o <bytecode>]\n";
}

std::optional<SourceFile> load(const std::string& path, Diagnostics& diagnostics) {
    return torture::readSourceFile(path, diagnostics);
}

std::optional<torture::compiler::LexResult> lexOnly(const SourceFile& source, Diagnostics& diagnostics) {
    auto lexed = torture::compiler::lexSource(source, diagnostics);
    if (diagnostics.hasErrors()) {
        return std::nullopt;
    }
    return lexed;
}

std::optional<Program> parsePipeline(const SourceFile& source, Diagnostics& diagnostics) {
    if (!torture::compiler::checkIndentation(source, diagnostics)) {
        return std::nullopt;
    }
    auto lexed = lexOnly(source, diagnostics);
    if (!lexed) {
        return std::nullopt;
    }
    auto parsed = torture::compiler::parseTokens(lexed->tokens, diagnostics);
    if (!parsed || diagnostics.hasErrors()) {
        return std::nullopt;
    }
    return parsed;
}

std::optional<Program> checkedPipeline(const SourceFile& source, Diagnostics& diagnostics) {
    auto parsed = parsePipeline(source, diagnostics);
    if (!parsed) {
        return std::nullopt;
    }
    if (!torture::compiler::checkProgramSemantics(*parsed, diagnostics)) {
        return std::nullopt;
    }
    return parsed;
}

int commandLex(const SourceFile& source, Diagnostics& diagnostics) {
    auto lexed = lexOnly(source, diagnostics);
    if (!lexed) {
        return 1;
    }
    for (const auto& token : lexed->tokens) {
        std::cout << token.location.line << ':' << token.location.column << ' '
                  << torture::compiler::tokenKindName(token.kind) << " `" << token.text << "`\n";
    }
    return 0;
}

int commandParse(const SourceFile& source, Diagnostics& diagnostics) {
    auto parsed = parsePipeline(source, diagnostics);
    if (!parsed) {
        return 1;
    }
    std::cout << torture::compiler::summarizeProgram(*parsed);
    return 0;
}

int commandCheck(const SourceFile& source, Diagnostics& diagnostics) {
    auto checked = checkedPipeline(source, diagnostics);
    if (!checked) {
        return 1;
    }
    std::cout << "ok\n";
    return 0;
}

int commandCompile(const SourceFile& source, int argc, char** argv, Diagnostics& diagnostics) {
    std::string outputPath;
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        } else {
            diagnostics.error("cli", "bad-argument", torture::SourceLocation{source.path, 1, 1}, "unknown compile argument '" + arg + "'");
            return 1;
        }
    }

    auto checked = checkedPipeline(source, diagnostics);
    if (!checked) {
        return 1;
    }
    auto bytecode = torture::compiler::compileToBytecode(*checked, diagnostics);
    if (!bytecode || diagnostics.hasErrors()) {
        return 1;
    }

    if (outputPath.empty()) {
        try {
            torture::vm::writeBytecode(std::cout, *bytecode);
        } catch (const std::exception& error) {
            diagnostics.error("bytecode", "write-failed", torture::SourceLocation{source.path, 1, 1}, error.what());
            return 1;
        }
        return 0;
    }

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        diagnostics.error("io", "write-failed", torture::SourceLocation{outputPath, 1, 1}, "could not open output bytecode file");
        return 1;
    }
    try {
        torture::vm::writeBytecode(out, *bytecode);
    } catch (const std::exception& error) {
        diagnostics.error("bytecode", "write-failed", torture::SourceLocation{outputPath, 1, 1}, error.what());
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    Diagnostics diagnostics;
    if (argc < 3) {
        usage();
        return 2;
    }

    const std::string command = argv[1];
    auto source = load(argv[2], diagnostics);
    if (!source) {
        torture::printDiagnostics(std::cerr, diagnostics);
        return 1;
    }

    int status = 2;
    if (command == "lex") {
        status = commandLex(*source, diagnostics);
    } else if (command == "parse") {
        status = commandParse(*source, diagnostics);
    } else if (command == "check") {
        status = commandCheck(*source, diagnostics);
    } else if (command == "compile") {
        status = commandCompile(*source, argc, argv, diagnostics);
    } else {
        usage();
        return 2;
    }

    if (diagnostics.hasErrors()) {
        torture::printDiagnostics(std::cerr, diagnostics);
    }
    return status;
}
