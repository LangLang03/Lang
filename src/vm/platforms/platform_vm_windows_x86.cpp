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
    {platform_bytecode::kOpcodeVerify,             "verify function fingerprint (windows-x86)"},
    {platform_bytecode::kOpcodePushInteger,        "push integer constant (windows-x86)"},
    {platform_bytecode::kOpcodePushString,         "push string constant (windows-x86)"},
    {platform_bytecode::kOpcodePushNull,           "push null literal (windows-x86)"},
    {platform_bytecode::kOpcodePushReference,      "push reference to local (windows-x86)"},
    {platform_bytecode::kOpcodeDereference,        "dereference reference (windows-x86)"},
    {platform_bytecode::kOpcodeFieldReference,     "compute field reference (windows-x86)"},
    {platform_bytecode::kOpcodeLoad,               "load local value (windows-x86)"},
    {platform_bytecode::kOpcodeStore,              "store local value (windows-x86)"},
    {platform_bytecode::kOpcodeStoreDereference,   "store through reference (windows-x86)"},
    {platform_bytecode::kOpcodeStorePointer,       "store function pointer (windows-x86)"},
    {platform_bytecode::kOpcodePop,                "pop top of stack (windows-x86)"},
    {platform_bytecode::kOpcodeAdd,                "integer add (windows-x86)"},
    {platform_bytecode::kOpcodeAddTrap,            "integer add trap (windows-x86)"},
    {platform_bytecode::kOpcodeAddWrap,            "integer add wrap (windows-x86)"},
    {platform_bytecode::kOpcodeAddSaturate,        "integer add saturate (windows-x86)"},
    {platform_bytecode::kOpcodeAddExtend,          "integer add extend (windows-x86)"},
    {platform_bytecode::kOpcodeSub,                "integer sub (windows-x86)"},
    {platform_bytecode::kOpcodeSubTrap,            "integer sub trap (windows-x86)"},
    {platform_bytecode::kOpcodeSubWrap,            "integer sub wrap (windows-x86)"},
    {platform_bytecode::kOpcodeSubSaturate,        "integer sub saturate (windows-x86)"},
    {platform_bytecode::kOpcodeSubExtend,          "integer sub extend (windows-x86)"},
    {platform_bytecode::kOpcodeMul,                "integer mul (windows-x86)"},
    {platform_bytecode::kOpcodeMulTrap,            "integer mul trap (windows-x86)"},
    {platform_bytecode::kOpcodeMulWrap,            "integer mul wrap (windows-x86)"},
    {platform_bytecode::kOpcodeMulSaturate,        "integer mul saturate (windows-x86)"},
    {platform_bytecode::kOpcodeMulExtend,          "integer mul extend (windows-x86)"},
    {platform_bytecode::kOpcodeDiv,                "integer div (windows-x86)"},
    {platform_bytecode::kOpcodeDivTrap,            "integer div trap (windows-x86)"},
    {platform_bytecode::kOpcodeDivWrap,            "integer div wrap (windows-x86)"},
    {platform_bytecode::kOpcodeDivSaturate,        "integer div saturate (windows-x86)"},
    {platform_bytecode::kOpcodeDivExtend,          "integer div extend (windows-x86)"},
    {platform_bytecode::kOpcodeEqual,              "compare equal (windows-x86)"},
    {platform_bytecode::kOpcodeNotEqual,           "compare not equal (windows-x86)"},
    {platform_bytecode::kOpcodeLess,               "compare less (windows-x86)"},
    {platform_bytecode::kOpcodeLessEqual,          "compare less equal (windows-x86)"},
    {platform_bytecode::kOpcodeGreater,            "compare greater (windows-x86)"},
    {platform_bytecode::kOpcodeGreaterEqual,       "compare greater equal (windows-x86)"},
    {platform_bytecode::kOpcodeAnd,                "logical and (windows-x86)"},
    {platform_bytecode::kOpcodeOr,                 "logical or (windows-x86)"},
    {platform_bytecode::kOpcodeXor,                "logical xor (windows-x86)"},
    {platform_bytecode::kOpcodeNand,               "logical nand (windows-x86)"},
    {platform_bytecode::kOpcodeNor,                "logical nor (windows-x86)"},
    {platform_bytecode::kOpcodeNot,                "logical not (windows-x86)"},
    {platform_bytecode::kOpcodeJump,               "unconditional jump (windows-x86)"},
    {platform_bytecode::kOpcodeJumpIfZero,         "jump if zero (windows-x86)"},
    {platform_bytecode::kOpcodeCheckApproval,      "check approval gate (windows-x86)"},
    {platform_bytecode::kOpcodeCheckLimits,        "check resource limits (windows-x86)"},
    {platform_bytecode::kOpcodeCheckRole,          "check role (windows-x86)"},
    {platform_bytecode::kOpcodeCheckRoleIndirect,  "check role via pointer (windows-x86)"},
    {platform_bytecode::kOpcodeNullCheck,          "null function pointer check (windows-x86)"},
    {platform_bytecode::kOpcodeAssert,             "proof obligation assert (windows-x86)"},
    {platform_bytecode::kOpcodeCall,               "direct call (windows-x86)"},
    {platform_bytecode::kOpcodeCallIndirect,       "indirect call (windows-x86)"},
    {platform_bytecode::kOpcodeOutputInt,          "output integer (windows-x86)"},
    {platform_bytecode::kOpcodeOutputChar,         "output character (windows-x86)"},
    {platform_bytecode::kOpcodePrintLine,          "print line (windows-x86)"},
    {platform_bytecode::kOpcodeInputInt,           "input integer (windows-x86)"},
    {platform_bytecode::kOpcodeInputChar,          "input character (windows-x86)"},
    {platform_bytecode::kOpcodeVerifyEcc,          "verify memory ECC (windows-x86)"},
    {platform_bytecode::kOpcodeCopyMemory,         "copy memory (windows-x86)"},
    {platform_bytecode::kOpcodeCompareMemory,      "compare memory (windows-x86)"},
    {platform_bytecode::kOpcodeRole,               "role chain operation (windows-x86)"},
    {platform_bytecode::kOpcodeOperatorInput,      "operator yes/no input (windows-x86)"},
    {platform_bytecode::kOpcodeReturn,             "function return (windows-x86)"},
    {platform_bytecode::kOpcodeHalt,               "halt VM (windows-x86)"},
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
    const auto local = static_cast<std::uint8_t>(id & 0xFFu);
    return local >= platform_bytecode::kLocalOpcodeVerify &&
           local <= platform_bytecode::kLocalOpcodeHalt;
}

}  // namespace torture::vm::platform_vm
