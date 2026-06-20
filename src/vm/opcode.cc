#include "vm/opcode.h"

#include "vm/platform_bytecode.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace torture::vm {
namespace {

struct OpcodeDescriptor {
  Opcode opcode;
  std::string_view name;  // 带当前平台后缀的全名
};

constexpr std::array<OpcodeDescriptor, 66> kOpcodeDescriptors = {{
    {Opcode::kVerify,             "VERIFY_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kPushInteger,        "PUSHI_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kPushString,         "PUSHS_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kPushNull,           "PUSHNULL_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kPushReference,      "PUSHREF_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kDereference,        "DEREF_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kFieldReference,     "FIELDREF_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kLoad,               "LOAD_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kStore,              "STORE_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kStoreDereference,   "STORE_DEREF_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kStorePointer,       "STOREPTR_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kPop,                "POP_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kAdd,                "ADD_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kAddTrap,            "ADD_trap_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kAddWrap,            "ADD_wrap_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kAddSaturate,        "ADD_saturate_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kAddExtend,          "ADD_extend_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kSub,                "SUB_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kSubTrap,            "SUB_trap_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kSubWrap,            "SUB_wrap_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kSubSaturate,        "SUB_saturate_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kSubExtend,          "SUB_extend_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kMul,                "MUL_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kMulTrap,            "MUL_trap_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kMulWrap,            "MUL_wrap_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kMulSaturate,        "MUL_saturate_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kMulExtend,          "MUL_extend_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kDiv,                "DIV_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kDivTrap,            "DIV_trap_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kDivWrap,            "DIV_wrap_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kDivSaturate,        "DIV_saturate_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kDivExtend,          "DIV_extend_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kEqual,              "EQ_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kNotEqual,           "NE_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kLess,               "LT_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kLessEqual,          "LE_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kGreater,            "GT_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kGreaterEqual,       "GE_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kAnd,                "AND_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kOr,                 "OR_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kXor,                "XOR_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kNand,               "NAND_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kNor,                "NOR_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kNot,                "NOT_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kJump,               "JMP_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kJumpIfZero,         "JZ_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kCheckApproval,      "CHECKAPPROVAL_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kCheckLimits,        "CHECKLIMITS_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kCheckRole,          "CHECKROLE_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kCheckRoleIndirect,  "CHECKROLEIND_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kNullCheck,          "NULLCHECK_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kAssert,             "ASSERT_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kCall,               "CALL_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kCallIndirect,       "CALLIND_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kOutputInt,          "OUTPUTINT_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kOutputChar,         "OUTPUTCHAR_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kPrintLine,          "PRINTLN_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kInputInt,           "INPUTINT_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kInputChar,          "INPUTCHAR_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kVerifyEcc,          "VERIFYECC_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kCopyMemory,         "COPYMEMORY_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kCompareMemory,      "COMPAREMEMORY_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kRole,               "ROLE_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kOperatorInput,      "OPINPUT_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kReturn,             "RET_" TORTURE_PLATFORM_SUFFIX_LITERAL},
    {Opcode::kHalt,               "HALT_" TORTURE_PLATFORM_SUFFIX_LITERAL},
}};

}  // namespace

std::string_view opcodeName(Opcode opcode) {
  for (const auto& descriptor : kOpcodeDescriptors) {
    if (descriptor.opcode == opcode) {
      return descriptor.name;
    }
  }
  return {};
}

std::optional<Opcode> opcodeFromName(std::string_view name) {
  // 1) 优先匹配带后缀的新名
  for (const auto& descriptor : kOpcodeDescriptors) {
    if (descriptor.name == name) {
      return descriptor.opcode;
    }
  }
  // 2) 否则尝试把旧名（不带后缀）映射到当前平台
  for (const auto& descriptor : kOpcodeDescriptors) {
    const auto sep = descriptor.name.find('_');
    if (sep != std::string_view::npos && descriptor.name.substr(0, sep) == name) {
      return descriptor.opcode;
    }
  }
  return std::nullopt;
}

Instruction makeInstruction(Opcode opcode, std::vector<std::string> args) {
  const auto name = opcodeName(opcode);
  if (name.empty()) {
    throw std::runtime_error("cannot construct instruction for unknown opcode");
  }
  return Instruction{std::string{name}, std::move(args)};
}

}  // namespace torture::vm
