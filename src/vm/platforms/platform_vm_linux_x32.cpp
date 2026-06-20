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

constexpr DispatchEntry kEntries[] = {
    {platform_bytecode::kOpcodeVerify,             "verify function fingerprint (linux-x32)"},
    {platform_bytecode::kOpcodePushInteger,        "push integer constant (linux-x32)"},
    {platform_bytecode::kOpcodePushString,         "push string constant (linux-x32)"},
    {platform_bytecode::kOpcodePushNull,           "push null literal (linux-x32)"},
    {platform_bytecode::kOpcodePushReference,      "push reference to local (linux-x32)"},
    {platform_bytecode::kOpcodeDereference,        "dereference reference (linux-x32)"},
    {platform_bytecode::kOpcodeFieldReference,     "compute field reference (linux-x32)"},
    {platform_bytecode::kOpcodeLoad,               "load local value (linux-x32)"},
    {platform_bytecode::kOpcodeStore,              "store local value (linux-x32)"},
    {platform_bytecode::kOpcodeStoreDereference,   "store through reference (linux-x32)"},
    {platform_bytecode::kOpcodeStorePointer,       "store function pointer (linux-x32)"},
    {platform_bytecode::kOpcodePop,                "pop top of stack (linux-x32)"},
    {platform_bytecode::kOpcodeAdd,                "integer add (linux-x32)"},
    {platform_bytecode::kOpcodeAddTrap,            "integer add trap (linux-x32)"},
    {platform_bytecode::kOpcodeAddWrap,            "integer add wrap (linux-x32)"},
    {platform_bytecode::kOpcodeAddSaturate,        "integer add saturate (linux-x32)"},
    {platform_bytecode::kOpcodeAddExtend,          "integer add extend (linux-x32)"},
    {platform_bytecode::kOpcodeSub,                "integer sub (linux-x32)"},
    {platform_bytecode::kOpcodeSubTrap,            "integer sub trap (linux-x32)"},
    {platform_bytecode::kOpcodeSubWrap,            "integer sub wrap (linux-x32)"},
    {platform_bytecode::kOpcodeSubSaturate,        "integer sub saturate (linux-x32)"},
    {platform_bytecode::kOpcodeSubExtend,          "integer sub extend (linux-x32)"},
    {platform_bytecode::kOpcodeMul,                "integer mul (linux-x32)"},
    {platform_bytecode::kOpcodeMulTrap,            "integer mul trap (linux-x32)"},
    {platform_bytecode::kOpcodeMulWrap,            "integer mul wrap (linux-x32)"},
    {platform_bytecode::kOpcodeMulSaturate,        "integer mul saturate (linux-x32)"},
    {platform_bytecode::kOpcodeMulExtend,          "integer mul extend (linux-x32)"},
    {platform_bytecode::kOpcodeDiv,                "integer div (linux-x32)"},
    {platform_bytecode::kOpcodeDivTrap,            "integer div trap (linux-x32)"},
    {platform_bytecode::kOpcodeDivWrap,            "integer div wrap (linux-x32)"},
    {platform_bytecode::kOpcodeDivSaturate,        "integer div saturate (linux-x32)"},
    {platform_bytecode::kOpcodeDivExtend,          "integer div extend (linux-x32)"},
    {platform_bytecode::kOpcodeEqual,              "compare equal (linux-x32)"},
    {platform_bytecode::kOpcodeNotEqual,           "compare not equal (linux-x32)"},
    {platform_bytecode::kOpcodeLess,               "compare less (linux-x32)"},
    {platform_bytecode::kOpcodeLessEqual,          "compare less equal (linux-x32)"},
    {platform_bytecode::kOpcodeGreater,            "compare greater (linux-x32)"},
    {platform_bytecode::kOpcodeGreaterEqual,       "compare greater equal (linux-x32)"},
    {platform_bytecode::kOpcodeAnd,                "logical and (linux-x32)"},
    {platform_bytecode::kOpcodeOr,                 "logical or (linux-x32)"},
    {platform_bytecode::kOpcodeXor,                "logical xor (linux-x32)"},
    {platform_bytecode::kOpcodeNand,               "logical nand (linux-x32)"},
    {platform_bytecode::kOpcodeNor,                "logical nor (linux-x32)"},
    {platform_bytecode::kOpcodeNot,                "logical not (linux-x32)"},
    {platform_bytecode::kOpcodeJump,               "unconditional jump (linux-x32)"},
    {platform_bytecode::kOpcodeJumpIfZero,         "jump if zero (linux-x32)"},
    {platform_bytecode::kOpcodeCheckApproval,      "check approval gate (linux-x32)"},
    {platform_bytecode::kOpcodeCheckLimits,        "check resource limits (linux-x32)"},
    {platform_bytecode::kOpcodeCheckRole,          "check role (linux-x32)"},
    {platform_bytecode::kOpcodeCheckRoleIndirect,  "check role via pointer (linux-x32)"},
    {platform_bytecode::kOpcodeNullCheck,          "null function pointer check (linux-x32)"},
    {platform_bytecode::kOpcodeAssert,             "proof obligation assert (linux-x32)"},
    {platform_bytecode::kOpcodeCall,               "direct call (linux-x32)"},
    {platform_bytecode::kOpcodeCallIndirect,       "indirect call (linux-x32)"},
    {platform_bytecode::kOpcodeOutputInt,          "output integer (linux-x32)"},
    {platform_bytecode::kOpcodeOutputChar,         "output character (linux-x32)"},
    {platform_bytecode::kOpcodePrintLine,          "print line (linux-x32)"},
    {platform_bytecode::kOpcodeInputInt,           "input integer (linux-x32)"},
    {platform_bytecode::kOpcodeInputChar,          "input character (linux-x32)"},
    {platform_bytecode::kOpcodeVerifyEcc,          "verify memory ECC (linux-x32)"},
    {platform_bytecode::kOpcodeCopyMemory,         "copy memory (linux-x32)"},
    {platform_bytecode::kOpcodeCompareMemory,      "compare memory (linux-x32)"},
    {platform_bytecode::kOpcodeRole,               "role chain operation (linux-x32)"},
    {platform_bytecode::kOpcodeOperatorInput,      "operator yes/no input (linux-x32)"},
    {platform_bytecode::kOpcodeReturn,             "function return (linux-x32)"},
    {platform_bytecode::kOpcodeHalt,               "halt VM (linux-x32)"},
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

bool isOwnPlatformOpcode(std::uint16_t id) {
    if (static_cast<std::uint8_t>(id >> 8) != TORTURE_PLATFORM_ID) {
        return false;
    }
    const auto local = static_cast<std::uint16_t>(id & 0xFFu);
    // linux_x32 低 8 位编号范围：0x43 - 0x84
    return local >= 0x43 && local <= 0x84;
}

}  // namespace torture::vm::platform_vm
