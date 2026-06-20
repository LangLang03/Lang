#include "vm/platform_bytecode.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace torture::vm::platform_bytecode {
namespace {

// linux_x64 平台 66 opcode 查表。直接使用平台头文件中的 kOpcode* 常量。
struct OpcodeEntry {
    std::uint16_t id;
    std::string_view name;
};

constexpr std::array<OpcodeEntry, 66> kPlatformTable = {{
    {kOpcodeVerify,           "VERIFY_LX64"},
    {kOpcodePushInteger,      "PUSHI_LX64"},
    {kOpcodePushString,       "PUSHS_LX64"},
    {kOpcodePushNull,         "PUSHNULL_LX64"},
    {kOpcodePushReference,    "PUSHREF_LX64"},
    {kOpcodeDereference,      "DEREF_LX64"},
    {kOpcodeFieldReference,   "FIELDREF_LX64"},
    {kOpcodeLoad,             "LOAD_LX64"},
    {kOpcodeStore,            "STORE_LX64"},
    {kOpcodeStoreDereference, "STORE_DEREF_LX64"},
    {kOpcodeStorePointer,     "STOREPTR_LX64"},
    {kOpcodePop,              "POP_LX64"},
    {kOpcodeAdd,              "ADD_LX64"},
    {kOpcodeAddTrap,          "ADD_trap_LX64"},
    {kOpcodeAddWrap,          "ADD_wrap_LX64"},
    {kOpcodeAddSaturate,      "ADD_saturate_LX64"},
    {kOpcodeAddExtend,        "ADD_extend_LX64"},
    {kOpcodeSub,              "SUB_LX64"},
    {kOpcodeSubTrap,          "SUB_trap_LX64"},
    {kOpcodeSubWrap,          "SUB_wrap_LX64"},
    {kOpcodeSubSaturate,      "SUB_saturate_LX64"},
    {kOpcodeSubExtend,        "SUB_extend_LX64"},
    {kOpcodeMul,              "MUL_LX64"},
    {kOpcodeMulTrap,          "MUL_trap_LX64"},
    {kOpcodeMulWrap,          "MUL_wrap_LX64"},
    {kOpcodeMulSaturate,      "MUL_saturate_LX64"},
    {kOpcodeMulExtend,        "MUL_extend_LX64"},
    {kOpcodeDiv,              "DIV_LX64"},
    {kOpcodeDivTrap,          "DIV_trap_LX64"},
    {kOpcodeDivWrap,          "DIV_wrap_LX64"},
    {kOpcodeDivSaturate,      "DIV_saturate_LX64"},
    {kOpcodeDivExtend,        "DIV_extend_LX64"},
    {kOpcodeEqual,            "EQ_LX64"},
    {kOpcodeNotEqual,         "NE_LX64"},
    {kOpcodeLess,             "LT_LX64"},
    {kOpcodeLessEqual,        "LE_LX64"},
    {kOpcodeGreater,          "GT_LX64"},
    {kOpcodeGreaterEqual,     "GE_LX64"},
    {kOpcodeAnd,              "AND_LX64"},
    {kOpcodeOr,               "OR_LX64"},
    {kOpcodeXor,              "XOR_LX64"},
    {kOpcodeNand,             "NAND_LX64"},
    {kOpcodeNor,              "NOR_LX64"},
    {kOpcodeNot,              "NOT_LX64"},
    {kOpcodeJump,             "JMP_LX64"},
    {kOpcodeJumpIfZero,       "JZ_LX64"},
    {kOpcodeCheckApproval,    "CHECKAPPROVAL_LX64"},
    {kOpcodeCheckLimits,      "CHECKLIMITS_LX64"},
    {kOpcodeCheckRole,        "CHECKROLE_LX64"},
    {kOpcodeCheckRoleIndirect,"CHECKROLEIND_LX64"},
    {kOpcodeNullCheck,        "NULLCHECK_LX64"},
    {kOpcodeAssert,           "ASSERT_LX64"},
    {kOpcodeCall,             "CALL_LX64"},
    {kOpcodeCallIndirect,     "CALLIND_LX64"},
    {kOpcodeOutputInt,        "OUTPUTINT_LX64"},
    {kOpcodeOutputChar,       "OUTPUTCHAR_LX64"},
    {kOpcodePrintLine,        "PRINTLN_LX64"},
    {kOpcodeInputInt,         "INPUTINT_LX64"},
    {kOpcodeInputChar,        "INPUTCHAR_LX64"},
    {kOpcodeVerifyEcc,        "VERIFYECC_LX64"},
    {kOpcodeCopyMemory,       "COPYMEMORY_LX64"},
    {kOpcodeCompareMemory,    "COMPAREMEMORY_LX64"},
    {kOpcodeRole,             "ROLE_LX64"},
    {kOpcodeOperatorInput,    "OPINPUT_LX64"},
    {kOpcodeReturn,           "RET_LX64"},
    {kOpcodeHalt,             "HALT_LX64"},
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
