#include "common/diagnostic.h"

#include <utility>

namespace torture {

void Diagnostics::error(std::string stage, std::string code, SourceLocation location, std::string message) {
    diagnostics_.push_back(Diagnostic{
        DiagnosticSeverity::Error,
        std::move(stage),
        std::move(code),
        std::move(location),
        std::move(message),
    });
}

void Diagnostics::warning(std::string stage, std::string code, SourceLocation location, std::string message) {
    diagnostics_.push_back(Diagnostic{
        DiagnosticSeverity::Warning,
        std::move(stage),
        std::move(code),
        std::move(location),
        std::move(message),
    });
}

bool Diagnostics::hasErrors() const {
    for (const auto& diagnostic : diagnostics_) {
        if (diagnostic.severity == DiagnosticSeverity::Error) {
            return true;
        }
    }
    return false;
}

const std::vector<Diagnostic>& Diagnostics::all() const {
    return diagnostics_;
}

void Diagnostics::clear() {
    diagnostics_.clear();
}

std::ostream& operator<<(std::ostream& out, const Diagnostic& diagnostic) {
    out << diagnostic.location.file << ':' << diagnostic.location.line << ':' << diagnostic.location.column
        << ": " << (diagnostic.severity == DiagnosticSeverity::Error ? "error" : "warning")
        << '[' << diagnostic.stage << ':' << diagnostic.code << "]: " << diagnostic.message;
    return out;
}

void printDiagnostics(std::ostream& out, const Diagnostics& diagnostics) {
    for (const auto& diagnostic : diagnostics.all()) {
        out << diagnostic << '\n';
    }
}

} // namespace torture
