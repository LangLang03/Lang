#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

#include "vm/platform.h"

// CMake 定义: TORTURE_PLATFORM_BYTECODE_HEADER="vm/platforms/platform_bytecode_linux_x64.h"
// 编译时只包含当前平台的头文件，不包含其他平台代码
#include TORTURE_PLATFORM_BYTECODE_HEADER

namespace torture::vm::platform_bytecode {

// 平台 ID 字段：写入字节流第 5 字节
inline constexpr std::uint8_t kPlatformId = TORTURE_PLATFORM_ID;

// 跨平台查找：把任意 uint16_t opcode id 翻译为带平台后缀的名字（仅当它属于本平台时）
std::string_view opcodeNameForOpcodeId(std::uint16_t id);

// 反向查找：接受带后缀的新名或旧别名（不带后缀视为当前平台）
std::optional<std::uint16_t> opcodeIdFromName(std::string_view name);

}  // namespace torture::vm::platform_bytecode
