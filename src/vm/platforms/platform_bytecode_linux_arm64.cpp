#include "vm/platform_bytecode.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace torture::vm::platform_bytecode {
namespace {

// linux_arm64 平台查表：66 个 opcode 条目。
// local_id 使用 0xC7-0xFF 环绕到 0x01-0x09 的值，与 linux_arm32 完全不同。
struct OpcodeEntry {
    std::uint16_t id;
    std::string_view name;
};

// (local_id, base_name) 列表。base_name 是与平台无关的"大写名"，
// 它会被拼接上当前平台后缀形成最终名字（如 "ADD" -> "ADD_LA64"）。
#define TORTURE_OPCODES(X) \
    X(0xC7, "VERIFY") \
    X(0xC8, "PUSHI") \
    X(0xC9, "PUSHS") \
    X(0xCA, "PUSHNULL") \
    X(0xCB, "PUSHREF") \
    X(0xCC, "DEREF") \
    X(0xCD, "FIELDREF") \
    X(0xCE, "LOAD") \
    X(0xCF, "STORE") \
    X(0xD0, "STORE_DEREF") \
    X(0xD1, "STOREPTR") \
    X(0xD2, "POP") \
    X(0xD3, "ADD") \
    X(0xD4, "ADD_trap") \
    X(0xD5, "ADD_wrap") \
    X(0xD6, "ADD_saturate") \
    X(0xD7, "ADD_extend") \
    X(0xD8, "SUB") \
    X(0xD9, "SUB_trap") \
    X(0xDA, "SUB_wrap") \
    X(0xDB, "SUB_saturate") \
    X(0xDC, "SUB_extend") \
    X(0xDD, "MUL") \
    X(0xDE, "MUL_trap") \
    X(0xDF, "MUL_wrap") \
    X(0xE0, "MUL_saturate") \
    X(0xE1, "MUL_extend") \
    X(0xE2, "DIV") \
    X(0xE3, "DIV_trap") \
    X(0xE4, "DIV_wrap") \
    X(0xE5, "DIV_saturate") \
    X(0xE6, "DIV_extend") \
    X(0xE7, "EQ") \
    X(0xE8, "NE") \
    X(0xE9, "LT") \
    X(0xEA, "LE") \
    X(0xEB, "GT") \
    X(0xEC, "GE") \
    X(0xED, "AND") \
    X(0xEE, "OR") \
    X(0xEF, "XOR") \
    X(0xF0, "NAND") \
    X(0xF1, "NOR") \
    X(0xF2, "NOT") \
    X(0xF3, "JMP") \
    X(0xF4, "JZ") \
    X(0xF5, "CHECKAPPROVAL") \
    X(0xF6, "CHECKLIMITS") \
    X(0xF7, "CHECKROLE") \
    X(0xF8, "CHECKROLEIND") \
    X(0xF9, "NULLCHECK") \
    X(0xFA, "ASSERT") \
    X(0xFB, "CALL") \
    X(0xFC, "CALLIND") \
    X(0xFD, "OUTPUTINT") \
    X(0xFE, "OUTPUTCHAR") \
    X(0xFF, "PRINTLN") \
    X(0x01, "INPUTINT") \
    X(0x02, "INPUTCHAR") \
    X(0x03, "VERIFYECC") \
    X(0x04, "COPYMEMORY") \
    X(0x05, "COMPAREMEMORY") \
    X(0x06, "ROLE") \
    X(0x07, "OPINPUT") \
    X(0x08, "RET") \
    X(0x09, "HALT")

#define TORTURE_MK_ENTRY(local_id, base_name) \
    { static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | (local_id)), \
      base_name "_" TORTURE_PLATFORM_SUFFIX_LITERAL },

constexpr std::array<OpcodeEntry, 67> kPlatformTable = {{
    TORTURE_OPCODES(TORTURE_MK_ENTRY)
}};

#undef TORTURE_MK_ENTRY

}  // namespace

std::string_view opcodeNameForOpcodeId(std::uint16_t id) {
    if (static_cast<std::uint8_t>(id >> 8) != TORTURE_PLATFORM_ID) {
        return {};
    }
    for (const auto& entry : kPlatformTable) {
        if (entry.id == id) {
            return entry.name;
        }
    }
    return {};
}

std::optional<std::uint16_t> opcodeIdFromName(std::string_view name) {
    // 1) 优先匹配带后缀的新名（如 "ADD_LA64"）
    for (const auto& entry : kPlatformTable) {
        if (entry.name == name) {
            return entry.id;
        }
    }
    // 2) 否则尝试把旧名（不带后缀）映射到当前平台（"ADD" -> "ADD_LA64"）
    for (const auto& entry : kPlatformTable) {
        const auto sep = entry.name.find('_');
        if (sep != std::string_view::npos && entry.name.substr(0, sep) == name) {
            return entry.id;
        }
    }
    return std::nullopt;
}

}  // namespace torture::vm::platform_bytecode
