#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace torture {

struct SourceLocation {
    std::string file;
    std::size_t line = 1;
    std::size_t column = 1;
};

enum class DiagnosticSeverity {
    Error,
    Warning,
};

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string stage;
    std::string code;
    SourceLocation location;
    std::string message;
};

class Diagnostics {
public:
    void error(std::string stage, std::string code, SourceLocation location, std::string message);
    void warning(std::string stage, std::string code, SourceLocation location, std::string message);

    [[nodiscard]] bool hasErrors() const;
    [[nodiscard]] const std::vector<Diagnostic>& all() const;
    void clear();

private:
    std::vector<Diagnostic> diagnostics_;
};

std::ostream& operator<<(std::ostream& out, const Diagnostic& diagnostic);
void printDiagnostics(std::ostream& out, const Diagnostics& diagnostics);

} // namespace torture
