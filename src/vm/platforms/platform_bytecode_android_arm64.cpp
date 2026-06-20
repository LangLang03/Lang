#include "vm/platform_bytecode.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace torture::vm::platform_bytecode {
namespace {

// android_arm64 平台 66 opcode 查表。
struct OpcodeEntry {
    std::uint16_t id;
    std::string_view name;
};

#define TORTURE_OPCODES(X) \
    X(kLocalOpcodeVerify,            "VERIFY") \
    X(kLocalOpcodePushInteger,       "PUSHI") \
    X(kLocalOpcodePushString,        "PUSHS") \
    X(kLocalOpcodePushNull,          "PUSHNULL") \
    X(kLocalOpcodePushReference,     "PUSHREF") \
    X(kLocalOpcodeDereference,       "DEREF") \
    X(kLocalOpcodeFieldReference,    "FIELDREF") \
    X(kLocalOpcodeLoad,              "LOAD") \
    X(kLocalOpcodeStore,             "STORE") \
    X(kLocalOpcodeStoreDereference,  "STORE_DEREF") \
    X(kLocalOpcodeStorePointer,      "STOREPTR") \
    X(kLocalOpcodePop,               "POP") \
    X(kLocalOpcodeAdd,               "ADD") \
    X(kLocalOpcodeAddTrap,           "ADD_trap") \
    X(kLocalOpcodeAddWrap,           "ADD_wrap") \
    X(kLocalOpcodeAddSaturate,       "ADD_saturate") \
    X(kLocalOpcodeAddExtend,         "ADD_extend") \
    X(kLocalOpcodeSub,               "SUB") \
    X(kLocalOpcodeSubTrap,           "SUB_trap") \
    X(kLocalOpcodeSubWrap,           "SUB_wrap") \
    X(kLocalOpcodeSubSaturate,       "SUB_saturate") \
    X(kLocalOpcodeSubExtend,         "SUB_extend") \
    X(kLocalOpcodeMul,               "MUL") \
    X(kLocalOpcodeMulTrap,           "MUL_trap") \
    X(kLocalOpcodeMulWrap,           "MUL_wrap") \
    X(kLocalOpcodeMulSaturate,       "MUL_saturate") \
    X(kLocalOpcodeMulExtend,         "MUL_extend") \
    X(kLocalOpcodeDiv,               "DIV") \
    X(kLocalOpcodeDivTrap,           "DIV_trap") \
    X(kLocalOpcodeDivWrap,           "DIV_wrap") \
    X(kLocalOpcodeDivSaturate,       "DIV_saturate") \
    X(kLocalOpcodeDivExtend,         "DIV_extend") \
    X(kLocalOpcodeEqual,             "EQ") \
    X(kLocalOpcodeNotEqual,          "NE") \
    X(kLocalOpcodeLess,              "LT") \
    X(kLocalOpcodeLessEqual,          "LE") \
    X(kLocalOpcodeGreater,           "GT") \
    X(kLocalOpcodeGreaterEqual,      "GE") \
    X(kLocalOpcodeAnd,               "AND") \
    X(kLocalOpcodeOr,                "OR") \
    X(kLocalOpcodeXor,               "XOR") \
    X(kLocalOpcodeNand,              "NAND") \
    X(kLocalOpcodeNor,               "NOR") \
    X(kLocalOpcodeNot,               "NOT") \
    X(kLocalOpcodeJump,              "JMP") \
    X(kLocalOpcodeJumpIfZero,        "JZ") \
    X(kLocalOpcodeCheckApproval,     "CHECKAPPROVAL") \
    X(kLocalOpcodeCheckLimits,       "CHECKLIMITS") \
    X(kLocalOpcodeCheckRole,         "CHECKROLE") \
    X(kLocalOpcodeCheckRoleIndirect, "CHECKROLEIND") \
    X(kLocalOpcodeNullCheck,         "NULLCHECK") \
    X(kLocalOpcodeAssert,            "ASSERT") \
    X(kLocalOpcodeCall,              "CALL") \
    X(kLocalOpcodeCallIndirect,      "CALLIND") \
    X(kLocalOpcodeOutputInt,         "OUTPUTINT") \
    X(kLocalOpcodeOutputChar,        "OUTPUTCHAR") \
    X(kLocalOpcodePrintLine,         "PRINTLN") \
    X(kLocalOpcodeInputInt,          "INPUTINT") \
    X(kLocalOpcodeInputChar,         "INPUTCHAR") \
    X(kLocalOpcodeVerifyEcc,         "VERIFYECC") \
    X(kLocalOpcodeCopyMemory,        "COPYMEMORY") \
    X(kLocalOpcodeCompareMemory,     "COMPAREMEMORY") \
    X(kLocalOpcodeRole,              "ROLE") \
    X(kLocalOpcodeOperatorInput,     "OPINPUT") \
    X(kLocalOpcodeReturn,            "RET") \
    X(kLocalOpcodeHalt,              "HALT")

#define TORTURE_MK_ENTRY(local_id, base_name) \
    { static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | (local_id)), \
      base_name "_" TORTURE_PLATFORM_SUFFIX_LITERAL },

constexpr std::array<OpcodeEntry, 66> kPlatformTable = {{
    TORTURE_OPCODES(TORTURE_MK_ENTRY)
}};

#undef TORTURE_MK_ENTRY
#undef TORTURE_OPCODES

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
    // 1) 优先匹配带后缀的新名（如 "ADD_AA64"）
    for (const auto& entry : kPlatformTable) {
        if (entry.name == name) {
            return entry.id;
        }
    }
    // 2) 否则尝试把旧名（不带后缀）映射到当前平台（"ADD" -> "ADD_AA64"）
    for (const auto& entry : kPlatformTable) {
        const auto sep = entry.name.find('_');
        if (sep != std::string_view::npos && entry.name.substr(0, sep) == name) {
            return entry.id;
        }
    }
    return std::nullopt;
}

}  // namespace torture::vm::platform_bytecode
