#include "vm/opcode.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace torture::vm {
namespace {

struct OpcodeDescriptor {
  Opcode opcode;
  std::string_view name;
};

constexpr auto kOpcodeDescriptors = std::to_array<OpcodeDescriptor>({
    {Opcode::kVerify, "VERIFY"},
    {Opcode::kPushInteger, "PUSHI"},
    {Opcode::kPushString, "PUSHS"},
    {Opcode::kPushNull, "PUSHNULL"},
    {Opcode::kPushReference, "PUSHREF"},
    {Opcode::kDereference, "DEREF"},
    {Opcode::kFieldReference, "FIELDREF"},
    {Opcode::kLoad, "LOAD"},
    {Opcode::kStore, "STORE"},
    {Opcode::kStoreDereference, "STORE_DEREF"},
    {Opcode::kStorePointer, "STOREPTR"},
    {Opcode::kPop, "POP"},
    {Opcode::kAdd, "ADD"},
    {Opcode::kAddTrap, "ADD_trap"},
    {Opcode::kAddWrap, "ADD_wrap"},
    {Opcode::kAddSaturate, "ADD_saturate"},
    {Opcode::kAddExtend, "ADD_extend"},
    {Opcode::kSub, "SUB"},
    {Opcode::kSubTrap, "SUB_trap"},
    {Opcode::kSubWrap, "SUB_wrap"},
    {Opcode::kSubSaturate, "SUB_saturate"},
    {Opcode::kSubExtend, "SUB_extend"},
    {Opcode::kMul, "MUL"},
    {Opcode::kMulTrap, "MUL_trap"},
    {Opcode::kMulWrap, "MUL_wrap"},
    {Opcode::kMulSaturate, "MUL_saturate"},
    {Opcode::kMulExtend, "MUL_extend"},
    {Opcode::kDiv, "DIV"},
    {Opcode::kDivTrap, "DIV_trap"},
    {Opcode::kDivWrap, "DIV_wrap"},
    {Opcode::kDivSaturate, "DIV_saturate"},
    {Opcode::kDivExtend, "DIV_extend"},
    {Opcode::kEqual, "EQ"},
    {Opcode::kNotEqual, "NE"},
    {Opcode::kLess, "LT"},
    {Opcode::kLessEqual, "LE"},
    {Opcode::kGreater, "GT"},
    {Opcode::kGreaterEqual, "GE"},
    {Opcode::kAnd, "AND"},
    {Opcode::kOr, "OR"},
    {Opcode::kXor, "XOR"},
    {Opcode::kNand, "NAND"},
    {Opcode::kNor, "NOR"},
    {Opcode::kNot, "NOT"},
    {Opcode::kJump, "JMP"},
    {Opcode::kJumpIfZero, "JZ"},
    {Opcode::kCheckApproval, "CHECKAPPROVAL"},
    {Opcode::kCheckLimits, "CHECKLIMITS"},
    {Opcode::kCheckRole, "CHECKROLE"},
    {Opcode::kCheckRoleIndirect, "CHECKROLEIND"},
    {Opcode::kNullCheck, "NULLCHECK"},
    {Opcode::kAssert, "ASSERT"},
    {Opcode::kCall, "CALL"},
    {Opcode::kCallIndirect, "CALLIND"},
    {Opcode::kOutputInt, "OUTPUTINT"},
    {Opcode::kOutputChar, "OUTPUTCHAR"},
    {Opcode::kPrintLine, "PRINTLN"},
    {Opcode::kInputInt, "INPUTINT"},
    {Opcode::kInputChar, "INPUTCHAR"},
    {Opcode::kVerifyEcc, "VERIFYECC"},
    {Opcode::kCopyMemory, "COPYMEMORY"},
    {Opcode::kCompareMemory, "COMPAREMEMORY"},
    {Opcode::kRole, "ROLE"},
    {Opcode::kOperatorInput, "OPINPUT"},
    {Opcode::kReturn, "RET"},
    {Opcode::kHalt, "HALT"},
});

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
  for (const auto& descriptor : kOpcodeDescriptors) {
    if (descriptor.name == name) {
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
