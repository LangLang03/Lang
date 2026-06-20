// windows-arm32 平台 VM 调度表实现
// 低 8 位编号范围: 0x94-0xD5

#include "vm/platform_vm.h"

#include "vm/platform.h"
#include "vm/platforms/platform_bytecode_windows_arm32.h"

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
    {platform_bytecode::kOpcodeVerify,             "verify function fingerprint (windows-arm32)"},
    {platform_bytecode::kOpcodePushInteger,        "push integer constant (windows-arm32)"},
    {platform_bytecode::kOpcodePushString,         "push string constant (windows-arm32)"},
    {platform_bytecode::kOpcodePushNull,           "push null literal (windows-arm32)"},
    {platform_bytecode::kOpcodePushReference,      "push reference to local (windows-arm32)"},
    {platform_bytecode::kOpcodeDereference,        "dereference reference (windows-arm32)"},
    {platform_bytecode::kOpcodeFieldReference,     "compute field reference (windows-arm32)"},
    {platform_bytecode::kOpcodeLoad,               "load local value (windows-arm32)"},
    {platform_bytecode::kOpcodeStore,              "store local value (windows-arm32)"},
    {platform_bytecode::kOpcodeStoreDereference,   "store through reference (windows-arm32)"},
    {platform_bytecode::kOpcodeStorePointer,       "store function pointer (windows-arm32)"},
    {platform_bytecode::kOpcodePop,                "pop top of stack (windows-arm32)"},
    {platform_bytecode::kOpcodeAdd,                "integer add (windows-arm32)"},
    {platform_bytecode::kOpcodeAddTrap,            "integer add trap (windows-arm32)"},
    {platform_bytecode::kOpcodeAddWrap,            "integer add wrap (windows-arm32)"},
    {platform_bytecode::kOpcodeAddSaturate,        "integer add saturate (windows-arm32)"},
    {platform_bytecode::kOpcodeAddExtend,          "integer add extend (windows-arm32)"},
    {platform_bytecode::kOpcodeSub,                "integer sub (windows-arm32)"},
    {platform_bytecode::kOpcodeSubTrap,            "integer sub trap (windows-arm32)"},
    {platform_bytecode::kOpcodeSubWrap,            "integer sub wrap (windows-arm32)"},
    {platform_bytecode::kOpcodeSubSaturate,        "integer sub saturate (windows-arm32)"},
    {platform_bytecode::kOpcodeSubExtend,          "integer sub extend (windows-arm32)"},
    {platform_bytecode::kOpcodeMul,                "integer mul (windows-arm32)"},
    {platform_bytecode::kOpcodeMulTrap,            "integer mul trap (windows-arm32)"},
    {platform_bytecode::kOpcodeMulWrap,            "integer mul wrap (windows-arm32)"},
    {platform_bytecode::kOpcodeMulSaturate,        "integer mul saturate (windows-arm32)"},
    {platform_bytecode::kOpcodeMulExtend,          "integer mul extend (windows-arm32)"},
    {platform_bytecode::kOpcodeDiv,                "integer div (windows-arm32)"},
    {platform_bytecode::kOpcodeDivTrap,            "integer div trap (windows-arm32)"},
    {platform_bytecode::kOpcodeDivWrap,            "integer div wrap (windows-arm32)"},
    {platform_bytecode::kOpcodeDivSaturate,        "integer div saturate (windows-arm32)"},
    {platform_bytecode::kOpcodeDivExtend,          "integer div extend (windows-arm32)"},
    {platform_bytecode::kOpcodeEqual,              "compare equal (windows-arm32)"},
    {platform_bytecode::kOpcodeNotEqual,           "compare not equal (windows-arm32)"},
    {platform_bytecode::kOpcodeLess,               "compare less (windows-arm32)"},
    {platform_bytecode::kOpcodeLessEqual,          "compare less equal (windows-arm32)"},
    {platform_bytecode::kOpcodeGreater,            "compare greater (windows-arm32)"},
    {platform_bytecode::kOpcodeGreaterEqual,       "compare greater equal (windows-arm32)"},
    {platform_bytecode::kOpcodeAnd,                "logical and (windows-arm32)"},
    {platform_bytecode::kOpcodeOr,                 "logical or (windows-arm32)"},
    {platform_bytecode::kOpcodeXor,                "logical xor (windows-arm32)"},
    {platform_bytecode::kOpcodeNand,               "logical nand (windows-arm32)"},
    {platform_bytecode::kOpcodeNor,                "logical nor (windows-arm32)"},
    {platform_bytecode::kOpcodeNot,                "logical not (windows-arm32)"},
    {platform_bytecode::kOpcodeJump,               "unconditional jump (windows-arm32)"},
    {platform_bytecode::kOpcodeJumpIfZero,         "jump if zero (windows-arm32)"},
    {platform_bytecode::kOpcodeCheckApproval,      "check approval gate (windows-arm32)"},
    {platform_bytecode::kOpcodeCheckLimits,        "check resource limits (windows-arm32)"},
    {platform_bytecode::kOpcodeCheckRole,          "check role (windows-arm32)"},
    {platform_bytecode::kOpcodeCheckRoleIndirect,  "check role via pointer (windows-arm32)"},
    {platform_bytecode::kOpcodeNullCheck,          "null function pointer check (windows-arm32)"},
    {platform_bytecode::kOpcodeAssert,             "proof obligation assert (windows-arm32)"},
    {platform_bytecode::kOpcodeCall,               "direct call (windows-arm32)"},
    {platform_bytecode::kOpcodeCallIndirect,       "indirect call (windows-arm32)"},
    {platform_bytecode::kOpcodeOutputInt,          "output integer (windows-arm32)"},
    {platform_bytecode::kOpcodeOutputChar,         "output character (windows-arm32)"},
    {platform_bytecode::kOpcodePrintLine,          "print line (windows-arm32)"},
    {platform_bytecode::kOpcodeInputInt,           "input integer (windows-arm32)"},
    {platform_bytecode::kOpcodeInputChar,          "input character (windows-arm32)"},
    {platform_bytecode::kOpcodeVerifyEcc,          "verify memory ECC (windows-arm32)"},
    {platform_bytecode::kOpcodeCopyMemory,         "copy memory (windows-arm32)"},
    {platform_bytecode::kOpcodeCompareMemory,      "compare memory (windows-arm32)"},
    {platform_bytecode::kOpcodeRole,               "role chain operation (windows-arm32)"},
    {platform_bytecode::kOpcodeOperatorInput,      "operator yes/no input (windows-arm32)"},
    {platform_bytecode::kOpcodeReturn,             "function return (windows-arm32)"},
    {platform_bytecode::kOpcodeHalt,               "halt VM (windows-arm32)"},
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

// windows-arm32 低 8 位编号范围: 0x94-0xD5
bool isOwnPlatformOpcode(std::uint16_t id) {
    if (static_cast<std::uint8_t>(id >> 8) != TORTURE_PLATFORM_ID) {
        return false;
    }
    const auto local = static_cast<std::uint16_t>(id & 0xFFu);
    return local >= 0x94 && local <= 0xD5;
}

}  // namespace torture::vm::platform_vm
