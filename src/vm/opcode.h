#pragma once

#include "vm/platform_bytecode.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace torture::vm {

// opcode 整数值 = (TORTURE_PLATFORM_ID << 8) | 局部编号。
// 由 platform_bytecode 提供的 constexpr 变量赋值，保证 12 平台枚举值两两不同。
enum class Opcode : std::uint16_t {
  kVerify = platform_bytecode::kOpcodeVerify,
  kPushInteger = platform_bytecode::kOpcodePushInteger,
  kPushString = platform_bytecode::kOpcodePushString,
  kPushNull = platform_bytecode::kOpcodePushNull,
  kPushReference = platform_bytecode::kOpcodePushReference,
  kDereference = platform_bytecode::kOpcodeDereference,
  kFieldReference = platform_bytecode::kOpcodeFieldReference,
  kLoad = platform_bytecode::kOpcodeLoad,
  kStore = platform_bytecode::kOpcodeStore,
  kStoreDereference = platform_bytecode::kOpcodeStoreDereference,
  kStorePointer = platform_bytecode::kOpcodeStorePointer,
  kPop = platform_bytecode::kOpcodePop,
  kAdd = platform_bytecode::kOpcodeAdd,
  kAddTrap = platform_bytecode::kOpcodeAddTrap,
  kAddWrap = platform_bytecode::kOpcodeAddWrap,
  kAddSaturate = platform_bytecode::kOpcodeAddSaturate,
  kAddExtend = platform_bytecode::kOpcodeAddExtend,
  kSub = platform_bytecode::kOpcodeSub,
  kSubTrap = platform_bytecode::kOpcodeSubTrap,
  kSubWrap = platform_bytecode::kOpcodeSubWrap,
  kSubSaturate = platform_bytecode::kOpcodeSubSaturate,
  kSubExtend = platform_bytecode::kOpcodeSubExtend,
  kMul = platform_bytecode::kOpcodeMul,
  kMulTrap = platform_bytecode::kOpcodeMulTrap,
  kMulWrap = platform_bytecode::kOpcodeMulWrap,
  kMulSaturate = platform_bytecode::kOpcodeMulSaturate,
  kMulExtend = platform_bytecode::kOpcodeMulExtend,
  kDiv = platform_bytecode::kOpcodeDiv,
  kDivTrap = platform_bytecode::kOpcodeDivTrap,
  kDivWrap = platform_bytecode::kOpcodeDivWrap,
  kDivSaturate = platform_bytecode::kOpcodeDivSaturate,
  kDivExtend = platform_bytecode::kOpcodeDivExtend,
  kEqual = platform_bytecode::kOpcodeEqual,
  kNotEqual = platform_bytecode::kOpcodeNotEqual,
  kLess = platform_bytecode::kOpcodeLess,
  kLessEqual = platform_bytecode::kOpcodeLessEqual,
  kGreater = platform_bytecode::kOpcodeGreater,
  kGreaterEqual = platform_bytecode::kOpcodeGreaterEqual,
  kAnd = platform_bytecode::kOpcodeAnd,
  kOr = platform_bytecode::kOpcodeOr,
  kXor = platform_bytecode::kOpcodeXor,
  kNand = platform_bytecode::kOpcodeNand,
  kNor = platform_bytecode::kOpcodeNor,
  kNot = platform_bytecode::kOpcodeNot,
  kJump = platform_bytecode::kOpcodeJump,
  kJumpIfZero = platform_bytecode::kOpcodeJumpIfZero,
  kCheckApproval = platform_bytecode::kOpcodeCheckApproval,
  kCheckLimits = platform_bytecode::kOpcodeCheckLimits,
  kCheckRole = platform_bytecode::kOpcodeCheckRole,
  kCheckRoleIndirect = platform_bytecode::kOpcodeCheckRoleIndirect,
  kNullCheck = platform_bytecode::kOpcodeNullCheck,
  kAssert = platform_bytecode::kOpcodeAssert,
  kCall = platform_bytecode::kOpcodeCall,
  kCallIndirect = platform_bytecode::kOpcodeCallIndirect,
  kOutputInt = platform_bytecode::kOpcodeOutputInt,
  kOutputChar = platform_bytecode::kOpcodeOutputChar,
  kPrintLine = platform_bytecode::kOpcodePrintLine,
  kInputInt = platform_bytecode::kOpcodeInputInt,
  kInputChar = platform_bytecode::kOpcodeInputChar,
  kVerifyEcc = platform_bytecode::kOpcodeVerifyEcc,
  kCopyMemory = platform_bytecode::kOpcodeCopyMemory,
  kCompareMemory = platform_bytecode::kOpcodeCompareMemory,
  kRole = platform_bytecode::kOpcodeRole,
  kOperatorInput = platform_bytecode::kOpcodeOperatorInput,
  kReturn = platform_bytecode::kOpcodeReturn,
  kHalt = platform_bytecode::kOpcodeHalt,
  kApply = platform_bytecode::kOpcodeApply,
};

struct Instruction {
  Opcode op = Opcode::kHalt;
  std::vector<std::string> args;
};

// 返回带当前平台后缀的全名（如 "ADD_LX64"）；未知 opcode 返回空。
std::string_view opcodeName(Opcode opcode);

// 接受带后缀的新名（"ADD_LX64"）或旧别名（"ADD"，视作当前平台）。
std::optional<Opcode> opcodeFromName(std::string_view name);

// 用枚举构造一个 Instruction。
Instruction makeInstruction(Opcode opcode, std::vector<std::string> args = {});

}  // namespace torture::vm
