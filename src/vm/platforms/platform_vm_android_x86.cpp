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

// android-x86 平台调度表。低 8 位编号范围：0x18 - 0x59。
constexpr DispatchEntry kEntries[] = {
    {platform_bytecode::kOpcodeVerify,             "verify function fingerprint (android-x86)"},
    {platform_bytecode::kOpcodePushInteger,        "push integer constant (android-x86)"},
    {platform_bytecode::kOpcodePushString,         "push string constant (android-x86)"},
    {platform_bytecode::kOpcodePushNull,           "push null literal (android-x86)"},
    {platform_bytecode::kOpcodePushReference,      "push reference to local (android-x86)"},
    {platform_bytecode::kOpcodeDereference,        "dereference reference (android-x86)"},
    {platform_bytecode::kOpcodeFieldReference,     "compute field reference (android-x86)"},
    {platform_bytecode::kOpcodeLoad,               "load local value (android-x86)"},
    {platform_bytecode::kOpcodeStore,              "store local value (android-x86)"},
    {platform_bytecode::kOpcodeStoreDereference,   "store through reference (android-x86)"},
    {platform_bytecode::kOpcodeStorePointer,       "store function pointer (android-x86)"},
    {platform_bytecode::kOpcodePop,                "pop top of stack (android-x86)"},
    {platform_bytecode::kOpcodeAdd,                "integer add (android-x86)"},
    {platform_bytecode::kOpcodeAddTrap,            "integer add trap (android-x86)"},
    {platform_bytecode::kOpcodeAddWrap,            "integer add wrap (android-x86)"},
    {platform_bytecode::kOpcodeAddSaturate,        "integer add saturate (android-x86)"},
    {platform_bytecode::kOpcodeAddExtend,          "integer add extend (android-x86)"},
    {platform_bytecode::kOpcodeSub,                "integer sub (android-x86)"},
    {platform_bytecode::kOpcodeSubTrap,            "integer sub trap (android-x86)"},
    {platform_bytecode::kOpcodeSubWrap,            "integer sub wrap (android-x86)"},
    {platform_bytecode::kOpcodeSubSaturate,        "integer sub saturate (android-x86)"},
    {platform_bytecode::kOpcodeSubExtend,          "integer sub extend (android-x86)"},
    {platform_bytecode::kOpcodeMul,                "integer mul (android-x86)"},
    {platform_bytecode::kOpcodeMulTrap,            "integer mul trap (android-x86)"},
    {platform_bytecode::kOpcodeMulWrap,            "integer mul wrap (android-x86)"},
    {platform_bytecode::kOpcodeMulSaturate,        "integer mul saturate (android-x86)"},
    {platform_bytecode::kOpcodeMulExtend,          "integer mul extend (android-x86)"},
    {platform_bytecode::kOpcodeDiv,                "integer div (android-x86)"},
    {platform_bytecode::kOpcodeDivTrap,            "integer div trap (android-x86)"},
    {platform_bytecode::kOpcodeDivWrap,            "integer div wrap (android-x86)"},
    {platform_bytecode::kOpcodeDivSaturate,        "integer div saturate (android-x86)"},
    {platform_bytecode::kOpcodeDivExtend,          "integer div extend (android-x86)"},
    {platform_bytecode::kOpcodeEqual,              "compare equal (android-x86)"},
    {platform_bytecode::kOpcodeNotEqual,           "compare not equal (android-x86)"},
    {platform_bytecode::kOpcodeLess,               "compare less (android-x86)"},
    {platform_bytecode::kOpcodeLessEqual,          "compare less equal (android-x86)"},
    {platform_bytecode::kOpcodeGreater,            "compare greater (android-x86)"},
    {platform_bytecode::kOpcodeGreaterEqual,       "compare greater equal (android-x86)"},
    {platform_bytecode::kOpcodeAnd,                "logical and (android-x86)"},
    {platform_bytecode::kOpcodeOr,                 "logical or (android-x86)"},
    {platform_bytecode::kOpcodeXor,                "logical xor (android-x86)"},
    {platform_bytecode::kOpcodeNand,               "logical nand (android-x86)"},
    {platform_bytecode::kOpcodeNor,                "logical nor (android-x86)"},
    {platform_bytecode::kOpcodeNot,                "logical not (android-x86)"},
    {platform_bytecode::kOpcodeJump,               "unconditional jump (android-x86)"},
    {platform_bytecode::kOpcodeJumpIfZero,         "jump if zero (android-x86)"},
    {platform_bytecode::kOpcodeCheckApproval,      "check approval gate (android-x86)"},
    {platform_bytecode::kOpcodeCheckLimits,        "check resource limits (android-x86)"},
    {platform_bytecode::kOpcodeCheckRole,          "check role (android-x86)"},
    {platform_bytecode::kOpcodeCheckRoleIndirect,  "check role via pointer (android-x86)"},
    {platform_bytecode::kOpcodeNullCheck,          "null function pointer check (android-x86)"},
    {platform_bytecode::kOpcodeAssert,             "proof obligation assert (android-x86)"},
    {platform_bytecode::kOpcodeCall,               "direct call (android-x86)"},
    {platform_bytecode::kOpcodeCallIndirect,       "indirect call (android-x86)"},
    {platform_bytecode::kOpcodeOutputInt,          "output integer (android-x86)"},
    {platform_bytecode::kOpcodeOutputChar,         "output character (android-x86)"},
    {platform_bytecode::kOpcodePrintLine,          "print line (android-x86)"},
    {platform_bytecode::kOpcodeInputInt,           "input integer (android-x86)"},
    {platform_bytecode::kOpcodeInputChar,          "input character (android-x86)"},
    {platform_bytecode::kOpcodeVerifyEcc,          "verify memory ECC (android-x86)"},
    {platform_bytecode::kOpcodeCopyMemory,         "copy memory (android-x86)"},
    {platform_bytecode::kOpcodeCompareMemory,      "compare memory (android-x86)"},
    {platform_bytecode::kOpcodeRole,               "role chain operation (android-x86)"},
    {platform_bytecode::kOpcodeOperatorInput,      "operator yes/no input (android-x86)"},
    {platform_bytecode::kOpcodeReturn,             "function return (android-x86)"},
    {platform_bytecode::kOpcodeHalt,               "halt VM (android-x86)"},
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

// android-x86 低 8 位编号范围：0x18 - 0x59
bool isOwnPlatformOpcode(std::uint16_t id) {
    if (static_cast<std::uint8_t>(id >> 8) != TORTURE_PLATFORM_ID) {
        return false;
    }
    const auto local = static_cast<std::uint16_t>(id & 0xFFu);
    return local >= 0x18 && local <= 0x59;
}

}  // namespace torture::vm::platform_vm
