// windows-arm64 平台 VM 调度表实现
// 低 8 位编号范围: 0xD6-0xFF, 0x01-0x18 (环绕)

#include "vm/platform_vm.h"

#include "vm/platform.h"
#include "vm/platforms/platform_bytecode_windows_arm64.h"

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
    {platform_bytecode::kOpcodeVerify,             "verify function fingerprint (windows-arm64)"},
    {platform_bytecode::kOpcodePushInteger,        "push integer constant (windows-arm64)"},
    {platform_bytecode::kOpcodePushString,         "push string constant (windows-arm64)"},
    {platform_bytecode::kOpcodePushNull,           "push null literal (windows-arm64)"},
    {platform_bytecode::kOpcodePushReference,      "push reference to local (windows-arm64)"},
    {platform_bytecode::kOpcodeDereference,        "dereference reference (windows-arm64)"},
    {platform_bytecode::kOpcodeFieldReference,     "compute field reference (windows-arm64)"},
    {platform_bytecode::kOpcodeLoad,               "load local value (windows-arm64)"},
    {platform_bytecode::kOpcodeStore,              "store local value (windows-arm64)"},
    {platform_bytecode::kOpcodeStoreDereference,   "store through reference (windows-arm64)"},
    {platform_bytecode::kOpcodeStorePointer,       "store function pointer (windows-arm64)"},
    {platform_bytecode::kOpcodePop,                "pop top of stack (windows-arm64)"},
    {platform_bytecode::kOpcodeAdd,                "integer add (windows-arm64)"},
    {platform_bytecode::kOpcodeAddTrap,            "integer add trap (windows-arm64)"},
    {platform_bytecode::kOpcodeAddWrap,            "integer add wrap (windows-arm64)"},
    {platform_bytecode::kOpcodeAddSaturate,        "integer add saturate (windows-arm64)"},
    {platform_bytecode::kOpcodeAddExtend,          "integer add extend (windows-arm64)"},
    {platform_bytecode::kOpcodeSub,                "integer sub (windows-arm64)"},
    {platform_bytecode::kOpcodeSubTrap,            "integer sub trap (windows-arm64)"},
    {platform_bytecode::kOpcodeSubWrap,            "integer sub wrap (windows-arm64)"},
    {platform_bytecode::kOpcodeSubSaturate,        "integer sub saturate (windows-arm64)"},
    {platform_bytecode::kOpcodeSubExtend,          "integer sub extend (windows-arm64)"},
    {platform_bytecode::kOpcodeMul,                "integer mul (windows-arm64)"},
    {platform_bytecode::kOpcodeMulTrap,            "integer mul trap (windows-arm64)"},
    {platform_bytecode::kOpcodeMulWrap,            "integer mul wrap (windows-arm64)"},
    {platform_bytecode::kOpcodeMulSaturate,        "integer mul saturate (windows-arm64)"},
    {platform_bytecode::kOpcodeMulExtend,          "integer mul extend (windows-arm64)"},
    {platform_bytecode::kOpcodeDiv,                "integer div (windows-arm64)"},
    {platform_bytecode::kOpcodeDivTrap,            "integer div trap (windows-arm64)"},
    {platform_bytecode::kOpcodeDivWrap,            "integer div wrap (windows-arm64)"},
    {platform_bytecode::kOpcodeDivSaturate,        "integer div saturate (windows-arm64)"},
    {platform_bytecode::kOpcodeDivExtend,          "integer div extend (windows-arm64)"},
    {platform_bytecode::kOpcodeEqual,              "compare equal (windows-arm64)"},
    {platform_bytecode::kOpcodeNotEqual,           "compare not equal (windows-arm64)"},
    {platform_bytecode::kOpcodeLess,               "compare less (windows-arm64)"},
    {platform_bytecode::kOpcodeLessEqual,          "compare less equal (windows-arm64)"},
    {platform_bytecode::kOpcodeGreater,            "compare greater (windows-arm64)"},
    {platform_bytecode::kOpcodeGreaterEqual,       "compare greater equal (windows-arm64)"},
    {platform_bytecode::kOpcodeAnd,                "logical and (windows-arm64)"},
    {platform_bytecode::kOpcodeOr,                 "logical or (windows-arm64)"},
    {platform_bytecode::kOpcodeXor,                "logical xor (windows-arm64)"},
    {platform_bytecode::kOpcodeNand,               "logical nand (windows-arm64)"},
    {platform_bytecode::kOpcodeNor,                "logical nor (windows-arm64)"},
    {platform_bytecode::kOpcodeNot,                "logical not (windows-arm64)"},
    {platform_bytecode::kOpcodeJump,               "unconditional jump (windows-arm64)"},
    {platform_bytecode::kOpcodeJumpIfZero,         "jump if zero (windows-arm64)"},
    {platform_bytecode::kOpcodeCheckApproval,      "check approval gate (windows-arm64)"},
    {platform_bytecode::kOpcodeCheckLimits,        "check resource limits (windows-arm64)"},
    {platform_bytecode::kOpcodeCheckRole,          "check role (windows-arm64)"},
    {platform_bytecode::kOpcodeCheckRoleIndirect,  "check role via pointer (windows-arm64)"},
    {platform_bytecode::kOpcodeNullCheck,          "null function pointer check (windows-arm64)"},
    {platform_bytecode::kOpcodeAssert,             "proof obligation assert (windows-arm64)"},
    {platform_bytecode::kOpcodeCall,               "direct call (windows-arm64)"},
    {platform_bytecode::kOpcodeCallIndirect,       "indirect call (windows-arm64)"},
    {platform_bytecode::kOpcodeOutputInt,          "output integer (windows-arm64)"},
    {platform_bytecode::kOpcodeOutputChar,         "output character (windows-arm64)"},
    {platform_bytecode::kOpcodePrintLine,          "print line (windows-arm64)"},
    {platform_bytecode::kOpcodeInputInt,           "input integer (windows-arm64)"},
    {platform_bytecode::kOpcodeInputChar,          "input character (windows-arm64)"},
    {platform_bytecode::kOpcodeVerifyEcc,          "verify memory ECC (windows-arm64)"},
    {platform_bytecode::kOpcodeCopyMemory,         "copy memory (windows-arm64)"},
    {platform_bytecode::kOpcodeCompareMemory,      "compare memory (windows-arm64)"},
    {platform_bytecode::kOpcodeRole,               "role chain operation (windows-arm64)"},
    {platform_bytecode::kOpcodeOperatorInput,      "operator yes/no input (windows-arm64)"},
    {platform_bytecode::kOpcodeReturn,             "function return (windows-arm64)"},
    {platform_bytecode::kOpcodeHalt,               "halt VM (windows-arm64)"},
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

// windows-arm64 低 8 位编号范围: 0xD6-0xFF 和 0x01-0x18 (环绕)
bool isOwnPlatformOpcode(std::uint16_t id) {
    if (static_cast<std::uint8_t>(id >> 8) != TORTURE_PLATFORM_ID) {
        return false;
    }
    const auto local = static_cast<std::uint16_t>(id & 0xFFu);
    return (local >= 0xD6 && local <= 0xFF) || (local >= 0x01 && local <= 0x18);
}

}  // namespace torture::vm::platform_vm
