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

// linux_arm32 平台调度表：66 个 opcode 条目。
// local opcode 范围 0x85-0xC6，与现有 platform_vm.cpp 中 LINUX_ARM32 分支的实现一致，
// 但作为独立文件不包含其他平台信息。
constexpr DispatchEntry kEntries[] = {
    {platform_bytecode::kOpcodeVerify,             "verify function fingerprint (linux-arm32)"},
    {platform_bytecode::kOpcodePushInteger,        "push integer constant (linux-arm32)"},
    {platform_bytecode::kOpcodePushString,         "push string constant (linux-arm32)"},
    {platform_bytecode::kOpcodePushNull,           "push null literal (linux-arm32)"},
    {platform_bytecode::kOpcodePushReference,      "push reference to local (linux-arm32)"},
    {platform_bytecode::kOpcodeDereference,        "dereference reference (linux-arm32)"},
    {platform_bytecode::kOpcodeFieldReference,     "compute field reference (linux-arm32)"},
    {platform_bytecode::kOpcodeLoad,               "load local value (linux-arm32)"},
    {platform_bytecode::kOpcodeStore,              "store local value (linux-arm32)"},
    {platform_bytecode::kOpcodeStoreDereference,   "store through reference (linux-arm32)"},
    {platform_bytecode::kOpcodeStorePointer,       "store function pointer (linux-arm32)"},
    {platform_bytecode::kOpcodePop,                "pop top of stack (linux-arm32)"},
    {platform_bytecode::kOpcodeAdd,                "integer add (linux-arm32)"},
    {platform_bytecode::kOpcodeAddTrap,            "integer add trap (linux-arm32)"},
    {platform_bytecode::kOpcodeAddWrap,            "integer add wrap (linux-arm32)"},
    {platform_bytecode::kOpcodeAddSaturate,        "integer add saturate (linux-arm32)"},
    {platform_bytecode::kOpcodeAddExtend,          "integer add extend (linux-arm32)"},
    {platform_bytecode::kOpcodeSub,                "integer sub (linux-arm32)"},
    {platform_bytecode::kOpcodeSubTrap,            "integer sub trap (linux-arm32)"},
    {platform_bytecode::kOpcodeSubWrap,            "integer sub wrap (linux-arm32)"},
    {platform_bytecode::kOpcodeSubSaturate,        "integer sub saturate (linux-arm32)"},
    {platform_bytecode::kOpcodeSubExtend,          "integer sub extend (linux-arm32)"},
    {platform_bytecode::kOpcodeMul,                "integer mul (linux-arm32)"},
    {platform_bytecode::kOpcodeMulTrap,            "integer mul trap (linux-arm32)"},
    {platform_bytecode::kOpcodeMulWrap,            "integer mul wrap (linux-arm32)"},
    {platform_bytecode::kOpcodeMulSaturate,        "integer mul saturate (linux-arm32)"},
    {platform_bytecode::kOpcodeMulExtend,          "integer mul extend (linux-arm32)"},
    {platform_bytecode::kOpcodeDiv,                "integer div (linux-arm32)"},
    {platform_bytecode::kOpcodeDivTrap,            "integer div trap (linux-arm32)"},
    {platform_bytecode::kOpcodeDivWrap,            "integer div wrap (linux-arm32)"},
    {platform_bytecode::kOpcodeDivSaturate,        "integer div saturate (linux-arm32)"},
    {platform_bytecode::kOpcodeDivExtend,          "integer div extend (linux-arm32)"},
    {platform_bytecode::kOpcodeEqual,              "compare equal (linux-arm32)"},
    {platform_bytecode::kOpcodeNotEqual,           "compare not equal (linux-arm32)"},
    {platform_bytecode::kOpcodeLess,               "compare less (linux-arm32)"},
    {platform_bytecode::kOpcodeLessEqual,          "compare less equal (linux-arm32)"},
    {platform_bytecode::kOpcodeGreater,            "compare greater (linux-arm32)"},
    {platform_bytecode::kOpcodeGreaterEqual,       "compare greater equal (linux-arm32)"},
    {platform_bytecode::kOpcodeAnd,                "logical and (linux-arm32)"},
    {platform_bytecode::kOpcodeOr,                 "logical or (linux-arm32)"},
    {platform_bytecode::kOpcodeXor,                "logical xor (linux-arm32)"},
    {platform_bytecode::kOpcodeNand,               "logical nand (linux-arm32)"},
    {platform_bytecode::kOpcodeNor,                "logical nor (linux-arm32)"},
    {platform_bytecode::kOpcodeNot,                "logical not (linux-arm32)"},
    {platform_bytecode::kOpcodeJump,               "unconditional jump (linux-arm32)"},
    {platform_bytecode::kOpcodeJumpIfZero,         "jump if zero (linux-arm32)"},
    {platform_bytecode::kOpcodeCheckApproval,      "check approval gate (linux-arm32)"},
    {platform_bytecode::kOpcodeCheckLimits,        "check resource limits (linux-arm32)"},
    {platform_bytecode::kOpcodeCheckRole,          "check role (linux-arm32)"},
    {platform_bytecode::kOpcodeCheckRoleIndirect,  "check role via pointer (linux-arm32)"},
    {platform_bytecode::kOpcodeNullCheck,          "null function pointer check (linux-arm32)"},
    {platform_bytecode::kOpcodeAssert,             "proof obligation assert (linux-arm32)"},
    {platform_bytecode::kOpcodeCall,               "direct call (linux-arm32)"},
    {platform_bytecode::kOpcodeCallIndirect,       "indirect call (linux-arm32)"},
    {platform_bytecode::kOpcodeOutputInt,          "output integer (linux-arm32)"},
    {platform_bytecode::kOpcodeOutputChar,         "output character (linux-arm32)"},
    {platform_bytecode::kOpcodePrintLine,          "print line (linux-arm32)"},
    {platform_bytecode::kOpcodeInputInt,           "input integer (linux-arm32)"},
    {platform_bytecode::kOpcodeInputChar,          "input character (linux-arm32)"},
    {platform_bytecode::kOpcodeVerifyEcc,          "verify memory ECC (linux-arm32)"},
    {platform_bytecode::kOpcodeCopyMemory,         "copy memory (linux-arm32)"},
    {platform_bytecode::kOpcodeCompareMemory,      "compare memory (linux-arm32)"},
    {platform_bytecode::kOpcodeRole,               "role chain operation (linux-arm32)"},
    {platform_bytecode::kOpcodeOperatorInput,      "operator yes/no input (linux-arm32)"},
    {platform_bytecode::kOpcodeReturn,             "function return (linux-arm32)"},
    {platform_bytecode::kOpcodeHalt,               "halt VM (linux-arm32)"},
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

// linux_arm32 平台的 local opcode 范围是 0x85-0xC6（连续递增），
// 不使用硬编码的 0x01-0x42 范围。
bool isOwnPlatformOpcode(std::uint16_t id) {
    if (static_cast<std::uint8_t>(id >> 8) != TORTURE_PLATFORM_ID) {
        return false;
    }
    const auto local = static_cast<std::uint16_t>(id & 0xFFu);
    return local >= 0x85 && local <= 0xC6;
}

}  // namespace torture::vm::platform_vm
