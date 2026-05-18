#include "compiler/indentation.h"

#include <string>
#include <string_view>
#include <vector>

namespace torture::compiler {
namespace {

std::string_view trimLeft(std::string_view line) {
    const auto first = line.find_first_not_of(' ');
    if (first == std::string_view::npos) {
        return {};
    }
    return line.substr(first);
}

std::size_t countLeadingSpaces(std::string_view line) {
    std::size_t count = 0;
    while (count < line.size() && line[count] == ' ') {
        ++count;
    }
    return count;
}

bool startsWith(std::string_view text, std::string_view prefix) {
    return text.substr(0, prefix.size()) == prefix;
}

bool isDeclarationContainerLine(std::string_view trimmed) {
    return startsWith(trimmed, "class ") || startsWith(trimmed, "struct ") || trimmed.find("implement") != std::string_view::npos;
}

struct BlockState {
    std::size_t indent = 0;
    bool needsProceed = true;
};

} // namespace

bool checkIndentation(const SourceFile& source, Diagnostics& diagnostics) {
    std::vector<BlockState> blocks;
    bool inComment = false;
    std::size_t lineNumber = 1;
    std::size_t offset = 0;

    while (offset <= source.text.size()) {
        auto next = source.text.find('\n', offset);
        if (next == std::string::npos) {
            next = source.text.size();
        }
        const std::string_view line(source.text.data() + offset, next - offset);
        const auto leading = countLeadingSpaces(line);
        const auto trimmed = trimLeft(line);

        if (!trimmed.empty() && leading % 4 != 0) {
            diagnostics.error(
                "indentation",
                "bad-indent-width",
                SourceLocation{source.path, lineNumber, leading + 1},
                "Indentation error: indentation must be a multiple of 4 spaces");
            return false;
        }

        bool commentOnly = false;
        if (inComment || startsWith(trimmed, "(*")) {
            commentOnly = true;
            const auto close = trimmed.find("*)");
            inComment = close == std::string_view::npos;
        }

        if (!trimmed.empty() && !commentOnly) {
            const bool closesBlock = trimmed.front() == '}';
            const auto expectedIndent = closesBlock ? (blocks.empty() ? 0 : blocks.back().indent) : blocks.size() * 4;
            if (leading != expectedIndent) {
                diagnostics.error(
                    "indentation",
                    "bad-indent",
                    SourceLocation{source.path, lineNumber, leading + 1},
                    "Indentation error: expected " + std::to_string(expectedIndent) + " spaces but found " +
                        std::to_string(leading));
                return false;
            }

            if (!blocks.empty() && blocks.back().needsProceed && !closesBlock) {
                if (!startsWith(trimmed, "proceed")) {
                    diagnostics.error(
                        "indentation",
                        "missing-proceed",
                        SourceLocation{source.path, lineNumber, leading + 1},
                        "block's first executable statement must start with proceed");
                    return false;
                }
                blocks.back().needsProceed = false;
            }
        }

        const bool declarationContainer = isDeclarationContainerLine(trimmed);
        for (std::size_t i = 0; i < trimmed.size(); ++i) {
            if (trimmed[i] == '{') {
                blocks.push_back(BlockState{leading, !declarationContainer});
            } else if (trimmed[i] == '}') {
                if (blocks.empty()) {
                    diagnostics.error(
                        "indentation",
                        "unmatched-close-brace",
                        SourceLocation{source.path, lineNumber, leading + i + 1},
                        "unmatched closing brace");
                    return false;
                }
                blocks.pop_back();
            }
        }

        if (next == source.text.size()) {
            break;
        }
        offset = next + 1;
        ++lineNumber;
    }

    if (!blocks.empty()) {
        diagnostics.error("indentation", "unclosed-block", SourceLocation{source.path, lineNumber, 1}, "unclosed block");
        return false;
    }
    if (inComment) {
        diagnostics.error("indentation", "unclosed-comment", SourceLocation{source.path, lineNumber, 1}, "unclosed comment");
        return false;
    }

    return true;
}

} // namespace torture::compiler
