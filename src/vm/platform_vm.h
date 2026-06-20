#pragma once

#include "vm/opcode.h"

#include <cstdint>
#include <unordered_map>

namespace torture::vm::platform_vm {

// 返回当前平台 opcode 整数值 -> 人类可读"动作描述" 的调度表。
// 由于 opcode 整数值已经平台特化（见 vm/platform_bytecode.h），同一份
// platform_vm.cpp 在 12 平台各自编译出不同内容；其他平台对应的 key 不存在。
const std::unordered_map<std::uint16_t, const char*>& dispatchTable();

// 检查 id 是否属于本平台合法 opcode 范围（高字节 == TORTURE_PLATFORM_ID，
// 低字节落在当前平台特化的本地 opcode 范围内，见 platform_bytecode_<plat>.h）。
bool isOwnPlatformOpcode(std::uint16_t id);

}  // namespace torture::vm::platform_vm
