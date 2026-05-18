#include "common/source.h"

#include <fstream>
#include <sstream>

namespace torture {

std::optional<SourceFile> readSourceFile(const std::string& path, Diagnostics& diagnostics) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        diagnostics.error("io", "file-not-found", SourceLocation{path, 1, 1}, "could not open source file");
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    if (!in.good() && !in.eof()) {
        diagnostics.error("io", "read-failed", SourceLocation{path, 1, 1}, "could not read source file");
        return std::nullopt;
    }

    return SourceFile{path, buffer.str()};
}

} // namespace torture
