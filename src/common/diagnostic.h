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

// 错误码枚举：标识不同来源的错误类型
enum class ErrorCode {
    None,           // 无错误
    LexError,       // 词法分析错误
    ParseError,     // 语法分析错误
    SemaError,      // 语义分析错误
    BytecodeError,  // 字节码错误
    VmRuntimeError, // VM 运行时错误
    IoError,        // IO 错误
    Unknown,        // 未知错误
};

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string stage;
    std::string code;
    SourceLocation location;
    std::string message;
    // 错误码字段，默认为未知错误，与现有 code 字段并存以保持兼容
    ErrorCode errorCode = ErrorCode::Unknown;
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
