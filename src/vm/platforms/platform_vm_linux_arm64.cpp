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

// linux_arm64 平台调度表：66 个 opcode 条目。
// local opcode 范围 0xC7-0xFF 环绕到 0x01-0x09，与现有 platform_vm.cpp 中 LINUX_ARM64 分支的实现一致，
// 但作为独立文件不包含其他平台信息。
constexpr DispatchEntry kEntries[] = {
    {platform_bytecode::kOpcodeVerify,             "verify function fingerprint (linux-arm64)"},
    {platform_bytecode::kOpcodePushInteger,        "push integer constant (linux-arm64)"},
    {platform_bytecode::kOpcodePushString,         "push string constant (linux-arm64)"},
    {platform_bytecode::kOpcodePushNull,           "push null literal (linux-arm64)"},
    {platform_bytecode::kOpcodePushReference,      "push reference to local (linux-arm64)"},
    {platform_bytecode::kOpcodeDereference,        "dereference reference (linux-arm64)"},
    {platform_bytecode::kOpcodeFieldReference,     "compute field reference (linux-arm64)"},
    {platform_bytecode::kOpcodeLoad,               "load local value (linux-arm64)"},
    {platform_bytecode::kOpcodeStore,              "store local value (linux-arm64)"},
    {platform_bytecode::kOpcodeStoreDereference,   "store through reference (linux-arm64)"},
    {platform_bytecode::kOpcodeStorePointer,       "store function pointer (linux-arm64)"},
    {platform_bytecode::kOpcodePop,                "pop top of stack (linux-arm64)"},
    {platform_bytecode::kOpcodeAdd,                "integer add (linux-arm64)"},
    {platform_bytecode::kOpcodeAddTrap,            "integer add trap (linux-arm64)"},
    {platform_bytecode::kOpcodeAddWrap,            "integer add wrap (linux-arm64)"},
    {platform_bytecode::kOpcodeAddSaturate,        "integer add saturate (linux-arm64)"},
    {platform_bytecode::kOpcodeAddExtend,          "integer add extend (linux-arm64)"},
    {platform_bytecode::kOpcodeSub,                "integer sub (linux-arm64)"},
    {platform_bytecode::kOpcodeSubTrap,            "integer sub trap (linux-arm64)"},
    {platform_bytecode::kOpcodeSubWrap,            "integer sub wrap (linux-arm64)"},
    {platform_bytecode::kOpcodeSubSaturate,        "integer sub saturate (linux-arm64)"},
    {platform_bytecode::kOpcodeSubExtend,          "integer sub extend (linux-arm64)"},
    {platform_bytecode::kOpcodeMul,                "integer mul (linux-arm64)"},
    {platform_bytecode::kOpcodeMulTrap,            "integer mul trap (linux-arm64)"},
    {platform_bytecode::kOpcodeMulWrap,            "integer mul wrap (linux-arm64)"},
    {platform_bytecode::kOpcodeMulSaturate,        "integer mul saturate (linux-arm64)"},
    {platform_bytecode::kOpcodeMulExtend,          "integer mul extend (linux-arm64)"},
    {platform_bytecode::kOpcodeDiv,                "integer div (linux-arm64)"},
    {platform_bytecode::kOpcodeDivTrap,            "integer div trap (linux-arm64)"},
    {platform_bytecode::kOpcodeDivWrap,            "integer div wrap (linux-arm64)"},
    {platform_bytecode::kOpcodeDivSaturate,        "integer div saturate (linux-arm64)"},
    {platform_bytecode::kOpcodeDivExtend,          "integer div extend (linux-arm64)"},
    {platform_bytecode::kOpcodeEqual,              "compare equal (linux-arm64)"},
    {platform_bytecode::kOpcodeNotEqual,           "compare not equal (linux-arm64)"},
    {platform_bytecode::kOpcodeLess,               "compare less (linux-arm64)"},
    {platform_bytecode::kOpcodeLessEqual,          "compare less equal (linux-arm64)"},
    {platform_bytecode::kOpcodeGreater,            "compare greater (linux-arm64)"},
    {platform_bytecode::kOpcodeGreaterEqual,       "compare greater equal (linux-arm64)"},
    {platform_bytecode::kOpcodeAnd,                "logical and (linux-arm64)"},
    {platform_bytecode::kOpcodeOr,                 "logical or (linux-arm64)"},
    {platform_bytecode::kOpcodeXor,                "logical xor (linux-arm64)"},
    {platform_bytecode::kOpcodeNand,               "logical nand (linux-arm64)"},
    {platform_bytecode::kOpcodeNor,                "logical nor (linux-arm64)"},
    {platform_bytecode::kOpcodeNot,                "logical not (linux-arm64)"},
    {platform_bytecode::kOpcodeJump,               "unconditional jump (linux-arm64)"},
    {platform_bytecode::kOpcodeJumpIfZero,         "jump if zero (linux-arm64)"},
    {platform_bytecode::kOpcodeCheckApproval,      "check approval gate (linux-arm64)"},
    {platform_bytecode::kOpcodeCheckLimits,        "check resource limits (linux-arm64)"},
    {platform_bytecode::kOpcodeCheckRole,          "check role (linux-arm64)"},
    {platform_bytecode::kOpcodeCheckRoleIndirect,  "check role via pointer (linux-arm64)"},
    {platform_bytecode::kOpcodeNullCheck,          "null function pointer check (linux-arm64)"},
    {platform_bytecode::kOpcodeAssert,             "proof obligation assert (linux-arm64)"},
    {platform_bytecode::kOpcodeCall,               "direct call (linux-arm64)"},
    {platform_bytecode::kOpcodeCallIndirect,       "indirect call (linux-arm64)"},
    {platform_bytecode::kOpcodeOutputInt,          "output integer (linux-arm64)"},
    {platform_bytecode::kOpcodeOutputChar,         "output character (linux-arm64)"},
    {platform_bytecode::kOpcodePrintLine,          "print line (linux-arm64)"},
    {platform_bytecode::kOpcodeInputInt,           "input integer (linux-arm64)"},
    {platform_bytecode::kOpcodeInputChar,          "input character (linux-arm64)"},
    {platform_bytecode::kOpcodeVerifyEcc,          "verify memory ECC (linux-arm64)"},
    {platform_bytecode::kOpcodeCopyMemory,         "copy memory (linux-arm64)"},
    {platform_bytecode::kOpcodeCompareMemory,      "compare memory (linux-arm64)"},
    {platform_bytecode::kOpcodeRole,               "role chain operation (linux-arm64)"},
    {platform_bytecode::kOpcodeOperatorInput,      "operator yes/no input (linux-arm64)"},
    {platform_bytecode::kOpcodeReturn,             "function return (linux-arm64)"},
    {platform_bytecode::kOpcodeHalt,               "halt VM (linux-arm64)"},
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

// linux_arm64 平台的 local opcode 范围是 0xC7-0xFF 环绕到 0x01-0x09，
// 不使用硬编码的 0x01-0x42 范围。
bool isOwnPlatformOpcode(std::uint16_t id) {
    if (static_cast<std::uint8_t>(id >> 8) != TORTURE_PLATFORM_ID) {
        return false;
    }
    const auto local = static_cast<std::uint16_t>(id & 0xFFu);
    // 0xC7-0xFF 范围（前 57 个 opcode）
    // 0x01-0x09 范围（后 9 个 opcode，环绕）
    return (local >= 0xC7) || (local >= 0x01 && local <= 0x09);
}

}  // namespace torture::vm::platform_vm
