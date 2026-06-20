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

// android-x64 平台调度表。低 8 位编号范围：0x5A - 0x9B。
constexpr DispatchEntry kEntries[] = {
    {platform_bytecode::kOpcodeVerify,             "verify function fingerprint (android-x64)"},
    {platform_bytecode::kOpcodePushInteger,        "push integer constant (android-x64)"},
    {platform_bytecode::kOpcodePushString,         "push string constant (android-x64)"},
    {platform_bytecode::kOpcodePushNull,           "push null literal (android-x64)"},
    {platform_bytecode::kOpcodePushReference,      "push reference to local (android-x64)"},
    {platform_bytecode::kOpcodeDereference,        "dereference reference (android-x64)"},
    {platform_bytecode::kOpcodeFieldReference,     "compute field reference (android-x64)"},
    {platform_bytecode::kOpcodeLoad,               "load local value (android-x64)"},
    {platform_bytecode::kOpcodeStore,              "store local value (android-x64)"},
    {platform_bytecode::kOpcodeStoreDereference,   "store through reference (android-x64)"},
    {platform_bytecode::kOpcodeStorePointer,       "store function pointer (android-x64)"},
    {platform_bytecode::kOpcodePop,                "pop top of stack (android-x64)"},
    {platform_bytecode::kOpcodeAdd,                "integer add (android-x64)"},
    {platform_bytecode::kOpcodeAddTrap,            "integer add trap (android-x64)"},
    {platform_bytecode::kOpcodeAddWrap,            "integer add wrap (android-x64)"},
    {platform_bytecode::kOpcodeAddSaturate,        "integer add saturate (android-x64)"},
    {platform_bytecode::kOpcodeAddExtend,          "integer add extend (android-x64)"},
    {platform_bytecode::kOpcodeSub,                "integer sub (android-x64)"},
    {platform_bytecode::kOpcodeSubTrap,            "integer sub trap (android-x64)"},
    {platform_bytecode::kOpcodeSubWrap,            "integer sub wrap (android-x64)"},
    {platform_bytecode::kOpcodeSubSaturate,        "integer sub saturate (android-x64)"},
    {platform_bytecode::kOpcodeSubExtend,          "integer sub extend (android-x64)"},
    {platform_bytecode::kOpcodeMul,                "integer mul (android-x64)"},
    {platform_bytecode::kOpcodeMulTrap,            "integer mul trap (android-x64)"},
    {platform_bytecode::kOpcodeMulWrap,            "integer mul wrap (android-x64)"},
    {platform_bytecode::kOpcodeMulSaturate,        "integer mul saturate (android-x64)"},
    {platform_bytecode::kOpcodeMulExtend,          "integer mul extend (android-x64)"},
    {platform_bytecode::kOpcodeDiv,                "integer div (android-x64)"},
    {platform_bytecode::kOpcodeDivTrap,            "integer div trap (android-x64)"},
    {platform_bytecode::kOpcodeDivWrap,            "integer div wrap (android-x64)"},
    {platform_bytecode::kOpcodeDivSaturate,        "integer div saturate (android-x64)"},
    {platform_bytecode::kOpcodeDivExtend,          "integer div extend (android-x64)"},
    {platform_bytecode::kOpcodeEqual,              "compare equal (android-x64)"},
    {platform_bytecode::kOpcodeNotEqual,           "compare not equal (android-x64)"},
    {platform_bytecode::kOpcodeLess,               "compare less (android-x64)"},
    {platform_bytecode::kOpcodeLessEqual,          "compare less equal (android-x64)"},
    {platform_bytecode::kOpcodeGreater,            "compare greater (android-x64)"},
    {platform_bytecode::kOpcodeGreaterEqual,       "compare greater equal (android-x64)"},
    {platform_bytecode::kOpcodeAnd,                "logical and (android-x64)"},
    {platform_bytecode::kOpcodeOr,                 "logical or (android-x64)"},
    {platform_bytecode::kOpcodeXor,                "logical xor (android-x64)"},
    {platform_bytecode::kOpcodeNand,               "logical nand (android-x64)"},
    {platform_bytecode::kOpcodeNor,                "logical nor (android-x64)"},
    {platform_bytecode::kOpcodeNot,                "logical not (android-x64)"},
    {platform_bytecode::kOpcodeJump,               "unconditional jump (android-x64)"},
    {platform_bytecode::kOpcodeJumpIfZero,         "jump if zero (android-x64)"},
    {platform_bytecode::kOpcodeCheckApproval,      "check approval gate (android-x64)"},
    {platform_bytecode::kOpcodeCheckLimits,        "check resource limits (android-x64)"},
    {platform_bytecode::kOpcodeCheckRole,          "check role (android-x64)"},
    {platform_bytecode::kOpcodeCheckRoleIndirect,  "check role via pointer (android-x64)"},
    {platform_bytecode::kOpcodeNullCheck,          "null function pointer check (android-x64)"},
    {platform_bytecode::kOpcodeAssert,             "proof obligation assert (android-x64)"},
    {platform_bytecode::kOpcodeCall,               "direct call (android-x64)"},
    {platform_bytecode::kOpcodeCallIndirect,       "indirect call (android-x64)"},
    {platform_bytecode::kOpcodeOutputInt,          "output integer (android-x64)"},
    {platform_bytecode::kOpcodeOutputChar,         "output character (android-x64)"},
    {platform_bytecode::kOpcodePrintLine,          "print line (android-x64)"},
    {platform_bytecode::kOpcodeInputInt,           "input integer (android-x64)"},
    {platform_bytecode::kOpcodeInputChar,          "input character (android-x64)"},
    {platform_bytecode::kOpcodeVerifyEcc,          "verify memory ECC (android-x64)"},
    {platform_bytecode::kOpcodeCopyMemory,         "copy memory (android-x64)"},
    {platform_bytecode::kOpcodeCompareMemory,      "compare memory (android-x64)"},
    {platform_bytecode::kOpcodeRole,               "role chain operation (android-x64)"},
    {platform_bytecode::kOpcodeOperatorInput,      "operator yes/no input (android-x64)"},
    {platform_bytecode::kOpcodeReturn,             "function return (android-x64)"},
    {platform_bytecode::kOpcodeHalt,               "halt VM (android-x64)"},
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

// android-x64 低 8 位编号范围：0x5A - 0x9B
bool isOwnPlatformOpcode(std::uint16_t id) {
    if (static_cast<std::uint8_t>(id >> 8) != TORTURE_PLATFORM_ID) {
        return false;
    }
    const auto local = static_cast<std::uint16_t>(id & 0xFFu);
    return local >= 0x5A && local <= 0x9B;
}

}  // namespace torture::vm::platform_vm
