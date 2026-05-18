#pragma once

#include "common/diagnostic.h"

#include <optional>
#include <string>

namespace torture {

struct SourceFile {
    std::string path;
    std::string text;
};

std::optional<SourceFile> readSourceFile(const std::string& path, Diagnostics& diagnostics);

} // namespace torture
