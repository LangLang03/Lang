#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#ifndef TORTURE_BYTECODE_FORMAT_VERSION
#define TORTURE_BYTECODE_FORMAT_VERSION 3
#endif

namespace torture::vm::bytecode_format {

inline constexpr std::string_view kMagicText = "TBC";
inline constexpr auto kMagicBytes = std::to_array<char>({'T', 'B', 'C', '\0'});
inline constexpr std::uint16_t kVersion = TORTURE_BYTECODE_FORMAT_VERSION;
inline constexpr std::uint16_t kFlags = 0;
inline constexpr std::uint32_t kMaxStringBytes = 1024U * 1024U;
inline constexpr std::uint32_t kMaxCollectionItems = 1024U * 1024U;
inline constexpr std::uint32_t kBooleanFalse = 0;
inline constexpr std::uint32_t kBooleanTrue = 1;
inline constexpr std::string_view kDiagnosticDomain = "bytecode";

}  // namespace torture::vm::bytecode_format

namespace torture::vm::bytecode_diagnostic {

inline constexpr std::string_view kBadMagic = "bad-magic";
inline constexpr std::string_view kBadBoolean = "bad-boolean";
inline constexpr std::string_view kCollectionTooLarge = "collection-too-large";
inline constexpr std::string_view kEnvironmentMismatch = "environment-mismatch";
inline constexpr std::string_view kFunctionLimitOverflow = "function-limit-overflow";
inline constexpr std::string_view kHashMismatch = "hash-mismatch";
inline constexpr std::string_view kStringTooLarge = "string-too-large";
inline constexpr std::string_view kTruncated = "truncated";
inline constexpr std::string_view kUnknownOpcode = "unknown-opcode";
inline constexpr std::string_view kUnsupportedFlags = "unsupported-flags";
inline constexpr std::string_view kUnsupportedVersion = "unsupported-version";

}  // namespace torture::vm::bytecode_diagnostic
