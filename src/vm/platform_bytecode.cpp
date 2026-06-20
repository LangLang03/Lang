#include "vm/platform_bytecode.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace torture::vm::platform_bytecode {
namespace {

// 12 平台 × 66 opcode = 792 条目的查表。同一份源文件在不同平台编译时只展开一份。
struct OpcodeEntry {
    std::uint16_t id;
    std::string_view name;
};

// (local_id, base_name) 列表。base_name 是与平台无关的"大写名"，
// 它会被拼接上当前平台后缀形成最终名字（如 "ADD" -> "ADD_LX64"）。
#define TORTURE_OPCODES(X) \
    X(0x01, "VERIFY") \
    X(0x02, "PUSHI") \
    X(0x03, "PUSHS") \
    X(0x04, "PUSHNULL") \
    X(0x05, "PUSHREF") \
    X(0x06, "DEREF") \
    X(0x07, "FIELDREF") \
    X(0x08, "LOAD") \
    X(0x09, "STORE") \
    X(0x0A, "STORE_DEREF") \
    X(0x0B, "STOREPTR") \
    X(0x0C, "POP") \
    X(0x0D, "ADD") \
    X(0x0E, "ADD_trap") \
    X(0x0F, "ADD_wrap") \
    X(0x10, "ADD_saturate") \
    X(0x11, "ADD_extend") \
    X(0x12, "SUB") \
    X(0x13, "SUB_trap") \
    X(0x14, "SUB_wrap") \
    X(0x15, "SUB_saturate") \
    X(0x16, "SUB_extend") \
    X(0x17, "MUL") \
    X(0x18, "MUL_trap") \
    X(0x19, "MUL_wrap") \
    X(0x1A, "MUL_saturate") \
    X(0x1B, "MUL_extend") \
    X(0x1C, "DIV") \
    X(0x1D, "DIV_trap") \
    X(0x1E, "DIV_wrap") \
    X(0x1F, "DIV_saturate") \
    X(0x20, "DIV_extend") \
    X(0x21, "EQ") \
    X(0x22, "NE") \
    X(0x23, "LT") \
    X(0x24, "LE") \
    X(0x25, "GT") \
    X(0x26, "GE") \
    X(0x27, "AND") \
    X(0x28, "OR") \
    X(0x29, "XOR") \
    X(0x2A, "NAND") \
    X(0x2B, "NOR") \
    X(0x2C, "NOT") \
    X(0x2D, "JMP") \
    X(0x2E, "JZ") \
    X(0x2F, "CHECKAPPROVAL") \
    X(0x30, "CHECKLIMITS") \
    X(0x31, "CHECKROLE") \
    X(0x32, "CHECKROLEIND") \
    X(0x33, "NULLCHECK") \
    X(0x34, "ASSERT") \
    X(0x35, "CALL") \
    X(0x36, "CALLIND") \
    X(0x37, "OUTPUTINT") \
    X(0x38, "OUTPUTCHAR") \
    X(0x39, "PRINTLN") \
    X(0x3A, "INPUTINT") \
    X(0x3B, "INPUTCHAR") \
    X(0x3C, "VERIFYECC") \
    X(0x3D, "COPYMEMORY") \
    X(0x3E, "COMPAREMEMORY") \
    X(0x3F, "ROLE") \
    X(0x40, "OPINPUT") \
    X(0x41, "RET") \
    X(0x42, "HALT")

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
    // 1) 优先匹配带后缀的新名（如 "ADD_LX64"）
    for (const auto& entry : kPlatformTable) {
        if (entry.name == name) {
            return entry.id;
        }
    }
    // 2) 否则尝试把旧名（不带后缀）映射到当前平台（"ADD" -> "ADD_LX64"）
    for (const auto& entry : kPlatformTable) {
        const auto sep = entry.name.find('_');
        if (sep != std::string_view::npos && entry.name.substr(0, sep) == name) {
            return entry.id;
        }
    }
    return std::nullopt;
}

}  // namespace torture::vm::platform_bytecode
