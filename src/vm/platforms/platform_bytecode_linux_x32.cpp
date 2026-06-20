#include "vm/platform_bytecode.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace torture::vm::platform_bytecode {
namespace {

// linux_x32 平台 66 opcode 查表。直接使用平台头文件中的 kOpcode* 常量。
struct OpcodeEntry {
    std::uint16_t id;
    std::string_view name;
};

constexpr std::array<OpcodeEntry, 66> kPlatformTable = {{
    {kOpcodeVerify,           "VERIFY_LX32"},
    {kOpcodePushInteger,      "PUSHI_LX32"},
    {kOpcodePushString,       "PUSHS_LX32"},
    {kOpcodePushNull,         "PUSHNULL_LX32"},
    {kOpcodePushReference,    "PUSHREF_LX32"},
    {kOpcodeDereference,      "DEREF_LX32"},
    {kOpcodeFieldReference,   "FIELDREF_LX32"},
    {kOpcodeLoad,             "LOAD_LX32"},
    {kOpcodeStore,            "STORE_LX32"},
    {kOpcodeStoreDereference, "STORE_DEREF_LX32"},
    {kOpcodeStorePointer,     "STOREPTR_LX32"},
    {kOpcodePop,              "POP_LX32"},
    {kOpcodeAdd,              "ADD_LX32"},
    {kOpcodeAddTrap,          "ADD_trap_LX32"},
    {kOpcodeAddWrap,          "ADD_wrap_LX32"},
    {kOpcodeAddSaturate,      "ADD_saturate_LX32"},
    {kOpcodeAddExtend,        "ADD_extend_LX32"},
    {kOpcodeSub,              "SUB_LX32"},
    {kOpcodeSubTrap,          "SUB_trap_LX32"},
    {kOpcodeSubWrap,          "SUB_wrap_LX32"},
    {kOpcodeSubSaturate,      "SUB_saturate_LX32"},
    {kOpcodeSubExtend,        "SUB_extend_LX32"},
    {kOpcodeMul,              "MUL_LX32"},
    {kOpcodeMulTrap,          "MUL_trap_LX32"},
    {kOpcodeMulWrap,          "MUL_wrap_LX32"},
    {kOpcodeMulSaturate,      "MUL_saturate_LX32"},
    {kOpcodeMulExtend,        "MUL_extend_LX32"},
    {kOpcodeDiv,              "DIV_LX32"},
    {kOpcodeDivTrap,          "DIV_trap_LX32"},
    {kOpcodeDivWrap,          "DIV_wrap_LX32"},
    {kOpcodeDivSaturate,      "DIV_saturate_LX32"},
    {kOpcodeDivExtend,        "DIV_extend_LX32"},
    {kOpcodeEqual,            "EQ_LX32"},
    {kOpcodeNotEqual,         "NE_LX32"},
    {kOpcodeLess,             "LT_LX32"},
    {kOpcodeLessEqual,        "LE_LX32"},
    {kOpcodeGreater,          "GT_LX32"},
    {kOpcodeGreaterEqual,     "GE_LX32"},
    {kOpcodeAnd,              "AND_LX32"},
    {kOpcodeOr,               "OR_LX32"},
    {kOpcodeXor,              "XOR_LX32"},
    {kOpcodeNand,             "NAND_LX32"},
    {kOpcodeNor,              "NOR_LX32"},
    {kOpcodeNot,              "NOT_LX32"},
    {kOpcodeJump,             "JMP_LX32"},
    {kOpcodeJumpIfZero,       "JZ_LX32"},
    {kOpcodeCheckApproval,    "CHECKAPPROVAL_LX32"},
    {kOpcodeCheckLimits,      "CHECKLIMITS_LX32"},
    {kOpcodeCheckRole,        "CHECKROLE_LX32"},
    {kOpcodeCheckRoleIndirect,"CHECKROLEIND_LX32"},
    {kOpcodeNullCheck,        "NULLCHECK_LX32"},
    {kOpcodeAssert,           "ASSERT_LX32"},
    {kOpcodeCall,             "CALL_LX32"},
    {kOpcodeCallIndirect,     "CALLIND_LX32"},
    {kOpcodeOutputInt,        "OUTPUTINT_LX32"},
    {kOpcodeOutputChar,       "OUTPUTCHAR_LX32"},
    {kOpcodePrintLine,        "PRINTLN_LX32"},
    {kOpcodeInputInt,         "INPUTINT_LX32"},
    {kOpcodeInputChar,        "INPUTCHAR_LX32"},
    {kOpcodeVerifyEcc,        "VERIFYECC_LX32"},
    {kOpcodeCopyMemory,       "COPYMEMORY_LX32"},
    {kOpcodeCompareMemory,    "COMPAREMEMORY_LX32"},
    {kOpcodeRole,             "ROLE_LX32"},
    {kOpcodeOperatorInput,    "OPINPUT_LX32"},
    {kOpcodeReturn,           "RET_LX32"},
    {kOpcodeHalt,             "HALT_LX32"},
}};

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
    // 1) 优先匹配带后缀的新名（如 "ADD_LX32"）
    for (const auto& entry : kPlatformTable) {
        if (entry.name == name) {
            return entry.id;
        }
    }
    // 2) 否则尝试把旧名（不带后缀）映射到当前平台（"ADD" -> "ADD_LX32"）
    for (const auto& entry : kPlatformTable) {
        const auto sep = entry.name.find('_');
        if (sep != std::string_view::npos && entry.name.substr(0, sep) == name) {
            return entry.id;
        }
    }
    return std::nullopt;
}

}  // namespace torture::vm::platform_bytecode
