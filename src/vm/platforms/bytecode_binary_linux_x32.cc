#include "vm/bytecode.h"

#include "common/sha256.h"
#include "vm/platform.h"
#include "vm/platform_bytecode.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace torture::vm {
namespace {

namespace field_name {

constexpr std::string_view kEnvironmentFingerprint = "environment-fingerprint";
constexpr std::string_view kEntry = "entry";
constexpr std::string_view kByteOrderLabel = "byte-order-label";
constexpr std::string_view kFlags = "flags";
constexpr std::string_view kFunctionCallableFrom = "function-callable-from";
constexpr std::string_view kFunctionCategory = "function-category";
constexpr std::string_view kFunctionCount = "function-count";
constexpr std::string_view kFunctionGrantor = "function-grantor";
constexpr std::string_view kFunctionHash = "function-hash";
constexpr std::string_view kFunctionLocals = "function-locals";
constexpr std::string_view kFunctionMaxStack = "function-max-stack";
constexpr std::string_view kFunctionName = "function-name";
constexpr std::string_view kFunctionParams = "function-params";
constexpr std::string_view kFunctionReturnable = "function-returnable";
constexpr std::string_view kFunctionRoles = "function-roles";
constexpr std::string_view kFunctionSecurityLevel = "function-security-level";
constexpr std::string_view kInstructionArgs = "instruction-args";
constexpr std::string_view kInstructionCount = "instruction-count";
constexpr std::string_view kOpcode = "opcode";
constexpr std::string_view kPlatformId = "platform-id";
constexpr std::string_view kProducer = "producer";
constexpr std::string_view kStringVector = "string vector";
constexpr std::string_view kVersion = "version";

}  // namespace field_name

constexpr std::string_view kFunctionVectorLabel = "function vector";
constexpr std::string_view kInstructionVectorLabel = "instruction vector";
constexpr std::string_view kMaxStackLabel = "function max stack size";
constexpr std::string_view kSecurityLevelLabel = "function security level";
constexpr int kDiagnosticLine = 1;
constexpr int kDiagnosticColumn = 1;
constexpr int kByteBits = std::numeric_limits<std::uint8_t>::digits;
constexpr auto kByteMask = std::numeric_limits<std::uint8_t>::max();
constexpr std::size_t kSha256HexLength = 64;

std::uint32_t CheckedCount(std::size_t value, std::string_view label) {
  if (value > bytecode_format::kMaxCollectionItems) {
    throw std::runtime_error(std::string{label} + " exceeds bytecode collection limit");
  }
  return static_cast<std::uint32_t>(value);
}

std::uint32_t CheckedStringSize(std::size_t value) {
  if (value > bytecode_format::kMaxStringBytes) {
    throw std::runtime_error("string exceeds bytecode string limit");
  }
  return static_cast<std::uint32_t>(value);
}

std::uint32_t CheckedNonNegative(int value, std::string_view label) {
  if (value < 0) {
    throw std::runtime_error(std::string{label} + " must not be negative");
  }
  return static_cast<std::uint32_t>(value);
}

// linux_x32 序列化使用大端编码，字段顺序与现有 bytecode_binary.cc 和 linux_x64 都不同：
// magic(4) -> byte_order(1) -> flags(2 BE) -> version(2 BE) -> platform_id(1)
// -> env_fingerprint -> entry -> producer -> functions -> 尾部SHA256(64 字符十六进制)
class BytecodeWriter {
 public:
  explicit BytecodeWriter(std::ostream& output) : output_(output) {}

  template <typename Integer>
  void WriteBigEndian(Integer value) {
    static_assert(std::is_integral_v<Integer>);
    using UnsignedInteger = std::make_unsigned_t<Integer>;
    auto remaining = static_cast<UnsignedInteger>(value);
    for (std::size_t byte_index = sizeof(Integer); byte_index > 0; --byte_index) {
      const auto shift = (byte_index - 1) * kByteBits;
      buffer_.push_back(static_cast<char>((remaining >> shift) & kByteMask));
    }
  }

  void WriteRawByte(std::uint8_t value) {
    buffer_.push_back(static_cast<char>(value));
  }

  void WriteRawBytes(const char* data, std::size_t size) {
    buffer_.append(data, size);
  }

  void WriteString(std::string_view text) {
    WriteBigEndian(CheckedStringSize(text.size()));
    buffer_.append(text.data(), text.size());
  }

  void WriteStringVector(const std::vector<std::string>& values) {
    WriteBigEndian(CheckedCount(values.size(), field_name::kStringVector));
    for (const auto& value : values) {
      WriteString(value);
    }
  }

  void FlushWithTrailer() {
    // 计算已写入内容的 SHA256 哈希（十六进制字符串）
    const std::string hash_hex = ::torture::sha256Hex(
        std::string_view{buffer_.data(), buffer_.size()});
    // 写入所有内容到输出流
    output_.write(buffer_.data(), static_cast<std::streamsize>(buffer_.size()));
    // 写入尾部 SHA256 十六进制校验
    output_.write(hash_hex.data(), static_cast<std::streamsize>(hash_hex.size()));
    if (!output_) {
      throw std::runtime_error("failed to write bytecode stream");
    }
  }

 private:
  std::ostream& output_;
  std::string buffer_;
};

class BytecodeReader {
 public:
  BytecodeReader(std::istream& input, std::string name, Diagnostics& diagnostics)
      : input_(input), name_(std::move(name)), diagnostics_(diagnostics) {}

  bool ReadMagic() {
    std::array<char, platform_bytecode::kMagicBytes.size()> actual = {};
    if (!ReadRaw(actual.data(), actual.size())) {
      return Fail(bytecode_diagnostic::kBadMagic, "not a Torture binary bytecode file");
    }
    if (actual != platform_bytecode::kMagicBytes) {
      return Fail(
          bytecode_diagnostic::kBadMagic,
          "bytecode magic does not match this platform '" + std::string{kPlatformName} + "'");
    }
    return true;
  }

  std::optional<std::uint8_t> ReadByte(std::string_view field) {
    char value = '\0';
    if (!ReadRaw(&value, sizeof(value))) {
      Fail(bytecode_diagnostic::kTruncated, std::string{"bytecode ended while reading "} + std::string{field});
      return std::nullopt;
    }
    return static_cast<std::uint8_t>(value);
  }

  template <typename Integer>
  std::optional<Integer> ReadBigEndian(std::string_view field) {
    static_assert(std::is_integral_v<Integer>);
    using UnsignedInteger = std::make_unsigned_t<Integer>;
    UnsignedInteger value = 0;
    for (std::size_t byte_index = 0; byte_index < sizeof(Integer); ++byte_index) {
      const auto byte = ReadByte(field);
      if (!byte) {
        return std::nullopt;
      }
      const auto shift = (sizeof(Integer) - 1 - byte_index) * kByteBits;
      value |= static_cast<UnsignedInteger>(*byte) << shift;
    }
    return static_cast<Integer>(value);
  }

  std::optional<std::string> ReadString(std::string_view field) {
    const auto size = ReadBigEndian<std::uint32_t>(field);
    if (!size) {
      return std::nullopt;
    }
    if (*size > bytecode_format::kMaxStringBytes) {
      Fail(bytecode_diagnostic::kStringTooLarge, std::string{"string field '"} + std::string{field} + "' exceeds bytecode limit");
      return std::nullopt;
    }
    std::string text(*size, '\0');
    if (!text.empty() && !ReadRaw(text.data(), text.size())) {
      Fail(bytecode_diagnostic::kTruncated, std::string{"bytecode ended while reading string field '"} + std::string{field} + "'");
      return std::nullopt;
    }
    return text;
  }

  bool ReadStringVector(std::vector<std::string>& output, std::string_view field) {
    const auto count = ReadBigEndian<std::uint32_t>(field);
    if (!count) {
      return false;
    }
    if (*count > bytecode_format::kMaxCollectionItems) {
      return Fail(bytecode_diagnostic::kCollectionTooLarge, std::string{"vector field '"} + std::string{field} + "' exceeds bytecode limit");
    }
    output.reserve(*count);
    for (std::uint32_t index = 0; index < *count; ++index) {
      auto value = ReadString(field);
      if (!value) {
        return false;
      }
      output.push_back(std::move(*value));
    }
    return true;
  }

  bool ReadTrailerHash() {
    // 先使用已读取的字节计算预期的 SHA256 哈希
    const std::string expected = ::torture::sha256Hex(
        std::string_view{accumulated_.data(), accumulated_.size()});
    // 读取尾部哈希（不追加到 accumulated_）
    std::array<char, kSha256HexLength> actual = {};
    if (size_t(actual.size()) > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()) ||
        !static_cast<bool>(input_.read(actual.data(), static_cast<std::streamsize>(actual.size())))) {
      return Fail(bytecode_diagnostic::kTruncated, "bytecode ended while reading trailer hash");
    }
    if (std::string_view{actual.data(), actual.size()} != expected) {
      return Fail(bytecode_diagnostic::kHashMismatch, "bytecode trailer hash mismatch");
    }
    return true;
  }

  bool Fail(std::string_view code, std::string message) {
    if (!failed_) {
      diagnostics_.error(
          std::string{bytecode_format::kDiagnosticDomain},
          std::string{code},
          SourceLocation{name_, kDiagnosticLine, kDiagnosticColumn},
          std::move(message));
      failed_ = true;
    }
    return false;
  }

 private:
  bool ReadRaw(char* data, std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
      return false;
    }
    if (!static_cast<bool>(input_.read(data, static_cast<std::streamsize>(size)))) {
      return false;
    }
    accumulated_.append(data, size);
    return true;
  }

  std::istream& input_;
  std::string name_;
  Diagnostics& diagnostics_;
  bool failed_ = false;
  std::string accumulated_;
};

void WriteInstruction(BytecodeWriter& writer, const Instruction& instruction) {
  // instruction.op 现在是 Opcode enum，直接写 2 字节
  writer.WriteBigEndian(static_cast<std::uint16_t>(instruction.op));
  writer.WriteStringVector(instruction.args);
}

void WriteFunction(BytecodeWriter& writer, const FunctionBytecode& function) {
  writer.WriteString(function.name);
  writer.WriteBigEndian(CheckedNonNegative(function.maxStackSize, kMaxStackLabel));
  writer.WriteBigEndian(CheckedNonNegative(function.securityLevel, kSecurityLevelLabel));
  writer.WriteBigEndian(function.returnable ? bytecode_format::kBooleanTrue : bytecode_format::kBooleanFalse);
  writer.WriteString(function.category);
  writer.WriteString(function.hash);
  writer.WriteString(function.grantor);
  writer.WriteStringVector(function.params);
  writer.WriteStringVector(function.allowedRoles);
  writer.WriteStringVector(function.callableFrom);
  writer.WriteStringVector(function.locals);
  writer.WriteBigEndian(CheckedCount(function.code.size(), kInstructionVectorLabel));
  for (const auto& instruction : function.code) {
    WriteInstruction(writer, instruction);
  }
}

std::optional<Instruction> ReadInstruction(BytecodeReader& reader) {
  const auto raw_opcode = reader.ReadBigEndian<std::uint16_t>(field_name::kOpcode);
  if (!raw_opcode) {
    return std::nullopt;
  }
  if (platform_bytecode::opcodeNameForOpcodeId(*raw_opcode).empty()) {
    reader.Fail(bytecode_diagnostic::kUnknownOpcode,
                "bytecode contains unknown opcode id " + std::to_string(*raw_opcode) +
                    " for platform '" + std::string{kPlatformName} + "'");
    return std::nullopt;
  }
  Instruction instruction;
  // 直接用 raw id 转 Opcode，省去 string 来回转换
  instruction.op = static_cast<Opcode>(*raw_opcode);
  if (!reader.ReadStringVector(instruction.args, field_name::kInstructionArgs)) {
    return std::nullopt;
  }
  return instruction;
}

std::optional<FunctionBytecode> ReadFunction(BytecodeReader& reader) {
  FunctionBytecode function;
  auto name = reader.ReadString(field_name::kFunctionName);
  auto max_stack_size = reader.ReadBigEndian<std::uint32_t>(field_name::kFunctionMaxStack);
  auto security_level = reader.ReadBigEndian<std::uint32_t>(field_name::kFunctionSecurityLevel);
  auto returnable = reader.ReadBigEndian<std::uint32_t>(field_name::kFunctionReturnable);
  auto category = reader.ReadString(field_name::kFunctionCategory);
  auto hash = reader.ReadString(field_name::kFunctionHash);
  auto grantor = reader.ReadString(field_name::kFunctionGrantor);
  if (!name || !max_stack_size || !security_level || !returnable || !category || !hash || !grantor) {
    return std::nullopt;
  }
  if (*max_stack_size > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
      *security_level > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
    reader.Fail(bytecode_diagnostic::kFunctionLimitOverflow, "function numeric limits exceed this VM build");
    return std::nullopt;
  }
  if (*returnable != bytecode_format::kBooleanFalse && *returnable != bytecode_format::kBooleanTrue) {
    reader.Fail(bytecode_diagnostic::kBadBoolean, "function returnable flag must be 0 or 1");
    return std::nullopt;
  }

  function.name = std::move(*name);
  function.maxStackSize = static_cast<int>(*max_stack_size);
  function.securityLevel = static_cast<int>(*security_level);
  function.returnable = *returnable == bytecode_format::kBooleanTrue;
  function.category = std::move(*category);
  function.hash = std::move(*hash);
  function.grantor = std::move(*grantor);

  if (!reader.ReadStringVector(function.params, field_name::kFunctionParams) ||
      !reader.ReadStringVector(function.allowedRoles, field_name::kFunctionRoles) ||
      !reader.ReadStringVector(function.callableFrom, field_name::kFunctionCallableFrom) ||
      !reader.ReadStringVector(function.locals, field_name::kFunctionLocals)) {
    return std::nullopt;
  }

  const auto instruction_count = reader.ReadBigEndian<std::uint32_t>(field_name::kInstructionCount);
  if (!instruction_count) {
    return std::nullopt;
  }
  if (*instruction_count > bytecode_format::kMaxCollectionItems) {
    reader.Fail(bytecode_diagnostic::kCollectionTooLarge, "function instruction vector exceeds bytecode limit");
    return std::nullopt;
  }
  function.code.reserve(*instruction_count);
  for (std::uint32_t index = 0; index < *instruction_count; ++index) {
    auto instruction = ReadInstruction(reader);
    if (!instruction) {
      return std::nullopt;
    }
    function.code.push_back(std::move(*instruction));
  }
  if (!function.hash.empty() && functionFingerprint(function) != function.hash) {
    reader.Fail(bytecode_diagnostic::kHashMismatch, "function '" + function.name + "' failed bytecode identity verification");
    return std::nullopt;
  }
  return function;
}

void AddBytecodeError(Diagnostics& diagnostics, std::string_view code, const std::string& name, std::string message) {
  diagnostics.error(
      std::string{bytecode_format::kDiagnosticDomain},
      std::string{code},
      SourceLocation{name, kDiagnosticLine, kDiagnosticColumn},
      std::move(message));
}

}  // namespace

void writeBytecode(std::ostream& output, const BytecodeProgram& program) {
  BytecodeWriter writer{output};
  // [0..3]   4 字节魔数
  writer.WriteRawBytes(platform_bytecode::kMagicBytes.data(),
                       platform_bytecode::kMagicBytes.size());
  // [4]      1 字节字节序标签
  writer.WriteRawByte(static_cast<std::uint8_t>(kPlatformByteOrderLabel));
  // [5..6]   2 字节 flags (big-endian)
  writer.WriteBigEndian(platform_bytecode::kFlags);
  // [7..8]   2 字节 version (big-endian)
  writer.WriteBigEndian(platform_bytecode::kVersion);
  // [9]      1 字节 platform id
  writer.WriteRawByte(platform_bytecode::kPlatformId);
  // [10..]   env_fingerprint -> entry -> producer -> functions
  // 字段顺序与现有 bytecode_binary.cc 和 linux_x64 都不同
  writer.WriteString(program.environmentFingerprint.empty() ? currentEnvironmentFingerprint() : program.environmentFingerprint);
  writer.WriteString(program.entry);
  writer.WriteString(program.producer.empty() ? environmentSummary() : program.producer);
  writer.WriteBigEndian(CheckedCount(program.functions.size(), kFunctionVectorLabel));
  for (const auto& function : program.functions) {
    WriteFunction(writer, function);
  }
  // 尾部 SHA256 十六进制校验（64 字符）
  writer.FlushWithTrailer();
}

std::optional<BytecodeProgram> readBytecode(std::istream& input, const std::string& name, Diagnostics& diagnostics) {
  BytecodeReader reader{input, name, diagnostics};
  if (!reader.ReadMagic()) {
    return std::nullopt;
  }

  // 字段顺序与现有 bytecode_binary.cc 和 linux_x64 都不同
  const auto byte_order = reader.ReadByte(field_name::kByteOrderLabel);
  if (!byte_order) {
    return std::nullopt;
  }
  if (static_cast<char>(*byte_order) != kPlatformByteOrderLabel) {
    AddBytecodeError(
        diagnostics,
        bytecode_diagnostic::kEndianMismatch,
        name,
        "bytecode byte order label '" + std::string(1, static_cast<char>(*byte_order)) +
            "' does not match current platform '" + std::string(1, kPlatformByteOrderLabel) + "'");
    return std::nullopt;
  }

  const auto flags = reader.ReadBigEndian<std::uint16_t>(field_name::kFlags);
  const auto version = reader.ReadBigEndian<std::uint16_t>(field_name::kVersion);
  if (!version || !flags) {
    return std::nullopt;
  }
  if (*version != platform_bytecode::kVersion) {
    AddBytecodeError(
        diagnostics,
        bytecode_diagnostic::kUnsupportedVersion,
        name,
        "bytecode format version " + std::to_string(*version) + " is not supported by this VM");
    return std::nullopt;
  }
  if (*flags != platform_bytecode::kFlags) {
    AddBytecodeError(diagnostics, bytecode_diagnostic::kUnsupportedFlags, name, "bytecode uses unsupported flags");
    return std::nullopt;
  }

  const auto platform_id = reader.ReadByte(field_name::kPlatformId);
  if (!platform_id) {
    return std::nullopt;
  }
  if (*platform_id != platform_bytecode::kPlatformId) {
    AddBytecodeError(
        diagnostics,
        bytecode_diagnostic::kPlatformMismatch,
        name,
        "bytecode platform id " + std::to_string(*platform_id) +
            " does not match current platform " + std::to_string(platform_bytecode::kPlatformId) +
            " ('" + std::string{kPlatformName} + "')");
    return std::nullopt;
  }

  // 字段顺序：env_fingerprint -> entry -> producer
  auto fingerprint = reader.ReadString(field_name::kEnvironmentFingerprint);
  auto entry = reader.ReadString(field_name::kEntry);
  auto producer = reader.ReadString(field_name::kProducer);
  const auto function_count = reader.ReadBigEndian<std::uint32_t>(field_name::kFunctionCount);
  if (!fingerprint || !producer || !entry || !function_count) {
    return std::nullopt;
  }
  if (!matchesCurrentEnvironment(*fingerprint)) {
    AddBytecodeError(
        diagnostics,
        bytecode_diagnostic::kEnvironmentMismatch,
        name,
        "bytecode environment " + shortFingerprint(*fingerprint) +
            " does not match current compiler environment " + shortFingerprint(currentEnvironmentFingerprint()) +
            " (platform '" + std::string{kPlatformName} + "')");
    return std::nullopt;
  }
  if (*function_count > bytecode_format::kMaxCollectionItems) {
    AddBytecodeError(diagnostics, bytecode_diagnostic::kCollectionTooLarge, name, "function vector exceeds bytecode limit");
    return std::nullopt;
  }

  BytecodeProgram program;
  program.platformId = *platform_id;
  program.environmentFingerprint = std::move(*fingerprint);
  program.producer = std::move(*producer);
  program.entry = std::move(*entry);
  program.functions.reserve(*function_count);
  for (std::uint32_t index = 0; index < *function_count; ++index) {
    auto function = ReadFunction(reader);
    if (!function) {
      return std::nullopt;
    }
    program.functions.push_back(std::move(*function));
  }

  // 读取并验证尾部 SHA256 校验
  if (!reader.ReadTrailerHash()) {
    return std::nullopt;
  }

  return program;
}

}  // namespace torture::vm
