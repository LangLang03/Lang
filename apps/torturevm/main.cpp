#include "common/diagnostic.h"
#include "vm/bytecode.h"
#include "vm/vm.h"

#include <fstream>
#include <iostream>
#include <string>

namespace {

void usage() {
    std::cerr << "usage:\n"
              << "  torturevm inspect <bytecode>\n"
              << "  torturevm run <bytecode>\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        usage();
        return 2;
    }

    torture::Diagnostics diagnostics;
    const std::string command = argv[1];
    const std::string path = argv[2];
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        diagnostics.error("io", "file-not-found", torture::SourceLocation{path, 1, 1}, "could not open bytecode file");
        torture::printDiagnostics(std::cerr, diagnostics);
        return 1;
    }

    auto bytecode = torture::vm::readBytecode(in, path, diagnostics);
    if (!bytecode) {
        torture::printDiagnostics(std::cerr, diagnostics);
        return 1;
    }

    if (command == "inspect") {
        std::cout << torture::vm::inspectBytecode(*bytecode);
        return 0;
    }
    if (command == "run") {
        const bool ok = torture::vm::runBytecode(*bytecode, std::cin, std::cout, diagnostics);
        if (!ok) {
            torture::printDiagnostics(std::cerr, diagnostics);
            return 1;
        }
        return 0;
    }

    usage();
    return 2;
}
