#include "vm/platform_bytecode.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace torture::vm::platform_bytecode {
namespace {

// linux_arm32 平台查表：66 个 opcode 条目。
// local_id 使用 0x85-0xC6 范围的值，与 linux_arm64 完全不同。
struct OpcodeEntry {
    std::uint16_t id;
    std::string_view name;
};

// (local_id, base_name) 列表。base_name 是与平台无关的"大写名"，
// 它会被拼接上当前平台后缀形成最终名字（如 "ADD" -> "ADD_LA32"）。
#define TORTURE_OPCODES(X) \
    X(0x85, "VERIFY") \
    X(0x86, "PUSHI") \
    X(0x87, "PUSHS") \
    X(0x88, "PUSHNULL") \
    X(0x89, "PUSHREF") \
    X(0x8A, "DEREF") \
    X(0x8B, "FIELDREF") \
    X(0x8C, "LOAD") \
    X(0x8D, "STORE") \
    X(0x8E, "STORE_DEREF") \
    X(0x8F, "STOREPTR") \
    X(0x90, "POP") \
    X(0x91, "ADD") \
    X(0x92, "ADD_trap") \
    X(0x93, "ADD_wrap") \
    X(0x94, "ADD_saturate") \
    X(0x95, "ADD_extend") \
    X(0x96, "SUB") \
    X(0x97, "SUB_trap") \
    X(0x98, "SUB_wrap") \
    X(0x99, "SUB_saturate") \
    X(0x9A, "SUB_extend") \
    X(0x9B, "MUL") \
    X(0x9C, "MUL_trap") \
    X(0x9D, "MUL_wrap") \
    X(0x9E, "MUL_saturate") \
    X(0x9F, "MUL_extend") \
    X(0xA0, "DIV") \
    X(0xA1, "DIV_trap") \
    X(0xA2, "DIV_wrap") \
    X(0xA3, "DIV_saturate") \
    X(0xA4, "DIV_extend") \
    X(0xA5, "EQ") \
    X(0xA6, "NE") \
    X(0xA7, "LT") \
    X(0xA8, "LE") \
    X(0xA9, "GT") \
    X(0xAA, "GE") \
    X(0xAB, "AND") \
    X(0xAC, "OR") \
    X(0xAD, "XOR") \
    X(0xAE, "NAND") \
    X(0xAF, "NOR") \
    X(0xB0, "NOT") \
    X(0xB1, "JMP") \
    X(0xB2, "JZ") \
    X(0xB3, "CHECKAPPROVAL") \
    X(0xB4, "CHECKLIMITS") \
    X(0xB5, "CHECKROLE") \
    X(0xB6, "CHECKROLEIND") \
    X(0xB7, "NULLCHECK") \
    X(0xB8, "ASSERT") \
    X(0xB9, "CALL") \
    X(0xBA, "CALLIND") \
    X(0xBB, "OUTPUTINT") \
    X(0xBC, "OUTPUTCHAR") \
    X(0xBD, "PRINTLN") \
    X(0xBE, "INPUTINT") \
    X(0xBF, "INPUTCHAR") \
    X(0xC0, "VERIFYECC") \
    X(0xC1, "COPYMEMORY") \
    X(0xC2, "COMPAREMEMORY") \
    X(0xC3, "ROLE") \
    X(0xC4, "OPINPUT") \
    X(0xC5, "RET") \
    X(0xC6, "HALT")

#define TORTURE_MK_ENTRY(local_id, base_name) \
    { static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | (local_id)), \
      base_name "_" TORTURE_PLATFORM_SUFFIX_LITERAL },

constexpr std::array<OpcodeEntry, 66> kPlatformTable = {{
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
    // 1) 优先匹配带后缀的新名（如 "ADD_LA32"）
    for (const auto& entry : kPlatformTable) {
        if (entry.name == name) {
            return entry.id;
        }
    }
    // 2) 否则尝试把旧名（不带后缀）映射到当前平台（"ADD" -> "ADD_LA32"）
    for (const auto& entry : kPlatformTable) {
        const auto sep = entry.name.find('_');
        if (sep != std::string_view::npos && entry.name.substr(0, sep) == name) {
            return entry.id;
        }
    }
    return std::nullopt;
}

}  // namespace torture::vm::platform_bytecode
