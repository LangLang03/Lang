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
    {platform_bytecode::kOpcodeVerify,             "verify function fingerprint (windows-x64)"},
    {platform_bytecode::kOpcodePushInteger,        "push integer constant (windows-x64)"},
    {platform_bytecode::kOpcodePushString,         "push string constant (windows-x64)"},
    {platform_bytecode::kOpcodePushNull,           "push null literal (windows-x64)"},
    {platform_bytecode::kOpcodePushReference,      "push reference to local (windows-x64)"},
    {platform_bytecode::kOpcodeDereference,        "dereference reference (windows-x64)"},
    {platform_bytecode::kOpcodeFieldReference,     "compute field reference (windows-x64)"},
    {platform_bytecode::kOpcodeLoad,               "load local value (windows-x64)"},
    {platform_bytecode::kOpcodeStore,              "store local value (windows-x64)"},
    {platform_bytecode::kOpcodeStoreDereference,   "store through reference (windows-x64)"},
    {platform_bytecode::kOpcodeStorePointer,       "store function pointer (windows-x64)"},
    {platform_bytecode::kOpcodePop,                "pop top of stack (windows-x64)"},
    {platform_bytecode::kOpcodeAdd,                "integer add (windows-x64)"},
    {platform_bytecode::kOpcodeAddTrap,            "integer add trap (windows-x64)"},
    {platform_bytecode::kOpcodeAddWrap,            "integer add wrap (windows-x64)"},
    {platform_bytecode::kOpcodeAddSaturate,        "integer add saturate (windows-x64)"},
    {platform_bytecode::kOpcodeAddExtend,          "integer add extend (windows-x64)"},
    {platform_bytecode::kOpcodeSub,                "integer sub (windows-x64)"},
    {platform_bytecode::kOpcodeSubTrap,            "integer sub trap (windows-x64)"},
    {platform_bytecode::kOpcodeSubWrap,            "integer sub wrap (windows-x64)"},
    {platform_bytecode::kOpcodeSubSaturate,        "integer sub saturate (windows-x64)"},
    {platform_bytecode::kOpcodeSubExtend,          "integer sub extend (windows-x64)"},
    {platform_bytecode::kOpcodeMul,                "integer mul (windows-x64)"},
    {platform_bytecode::kOpcodeMulTrap,            "integer mul trap (windows-x64)"},
    {platform_bytecode::kOpcodeMulWrap,            "integer mul wrap (windows-x64)"},
    {platform_bytecode::kOpcodeMulSaturate,        "integer mul saturate (windows-x64)"},
    {platform_bytecode::kOpcodeMulExtend,          "integer mul extend (windows-x64)"},
    {platform_bytecode::kOpcodeDiv,                "integer div (windows-x64)"},
    {platform_bytecode::kOpcodeDivTrap,            "integer div trap (windows-x64)"},
    {platform_bytecode::kOpcodeDivWrap,            "integer div wrap (windows-x64)"},
    {platform_bytecode::kOpcodeDivSaturate,        "integer div saturate (windows-x64)"},
    {platform_bytecode::kOpcodeDivExtend,          "integer div extend (windows-x64)"},
    {platform_bytecode::kOpcodeEqual,              "compare equal (windows-x64)"},
    {platform_bytecode::kOpcodeNotEqual,           "compare not equal (windows-x64)"},
    {platform_bytecode::kOpcodeLess,               "compare less (windows-x64)"},
    {platform_bytecode::kOpcodeLessEqual,          "compare less equal (windows-x64)"},
    {platform_bytecode::kOpcodeGreater,            "compare greater (windows-x64)"},
    {platform_bytecode::kOpcodeGreaterEqual,       "compare greater equal (windows-x64)"},
    {platform_bytecode::kOpcodeAnd,                "logical and (windows-x64)"},
    {platform_bytecode::kOpcodeOr,                 "logical or (windows-x64)"},
    {platform_bytecode::kOpcodeXor,                "logical xor (windows-x64)"},
    {platform_bytecode::kOpcodeNand,               "logical nand (windows-x64)"},
    {platform_bytecode::kOpcodeNor,                "logical nor (windows-x64)"},
    {platform_bytecode::kOpcodeNot,                "logical not (windows-x64)"},
    {platform_bytecode::kOpcodeJump,               "unconditional jump (windows-x64)"},
    {platform_bytecode::kOpcodeJumpIfZero,         "jump if zero (windows-x64)"},
    {platform_bytecode::kOpcodeCheckApproval,      "check approval gate (windows-x64)"},
    {platform_bytecode::kOpcodeCheckLimits,        "check resource limits (windows-x64)"},
    {platform_bytecode::kOpcodeCheckRole,          "check role (windows-x64)"},
    {platform_bytecode::kOpcodeCheckRoleIndirect,  "check role via pointer (windows-x64)"},
    {platform_bytecode::kOpcodeNullCheck,          "null function pointer check (windows-x64)"},
    {platform_bytecode::kOpcodeAssert,             "proof obligation assert (windows-x64)"},
    {platform_bytecode::kOpcodeCall,               "direct call (windows-x64)"},
    {platform_bytecode::kOpcodeCallIndirect,       "indirect call (windows-x64)"},
    {platform_bytecode::kOpcodeOutputInt,          "output integer (windows-x64)"},
    {platform_bytecode::kOpcodeOutputChar,         "output character (windows-x64)"},
    {platform_bytecode::kOpcodePrintLine,          "print line (windows-x64)"},
    {platform_bytecode::kOpcodeInputInt,           "input integer (windows-x64)"},
    {platform_bytecode::kOpcodeInputChar,          "input character (windows-x64)"},
    {platform_bytecode::kOpcodeVerifyEcc,          "verify memory ECC (windows-x64)"},
    {platform_bytecode::kOpcodeCopyMemory,         "copy memory (windows-x64)"},
    {platform_bytecode::kOpcodeCompareMemory,      "compare memory (windows-x64)"},
    {platform_bytecode::kOpcodeRole,               "role chain operation (windows-x64)"},
    {platform_bytecode::kOpcodeOperatorInput,      "operator yes/no input (windows-x64)"},
    {platform_bytecode::kOpcodeReturn,             "function return (windows-x64)"},
    {platform_bytecode::kOpcodeHalt,               "halt VM (windows-x64)"},
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
