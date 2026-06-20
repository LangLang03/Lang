#include "vm/platform_vm.h"

#include "vm/platform.h"
#include "vm/platform_bytecode.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace torture::vm::platform_vm {

namespace pbc = ::torture::vm::platform_bytecode;

struct DispatchEntry {
    std::uint16_t id;
    const char* description;
};

// android_arm64 平台调度表。低 8 位编号范围: 0xDE-0xFF (34 个) + 0x01-0x20 (32 个)。
constexpr DispatchEntry kEntries[] = {
    {platform_bytecode::kOpcodeVerify,             "verify function fingerprint (android-arm64)"},
    {platform_bytecode::kOpcodePushInteger,        "push integer constant (android-arm64)"},
    {platform_bytecode::kOpcodePushString,         "push string constant (android-arm64)"},
    {platform_bytecode::kOpcodePushNull,           "push null literal (android-arm64)"},
    {platform_bytecode::kOpcodePushReference,      "push reference to local (android-arm64)"},
    {platform_bytecode::kOpcodeDereference,        "dereference reference (android-arm64)"},
    {platform_bytecode::kOpcodeFieldReference,     "compute field reference (android-arm64)"},
    {platform_bytecode::kOpcodeLoad,               "load local value (android-arm64)"},
    {platform_bytecode::kOpcodeStore,              "store local value (android-arm64)"},
    {platform_bytecode::kOpcodeStoreDereference,   "store through reference (android-arm64)"},
    {platform_bytecode::kOpcodeStorePointer,       "store function pointer (android-arm64)"},
    {platform_bytecode::kOpcodePop,                "pop top of stack (android-arm64)"},
    {platform_bytecode::kOpcodeAdd,                "integer add (android-arm64)"},
    {platform_bytecode::kOpcodeAddTrap,            "integer add trap (android-arm64)"},
    {platform_bytecode::kOpcodeAddWrap,            "integer add wrap (android-arm64)"},
    {platform_bytecode::kOpcodeAddSaturate,        "integer add saturate (android-arm64)"},
    {platform_bytecode::kOpcodeAddExtend,          "integer add extend (android-arm64)"},
    {platform_bytecode::kOpcodeSub,                "integer sub (android-arm64)"},
    {platform_bytecode::kOpcodeSubTrap,            "integer sub trap (android-arm64)"},
    {platform_bytecode::kOpcodeSubWrap,            "integer sub wrap (android-arm64)"},
    {platform_bytecode::kOpcodeSubSaturate,        "integer sub saturate (android-arm64)"},
    {platform_bytecode::kOpcodeSubExtend,          "integer sub extend (android-arm64)"},
    {platform_bytecode::kOpcodeMul,                "integer mul (android-arm64)"},
    {platform_bytecode::kOpcodeMulTrap,            "integer mul trap (android-arm64)"},
    {platform_bytecode::kOpcodeMulWrap,            "integer mul wrap (android-arm64)"},
    {platform_bytecode::kOpcodeMulSaturate,        "integer mul saturate (android-arm64)"},
    {platform_bytecode::kOpcodeMulExtend,          "integer mul extend (android-arm64)"},
    {platform_bytecode::kOpcodeDiv,                "integer div (android-arm64)"},
    {platform_bytecode::kOpcodeDivTrap,            "integer div trap (android-arm64)"},
    {platform_bytecode::kOpcodeDivWrap,            "integer div wrap (android-arm64)"},
    {platform_bytecode::kOpcodeDivSaturate,        "integer div saturate (android-arm64)"},
    {platform_bytecode::kOpcodeDivExtend,          "integer div extend (android-arm64)"},
    {platform_bytecode::kOpcodeEqual,              "compare equal (android-arm64)"},
    {platform_bytecode::kOpcodeNotEqual,           "compare not equal (android-arm64)"},
    {platform_bytecode::kOpcodeLess,               "compare less (android-arm64)"},
    {platform_bytecode::kOpcodeLessEqual,          "compare less equal (android-arm64)"},
    {platform_bytecode::kOpcodeGreater,            "compare greater (android-arm64)"},
    {platform_bytecode::kOpcodeGreaterEqual,       "compare greater equal (android-arm64)"},
    {platform_bytecode::kOpcodeAnd,                "logical and (android-arm64)"},
    {platform_bytecode::kOpcodeOr,                 "logical or (android-arm64)"},
    {platform_bytecode::kOpcodeXor,                "logical xor (android-arm64)"},
    {platform_bytecode::kOpcodeNand,               "logical nand (android-arm64)"},
    {platform_bytecode::kOpcodeNor,                "logical nor (android-arm64)"},
    {platform_bytecode::kOpcodeNot,                "logical not (android-arm64)"},
    {platform_bytecode::kOpcodeJump,               "unconditional jump (android-arm64)"},
    {platform_bytecode::kOpcodeJumpIfZero,         "jump if zero (android-arm64)"},
    {platform_bytecode::kOpcodeCheckApproval,      "check approval gate (android-arm64)"},
    {platform_bytecode::kOpcodeCheckLimits,        "check resource limits (android-arm64)"},
    {platform_bytecode::kOpcodeCheckRole,          "check role (android-arm64)"},
    {platform_bytecode::kOpcodeCheckRoleIndirect,  "check role via pointer (android-arm64)"},
    {platform_bytecode::kOpcodeNullCheck,          "null function pointer check (android-arm64)"},
    {platform_bytecode::kOpcodeAssert,             "proof obligation assert (android-arm64)"},
    {platform_bytecode::kOpcodeCall,               "direct call (android-arm64)"},
    {platform_bytecode::kOpcodeCallIndirect,       "indirect call (android-arm64)"},
    {platform_bytecode::kOpcodeOutputInt,          "output integer (android-arm64)"},
    {platform_bytecode::kOpcodeOutputChar,         "output character (android-arm64)"},
    {platform_bytecode::kOpcodePrintLine,          "print line (android-arm64)"},
    {platform_bytecode::kOpcodeInputInt,           "input integer (android-arm64)"},
    {platform_bytecode::kOpcodeInputChar,          "input character (android-arm64)"},
    {platform_bytecode::kOpcodeVerifyEcc,          "verify memory ECC (android-arm64)"},
    {platform_bytecode::kOpcodeCopyMemory,         "copy memory (android-arm64)"},
    {platform_bytecode::kOpcodeCompareMemory,      "compare memory (android-arm64)"},
    {platform_bytecode::kOpcodeRole,               "role chain operation (android-arm64)"},
    {platform_bytecode::kOpcodeOperatorInput,      "operator yes/no input (android-arm64)"},
    {platform_bytecode::kOpcodeReturn,             "function return (android-arm64)"},
    {platform_bytecode::kOpcodeHalt,               "halt VM (android-arm64)"},
};

const std::unordered_map<std::uint16_t, const char*>& dispatchTable() {
    static const std::unordered_map<std::uint16_t, const char*> table = []() {
        std::unordered_map<std::uint16_t, const char*> built;
        built.reserve(sizeof(kEntries) / sizeof(kEntries[0]));
        for (const auto& entry : kEntries) {
            built.emplace(entry.id, entry.description);
        }
        return built;
    }();
    return table;
}

// android_arm64 低 8 位编号范围: 0xDE-0xFF (34 个) + 0x01-0x20 (32 个)，环绕。
bool isOwnPlatformOpcode(std::uint16_t id) {
    if (static_cast<std::uint8_t>(id >> 8) != TORTURE_PLATFORM_ID) {
        return false;
    }
    const auto local = static_cast<std::uint16_t>(id & 0xFFu);
    return (local >= 0xDE && local <= 0xFF) || (local >= 0x01 && local <= 0x20);
}

}  // namespace torture::vm::platform_vm
