#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace torture::vm {

enum class Opcode : std::uint16_t {
  kVerify = 1,
  kPushInteger,
  kPushString,
  kPushNull,
  kPushReference,
  kDereference,
  kFieldReference,
  kLoad,
  kStore,
  kStoreDereference,
  kStorePointer,
  kPop,
  kAdd,
  kAddTrap,
  kAddWrap,
  kAddSaturate,
  kAddExtend,
  kSub,
  kSubTrap,
  kSubWrap,
  kSubSaturate,
  kSubExtend,
  kMul,
  kMulTrap,
  kMulWrap,
  kMulSaturate,
  kMulExtend,
  kDiv,
  kDivTrap,
  kDivWrap,
  kDivSaturate,
  kDivExtend,
  kEqual,
  kNotEqual,
  kLess,
  kLessEqual,
  kGreater,
  kGreaterEqual,
  kAnd,
  kOr,
  kXor,
  kNand,
  kNor,
  kNot,
  kJump,
  kJumpIfZero,
  kCheckApproval,
  kCheckLimits,
  kCheckRole,
  kCheckRoleIndirect,
  kNullCheck,
  kAssert,
  kCall,
  kCallIndirect,
  kOutputInt,
  kOutputChar,
  kPrintLine,
  kInputInt,
  kInputChar,
  kVerifyEcc,
  kCopyMemory,
  kCompareMemory,
  kRole,
  kOperatorInput,
  kReturn,
  kHalt,
};

struct Instruction {
  std::string op;
  std::vector<std::string> args;
};

std::string_view opcodeName(Opcode opcode);
std::optional<Opcode> opcodeFromName(std::string_view name);
Instruction makeInstruction(Opcode opcode, std::vector<std::string> args = {});

}  // namespace torture::vm
