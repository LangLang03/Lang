#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "vm/platform.h"

namespace torture::vm::platform_bytecode {

// linux_arm32 平台魔数
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'L', '3', 'A'};

// 平台后缀
inline constexpr std::string_view kPlatformSuffix = "LA32";

// 字节码格式版本
inline constexpr std::uint16_t kVersion = 0x0102;

// 平台 flags
inline constexpr std::uint16_t kFlags = 0x0003;

// 66 个独立低 8 位编号（0x85-0xC6 范围，连续递增，平台内无重复）
inline constexpr std::uint8_t kLocalOpcodeVerify = 0x85;
inline constexpr std::uint8_t kLocalOpcodePushInteger = 0x86;
inline constexpr std::uint8_t kLocalOpcodePushString = 0x87;
inline constexpr std::uint8_t kLocalOpcodePushNull = 0x88;
inline constexpr std::uint8_t kLocalOpcodePushReference = 0x89;
inline constexpr std::uint8_t kLocalOpcodeDereference = 0x8A;
inline constexpr std::uint8_t kLocalOpcodeFieldReference = 0x8B;
inline constexpr std::uint8_t kLocalOpcodeLoad = 0x8C;
inline constexpr std::uint8_t kLocalOpcodeStore = 0x8D;
inline constexpr std::uint8_t kLocalOpcodeStoreDereference = 0x8E;
inline constexpr std::uint8_t kLocalOpcodeStorePointer = 0x8F;
inline constexpr std::uint8_t kLocalOpcodePop = 0x90;
inline constexpr std::uint8_t kLocalOpcodeAdd = 0x91;
inline constexpr std::uint8_t kLocalOpcodeAddTrap = 0x92;
inline constexpr std::uint8_t kLocalOpcodeAddWrap = 0x93;
inline constexpr std::uint8_t kLocalOpcodeAddSaturate = 0x94;
inline constexpr std::uint8_t kLocalOpcodeAddExtend = 0x95;
inline constexpr std::uint8_t kLocalOpcodeSub = 0x96;
inline constexpr std::uint8_t kLocalOpcodeSubTrap = 0x97;
inline constexpr std::uint8_t kLocalOpcodeSubWrap = 0x98;
inline constexpr std::uint8_t kLocalOpcodeSubSaturate = 0x99;
inline constexpr std::uint8_t kLocalOpcodeSubExtend = 0x9A;
inline constexpr std::uint8_t kLocalOpcodeMul = 0x9B;
inline constexpr std::uint8_t kLocalOpcodeMulTrap = 0x9C;
inline constexpr std::uint8_t kLocalOpcodeMulWrap = 0x9D;
inline constexpr std::uint8_t kLocalOpcodeMulSaturate = 0x9E;
inline constexpr std::uint8_t kLocalOpcodeMulExtend = 0x9F;
inline constexpr std::uint8_t kLocalOpcodeDiv = 0xA0;
inline constexpr std::uint8_t kLocalOpcodeDivTrap = 0xA1;
inline constexpr std::uint8_t kLocalOpcodeDivWrap = 0xA2;
inline constexpr std::uint8_t kLocalOpcodeDivSaturate = 0xA3;
inline constexpr std::uint8_t kLocalOpcodeDivExtend = 0xA4;
inline constexpr std::uint8_t kLocalOpcodeEqual = 0xA5;
inline constexpr std::uint8_t kLocalOpcodeNotEqual = 0xA6;
inline constexpr std::uint8_t kLocalOpcodeLess = 0xA7;
inline constexpr std::uint8_t kLocalOpcodeLessEqual = 0xA8;
inline constexpr std::uint8_t kLocalOpcodeGreater = 0xA9;
inline constexpr std::uint8_t kLocalOpcodeGreaterEqual = 0xAA;
inline constexpr std::uint8_t kLocalOpcodeAnd = 0xAB;
inline constexpr std::uint8_t kLocalOpcodeOr = 0xAC;
inline constexpr std::uint8_t kLocalOpcodeXor = 0xAD;
inline constexpr std::uint8_t kLocalOpcodeNand = 0xAE;
inline constexpr std::uint8_t kLocalOpcodeNor = 0xAF;
inline constexpr std::uint8_t kLocalOpcodeNot = 0xB0;
inline constexpr std::uint8_t kLocalOpcodeJump = 0xB1;
inline constexpr std::uint8_t kLocalOpcodeJumpIfZero = 0xB2;
inline constexpr std::uint8_t kLocalOpcodeCheckApproval = 0xB3;
inline constexpr std::uint8_t kLocalOpcodeCheckLimits = 0xB4;
inline constexpr std::uint8_t kLocalOpcodeCheckRole = 0xB5;
inline constexpr std::uint8_t kLocalOpcodeCheckRoleIndirect = 0xB6;
inline constexpr std::uint8_t kLocalOpcodeNullCheck = 0xB7;
inline constexpr std::uint8_t kLocalOpcodeAssert = 0xB8;
inline constexpr std::uint8_t kLocalOpcodeCall = 0xB9;
inline constexpr std::uint8_t kLocalOpcodeCallIndirect = 0xBA;
inline constexpr std::uint8_t kLocalOpcodeOutputInt = 0xBB;
inline constexpr std::uint8_t kLocalOpcodeOutputChar = 0xBC;
inline constexpr std::uint8_t kLocalOpcodePrintLine = 0xBD;
inline constexpr std::uint8_t kLocalOpcodeInputInt = 0xBE;
inline constexpr std::uint8_t kLocalOpcodeInputChar = 0xBF;
inline constexpr std::uint8_t kLocalOpcodeVerifyEcc = 0xC0;
inline constexpr std::uint8_t kLocalOpcodeCopyMemory = 0xC1;
inline constexpr std::uint8_t kLocalOpcodeCompareMemory = 0xC2;
inline constexpr std::uint8_t kLocalOpcodeRole = 0xC3;
inline constexpr std::uint8_t kLocalOpcodeOperatorInput = 0xC4;
inline constexpr std::uint8_t kLocalOpcodeReturn = 0xC5;
inline constexpr std::uint8_t kLocalOpcodeHalt = 0xC6;
inline constexpr std::uint8_t kLocalOpcodeApply = 0xC7;

// 66 个平台特化 opcode 值 = (TORTURE_PLATFORM_ID << 8) | kLocalOpcode*
inline constexpr std::uint16_t kOpcodeVerify =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeVerify;
inline constexpr std::uint16_t kOpcodePushInteger =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePushInteger;
inline constexpr std::uint16_t kOpcodePushString =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePushString;
inline constexpr std::uint16_t kOpcodePushNull =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePushNull;
inline constexpr std::uint16_t kOpcodePushReference =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePushReference;
inline constexpr std::uint16_t kOpcodeDereference =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDereference;
inline constexpr std::uint16_t kOpcodeFieldReference =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeFieldReference;
inline constexpr std::uint16_t kOpcodeLoad =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeLoad;
inline constexpr std::uint16_t kOpcodeStore =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeStore;
inline constexpr std::uint16_t kOpcodeStoreDereference =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeStoreDereference;
inline constexpr std::uint16_t kOpcodeStorePointer =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeStorePointer;
inline constexpr std::uint16_t kOpcodePop =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePop;
inline constexpr std::uint16_t kOpcodeAdd =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAdd;
inline constexpr std::uint16_t kOpcodeAddTrap =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAddTrap;
inline constexpr std::uint16_t kOpcodeAddWrap =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAddWrap;
inline constexpr std::uint16_t kOpcodeAddSaturate =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAddSaturate;
inline constexpr std::uint16_t kOpcodeAddExtend =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAddExtend;
inline constexpr std::uint16_t kOpcodeSub =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeSub;
inline constexpr std::uint16_t kOpcodeSubTrap =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeSubTrap;
inline constexpr std::uint16_t kOpcodeSubWrap =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeSubWrap;
inline constexpr std::uint16_t kOpcodeSubSaturate =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeSubSaturate;
inline constexpr std::uint16_t kOpcodeSubExtend =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeSubExtend;
inline constexpr std::uint16_t kOpcodeMul =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeMul;
inline constexpr std::uint16_t kOpcodeMulTrap =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeMulTrap;
inline constexpr std::uint16_t kOpcodeMulWrap =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeMulWrap;
inline constexpr std::uint16_t kOpcodeMulSaturate =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeMulSaturate;
inline constexpr std::uint16_t kOpcodeMulExtend =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeMulExtend;
inline constexpr std::uint16_t kOpcodeDiv =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDiv;
inline constexpr std::uint16_t kOpcodeDivTrap =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDivTrap;
inline constexpr std::uint16_t kOpcodeDivWrap =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDivWrap;
inline constexpr std::uint16_t kOpcodeDivSaturate =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDivSaturate;
inline constexpr std::uint16_t kOpcodeDivExtend =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDivExtend;
inline constexpr std::uint16_t kOpcodeEqual =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeEqual;
inline constexpr std::uint16_t kOpcodeNotEqual =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeNotEqual;
inline constexpr std::uint16_t kOpcodeLess =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeLess;
inline constexpr std::uint16_t kOpcodeLessEqual =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeLessEqual;
inline constexpr std::uint16_t kOpcodeGreater =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeGreater;
inline constexpr std::uint16_t kOpcodeGreaterEqual =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeGreaterEqual;
inline constexpr std::uint16_t kOpcodeAnd =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAnd;
inline constexpr std::uint16_t kOpcodeOr =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeOr;
inline constexpr std::uint16_t kOpcodeXor =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeXor;
inline constexpr std::uint16_t kOpcodeNand =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeNand;
inline constexpr std::uint16_t kOpcodeNor =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeNor;
inline constexpr std::uint16_t kOpcodeNot =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeNot;
inline constexpr std::uint16_t kOpcodeJump =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeJump;
inline constexpr std::uint16_t kOpcodeJumpIfZero =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeJumpIfZero;
inline constexpr std::uint16_t kOpcodeCheckApproval =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCheckApproval;
inline constexpr std::uint16_t kOpcodeCheckLimits =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCheckLimits;
inline constexpr std::uint16_t kOpcodeCheckRole =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCheckRole;
inline constexpr std::uint16_t kOpcodeCheckRoleIndirect =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCheckRoleIndirect;
inline constexpr std::uint16_t kOpcodeNullCheck =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeNullCheck;
inline constexpr std::uint16_t kOpcodeAssert =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAssert;
inline constexpr std::uint16_t kOpcodeCall =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCall;
inline constexpr std::uint16_t kOpcodeCallIndirect =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCallIndirect;
inline constexpr std::uint16_t kOpcodeOutputInt =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeOutputInt;
inline constexpr std::uint16_t kOpcodeOutputChar =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeOutputChar;
inline constexpr std::uint16_t kOpcodePrintLine =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePrintLine;
inline constexpr std::uint16_t kOpcodeInputInt =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeInputInt;
inline constexpr std::uint16_t kOpcodeInputChar =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeInputChar;
inline constexpr std::uint16_t kOpcodeVerifyEcc =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeVerifyEcc;
inline constexpr std::uint16_t kOpcodeCopyMemory =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCopyMemory;
inline constexpr std::uint16_t kOpcodeCompareMemory =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCompareMemory;
inline constexpr std::uint16_t kOpcodeRole =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeRole;
inline constexpr std::uint16_t kOpcodeOperatorInput =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeOperatorInput;
inline constexpr std::uint16_t kOpcodeReturn =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeReturn;
inline constexpr std::uint16_t kOpcodeHalt =
    (static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeHalt;
inline constexpr std::uint16_t kOpcodeApply = static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeApply);

static_assert(kLocalOpcodeVerify == 0x85, "linux_arm32 local opcode range must start at 0x85");
static_assert(kLocalOpcodeHalt == 0xC6, "linux_arm32 local opcode range must end at 0xC6");

}  // namespace torture::vm::platform_bytecode

// 平台后缀字面量宏，供 X-macro 拼接使用
#define TORTURE_PLATFORM_SUFFIX_LITERAL "LA32"
