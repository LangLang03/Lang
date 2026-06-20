// VM 值类型辅助函数实现

#include "vm/value.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "vm/bytecode_format.h"
#include "vm/platform_bytecode.h"

namespace torture::vm {
namespace {

using Int = __int128_t;

constexpr char kDigitZero = '0';
constexpr int kDecimalBase = 10;
constexpr int kHexAlphaOffset = 10;
constexpr int kHexHighNibbleShift = 4;
constexpr std::size_t kHexEncodedByteWidth = 2;
constexpr std::string_view kIntegerZeroText = "0";
constexpr std::string_view kInstructionTrue = "1";

} // namespace

Value integerValue(__int128_t value) {
    return Value{ValueKind::Integer, value, {}};
}

Value stringValue(std::string value) {
    return Value{ValueKind::String, 0, std::move(value)};
}

Value nullValue() {
    return Value{ValueKind::Null, 0, {}};
}

Value refValue(std::string name) {
    return Value{ValueKind::Ref, 0, std::move(name)};
}

bool equalsSymbol(const std::string& value, std::string_view symbol) {
    return std::string_view{value} == symbol;
}

__int128_t asInteger(const Value& value) {
    if (value.kind == ValueKind::Integer) {
        return value.integer;
    }
    if (value.kind == ValueKind::Null) {
        return 0;
    }
    if (value.kind == ValueKind::Ref) {
        throw std::runtime_error("cannot use reference as integer without dereference");
    }
    return parseInteger(value.string);
}

bool asBool(const Value& value) {
    if (value.kind == ValueKind::String) {
        return !value.string.empty();
    }
    return asInteger(value) != 0;
}

bool equalsValue(const Value& left, const Value& right) {
    if (left.kind == ValueKind::String || right.kind == ValueKind::String || left.kind == ValueKind::Ref || right.kind == ValueKind::Ref) {
        return left.string == right.string;
    }
    return asInteger(left) == asInteger(right);
}

__int128_t parseInteger(const std::string& text) {
    Int value = 0;
    bool negative = false;
    std::size_t index = 0;
    if (!text.empty() && text.front() == '-') {
        negative = true;
        index = 1;
    }
    for (; index < text.size(); ++index) {
        value = value * kDecimalBase + static_cast<Int>(text[index] - kDigitZero);
    }
    if (negative) {
        value = -value;
    }
    return value;
}

std::string intToString(__int128_t value) {
    if (value == 0) {
        return std::string{kIntegerZeroText};
    }
    bool negative = value < 0;
    if (negative) {
        value = -value;
    }
    std::string out;
    while (value > 0) {
        out.push_back(static_cast<char>(kDigitZero + value % kDecimalBase));
        value /= kDecimalBase;
    }
    if (negative) {
        out.push_back('-');
    }
    std::reverse(out.begin(), out.end());
    return out;
}

std::string valueToString(const Value& value) {
    if (value.kind == ValueKind::String) {
        return value.string;
    }
    if (value.kind == ValueKind::Null) {
        return std::string{source_literal::kNull};
    }
    if (value.kind == ValueKind::Ref) {
        return value.string;
    }
    return intToString(value.integer);
}

std::size_t toSize(__int128_t value) {
    if (value < 0) {
        throw std::runtime_error("negative bytecode jump target");
    }
    return static_cast<std::size_t>(value);
}

__int128_t int64Min() {
    return static_cast<Int>(std::numeric_limits<std::int64_t>::min());
}

__int128_t int64Max() {
    return static_cast<Int>(std::numeric_limits<std::int64_t>::max());
}

__int128_t clamp64(__int128_t value) {
    if (value < int64Min()) {
        return int64Min();
    }
    if (value > int64Max()) {
        return int64Max();
    }
    return value;
}

__int128_t wrap64(__int128_t value) {
    const auto signed_bits = std::numeric_limits<std::int64_t>::digits;
    const auto unsigned_bits = std::numeric_limits<std::uint64_t>::digits;
    const Int unsigned_range = static_cast<Int>(1) << unsigned_bits;
    const Int sign_bit = static_cast<Int>(1) << signed_bits;
    Int wrapped = value % unsigned_range;
    if (wrapped < 0) {
        wrapped += unsigned_range;
    }
    if (wrapped >= sign_bit) {
        wrapped -= unsigned_range;
    }
    return wrapped;
}

__int128_t applyOverflowPolicy(__int128_t value, const std::string& policy) {
    if (equalsSymbol(policy, overflow_policy::kWrap)) {
        return wrap64(value);
    }
    if (equalsSymbol(policy, overflow_policy::kSaturate)) {
        return clamp64(value);
    }
    if (equalsSymbol(policy, overflow_policy::kTrap) && (value < int64Min() || value > int64Max())) {
        throw std::runtime_error("integer overflow");
    }
    return value;
}

std::pair<std::string, std::string> splitOpPolicy(const std::string& op) {
    // 输入示例："ADD_LX64"、"ADD_trap_LX64"、"ADD_wrap_LX64"。
    // 目标：baseOp = "ADD_LX64"（保留平台后缀），policy = "" 或 "trap"/"wrap"/...
    // 做法：剥掉末尾平台后缀（"_LX64"），按 '_' 拆分；再把平台后缀拼回 base。
    constexpr std::string_view kPlatformSuffix = torture::vm::platform_bytecode::kPlatformSuffix;
    std::string suffix{std::string{kPlatformSuffix.size(), '_'}.append(std::string{kPlatformSuffix})};
    std::string stripped = op;
    std::string platformTail;
    if (stripped.size() > suffix.size() &&
        stripped.compare(stripped.size() - kPlatformSuffix.size(), kPlatformSuffix.size(), kPlatformSuffix) == 0 &&
        stripped[stripped.size() - kPlatformSuffix.size() - 1] == '_') {
        platformTail.assign(kPlatformSuffix);
        stripped.erase(stripped.size() - kPlatformSuffix.size() - 1);
    }
    const auto pos = stripped.find('_');
    if (pos == std::string::npos) {
        if (platformTail.empty()) {
            return {stripped, ""};
        }
        return {stripped + "_" + platformTail, ""};
    }
    std::string base = stripped.substr(0, pos) + (platformTail.empty() ? "" : "_" + platformTail);
    std::string policy = stripped.substr(pos + 1);
    return {base, policy};
}

int hexDigit(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return kHexAlphaOffset + ch - 'a';
    }
    if (ch >= 'A' && ch <= 'F') {
        return kHexAlphaOffset + ch - 'A';
    }
    throw std::runtime_error("bad hex string in bytecode");
}

std::string hexDecode(const std::string& text) {
    if (text.size() % kHexEncodedByteWidth != 0) {
        throw std::runtime_error("bad hex string length in bytecode");
    }
    std::string out;
    out.reserve(text.size() / kHexEncodedByteWidth);
    for (std::size_t i = 0; i < text.size(); i += kHexEncodedByteWidth) {
        out.push_back(static_cast<char>((hexDigit(text[i]) << kHexHighNibbleShift) | hexDigit(text[i + 1])));
    }
    return out;
}

void pushBool(std::vector<Value>& stack, bool value) {
    stack.push_back(integerValue(value ? 1 : 0));
}

Value pop(std::vector<Value>& stack) {
    if (stack.empty()) {
        throw std::runtime_error("stack underflow");
    }
    auto value = stack.back();
    stack.pop_back();
    return value;
}

} // namespace torture::vm
