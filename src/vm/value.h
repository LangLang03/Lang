// VM 值类型定义头文件

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "vm/language_symbols.h"

namespace torture::vm {

enum class ValueKind {
    Null,
    Integer,
    String,
    Ref,
};

struct Value {
    ValueKind kind = ValueKind::Integer;
    __int128_t integer = 0;
    std::string string;
};

Value integerValue(__int128_t value);
Value stringValue(std::string value);
Value nullValue();
Value refValue(std::string name);
bool equalsSymbol(const std::string& value, std::string_view symbol);
__int128_t asInteger(const Value& value);
bool asBool(const Value& value);
bool equalsValue(const Value& left, const Value& right);
__int128_t parseInteger(const std::string& text);
std::string intToString(__int128_t value);
std::string valueToString(const Value& value);
std::size_t toSize(__int128_t value);
__int128_t int64Min();
__int128_t int64Max();
__int128_t clamp64(__int128_t value);
__int128_t wrap64(__int128_t value);
__int128_t applyOverflowPolicy(__int128_t value, const std::string& policy);
std::pair<std::string, std::string> splitOpPolicy(const std::string& op);
int hexDigit(char ch);
std::string hexDecode(const std::string& text);
void pushBool(std::vector<Value>& stack, bool value);
Value pop(std::vector<Value>& stack);

} // namespace torture::vm
