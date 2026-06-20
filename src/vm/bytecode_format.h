#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "vm/platform.h"
#include "vm/platform_bytecode.h"

#ifndef TORTURE_BYTECODE_FORMAT_VERSION
#define TORTURE_BYTECODE_FORMAT_VERSION 3
#endif

namespace torture::vm::bytecode_format {

// kMagicText 改写为指向 platform_bytecode::kMagicBytes 的 4 字节视图，
// 不同平台编译出不同文本（如 "TL64"、"TW64"、"TA64" 等）。
// 保留 std::string_view 类型以维持 inspectBytecode 旧 API 兼容。
inline constexpr std::string_view kMagicText{
    platform_bytecode::kMagicBytes.data(),
    platform_bytecode::kMagicBytes.size()};

// 旧 API 兼容别名：序列化时仍用 platform_bytecode 提供的魔数数组。
inline constexpr auto& kMagicBytes = platform_bytecode::kMagicBytes;

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
inline constexpr std::string_view kEndianMismatch = "endian-mismatch";
inline constexpr std::string_view kEnvironmentMismatch = "environment-mismatch";
inline constexpr std::string_view kFunctionLimitOverflow = "function-limit-overflow";
inline constexpr std::string_view kHashMismatch = "hash-mismatch";
inline constexpr std::string_view kPlatformMismatch = "platform-mismatch";
inline constexpr std::string_view kStringTooLarge = "string-too-large";
inline constexpr std::string_view kTruncated = "truncated";
inline constexpr std::string_view kUnknownOpcode = "unknown-opcode";
inline constexpr std::string_view kUnsupportedFlags = "unsupported-flags";
inline constexpr std::string_view kUnsupportedVersion = "unsupported-version";

}  // namespace torture::vm::bytecode_diagnostic
