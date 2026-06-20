#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "vm/platform.h"

// android_arm64 平台特化字节码定义。
// 平台 ID: 0x0C (TORTURE_PLATFORM_ANDROID_ARM64)
// 魔数: {'T','A','A','8'}
// 后缀: "AA64"
// 版本: 0x0303
// flags: 0x000C
// 低 8 位编号范围: 0xDE-0xFF (34 个) + 0x01-0x20 (32 个) = 66 个值 (环绕)

namespace torture::vm::platform_bytecode {

inline constexpr std::array<char, 4> kMagicBytes = {'T', 'A', 'A', '8'};
inline constexpr std::string_view kPlatformSuffix = "AA64";
inline constexpr std::uint16_t kVersion = 0x0303;
inline constexpr std::uint16_t kFlags = 0x000C;

}  // namespace torture::vm::platform_bytecode

#define TORTURE_PLATFORM_SUFFIX_LITERAL "AA64"

namespace torture::vm::platform_bytecode {

// 本平台 66 个 opcode 的低 8 位编号 (0xDE-0xFF, 然后环绕到 0x01-0x20)。
inline constexpr std::uint8_t kLocalOpcodeVerify = 0xDE;
inline constexpr std::uint8_t kLocalOpcodePushInteger = 0xDF;
inline constexpr std::uint8_t kLocalOpcodePushString = 0xE0;
inline constexpr std::uint8_t kLocalOpcodePushNull = 0xE1;
inline constexpr std::uint8_t kLocalOpcodePushReference = 0xE2;
inline constexpr std::uint8_t kLocalOpcodeDereference = 0xE3;
inline constexpr std::uint8_t kLocalOpcodeFieldReference = 0xE4;
inline constexpr std::uint8_t kLocalOpcodeLoad = 0xE5;
inline constexpr std::uint8_t kLocalOpcodeStore = 0xE6;
inline constexpr std::uint8_t kLocalOpcodeStoreDereference = 0xE7;
inline constexpr std::uint8_t kLocalOpcodeStorePointer = 0xE8;
inline constexpr std::uint8_t kLocalOpcodePop = 0xE9;
inline constexpr std::uint8_t kLocalOpcodeAdd = 0xEA;
inline constexpr std::uint8_t kLocalOpcodeAddTrap = 0xEB;
inline constexpr std::uint8_t kLocalOpcodeAddWrap = 0xEC;
inline constexpr std::uint8_t kLocalOpcodeAddSaturate = 0xED;
inline constexpr std::uint8_t kLocalOpcodeAddExtend = 0xEE;
inline constexpr std::uint8_t kLocalOpcodeSub = 0xEF;
inline constexpr std::uint8_t kLocalOpcodeSubTrap = 0xF0;
inline constexpr std::uint8_t kLocalOpcodeSubWrap = 0xF1;
inline constexpr std::uint8_t kLocalOpcodeSubSaturate = 0xF2;
inline constexpr std::uint8_t kLocalOpcodeSubExtend = 0xF3;
inline constexpr std::uint8_t kLocalOpcodeMul = 0xF4;
inline constexpr std::uint8_t kLocalOpcodeMulTrap = 0xF5;
inline constexpr std::uint8_t kLocalOpcodeMulWrap = 0xF6;
inline constexpr std::uint8_t kLocalOpcodeMulSaturate = 0xF7;
inline constexpr std::uint8_t kLocalOpcodeMulExtend = 0xF8;
inline constexpr std::uint8_t kLocalOpcodeDiv = 0xF9;
inline constexpr std::uint8_t kLocalOpcodeDivTrap = 0xFA;
inline constexpr std::uint8_t kLocalOpcodeDivWrap = 0xFB;
inline constexpr std::uint8_t kLocalOpcodeDivSaturate = 0xFC;
inline constexpr std::uint8_t kLocalOpcodeDivExtend = 0xFD;
inline constexpr std::uint8_t kLocalOpcodeEqual = 0xFE;
inline constexpr std::uint8_t kLocalOpcodeNotEqual = 0xFF;
inline constexpr std::uint8_t kLocalOpcodeLess = 0x01;
inline constexpr std::uint8_t kLocalOpcodeLessEqual = 0x02;
inline constexpr std::uint8_t kLocalOpcodeGreater = 0x03;
inline constexpr std::uint8_t kLocalOpcodeGreaterEqual = 0x04;
inline constexpr std::uint8_t kLocalOpcodeAnd = 0x05;
inline constexpr std::uint8_t kLocalOpcodeOr = 0x06;
inline constexpr std::uint8_t kLocalOpcodeXor = 0x07;
inline constexpr std::uint8_t kLocalOpcodeNand = 0x08;
inline constexpr std::uint8_t kLocalOpcodeNor = 0x09;
inline constexpr std::uint8_t kLocalOpcodeNot = 0x0A;
inline constexpr std::uint8_t kLocalOpcodeJump = 0x0B;
inline constexpr std::uint8_t kLocalOpcodeJumpIfZero = 0x0C;
inline constexpr std::uint8_t kLocalOpcodeCheckApproval = 0x0D;
inline constexpr std::uint8_t kLocalOpcodeCheckLimits = 0x0E;
inline constexpr std::uint8_t kLocalOpcodeCheckRole = 0x0F;
inline constexpr std::uint8_t kLocalOpcodeCheckRoleIndirect = 0x10;
inline constexpr std::uint8_t kLocalOpcodeNullCheck = 0x11;
inline constexpr std::uint8_t kLocalOpcodeAssert = 0x12;
inline constexpr std::uint8_t kLocalOpcodeCall = 0x13;
inline constexpr std::uint8_t kLocalOpcodeCallIndirect = 0x14;
inline constexpr std::uint8_t kLocalOpcodeOutputInt = 0x15;
inline constexpr std::uint8_t kLocalOpcodeOutputChar = 0x16;
inline constexpr std::uint8_t kLocalOpcodePrintLine = 0x17;
inline constexpr std::uint8_t kLocalOpcodeInputInt = 0x18;
inline constexpr std::uint8_t kLocalOpcodeInputChar = 0x19;
inline constexpr std::uint8_t kLocalOpcodeVerifyEcc = 0x1A;
inline constexpr std::uint8_t kLocalOpcodeCopyMemory = 0x1B;
inline constexpr std::uint8_t kLocalOpcodeCompareMemory = 0x1C;
inline constexpr std::uint8_t kLocalOpcodeRole = 0x1D;
inline constexpr std::uint8_t kLocalOpcodeOperatorInput = 0x1E;
inline constexpr std::uint8_t kLocalOpcodeReturn = 0x1F;
inline constexpr std::uint8_t kLocalOpcodeHalt = 0x20;

// 平台特化 opcode 值 = (TORTURE_PLATFORM_ID << 8) | kLocalOpcode*
inline constexpr std::uint16_t kOpcodeVerify =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeVerify);
inline constexpr std::uint16_t kOpcodePushInteger =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePushInteger);
inline constexpr std::uint16_t kOpcodePushString =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePushString);
inline constexpr std::uint16_t kOpcodePushNull =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePushNull);
inline constexpr std::uint16_t kOpcodePushReference =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePushReference);
inline constexpr std::uint16_t kOpcodeDereference =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDereference);
inline constexpr std::uint16_t kOpcodeFieldReference =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeFieldReference);
inline constexpr std::uint16_t kOpcodeLoad =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeLoad);
inline constexpr std::uint16_t kOpcodeStore =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeStore);
inline constexpr std::uint16_t kOpcodeStoreDereference =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeStoreDereference);
inline constexpr std::uint16_t kOpcodeStorePointer =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeStorePointer);
inline constexpr std::uint16_t kOpcodePop =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePop);
inline constexpr std::uint16_t kOpcodeAdd =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAdd);
inline constexpr std::uint16_t kOpcodeAddTrap =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAddTrap);
inline constexpr std::uint16_t kOpcodeAddWrap =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAddWrap);
inline constexpr std::uint16_t kOpcodeAddSaturate =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAddSaturate);
inline constexpr std::uint16_t kOpcodeAddExtend =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAddExtend);
inline constexpr std::uint16_t kOpcodeSub =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeSub);
inline constexpr std::uint16_t kOpcodeSubTrap =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeSubTrap);
inline constexpr std::uint16_t kOpcodeSubWrap =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeSubWrap);
inline constexpr std::uint16_t kOpcodeSubSaturate =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeSubSaturate);
inline constexpr std::uint16_t kOpcodeSubExtend =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeSubExtend);
inline constexpr std::uint16_t kOpcodeMul =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeMul);
inline constexpr std::uint16_t kOpcodeMulTrap =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeMulTrap);
inline constexpr std::uint16_t kOpcodeMulWrap =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeMulWrap);
inline constexpr std::uint16_t kOpcodeMulSaturate =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeMulSaturate);
inline constexpr std::uint16_t kOpcodeMulExtend =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeMulExtend);
inline constexpr std::uint16_t kOpcodeDiv =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDiv);
inline constexpr std::uint16_t kOpcodeDivTrap =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDivTrap);
inline constexpr std::uint16_t kOpcodeDivWrap =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDivWrap);
inline constexpr std::uint16_t kOpcodeDivSaturate =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDivSaturate);
inline constexpr std::uint16_t kOpcodeDivExtend =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeDivExtend);
inline constexpr std::uint16_t kOpcodeEqual =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeEqual);
inline constexpr std::uint16_t kOpcodeNotEqual =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeNotEqual);
inline constexpr std::uint16_t kOpcodeLess =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeLess);
inline constexpr std::uint16_t kOpcodeLessEqual =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeLessEqual);
inline constexpr std::uint16_t kOpcodeGreater =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeGreater);
inline constexpr std::uint16_t kOpcodeGreaterEqual =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeGreaterEqual);
inline constexpr std::uint16_t kOpcodeAnd =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAnd);
inline constexpr std::uint16_t kOpcodeOr =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeOr);
inline constexpr std::uint16_t kOpcodeXor =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeXor);
inline constexpr std::uint16_t kOpcodeNand =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeNand);
inline constexpr std::uint16_t kOpcodeNor =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeNor);
inline constexpr std::uint16_t kOpcodeNot =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeNot);
inline constexpr std::uint16_t kOpcodeJump =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeJump);
inline constexpr std::uint16_t kOpcodeJumpIfZero =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeJumpIfZero);
inline constexpr std::uint16_t kOpcodeCheckApproval =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCheckApproval);
inline constexpr std::uint16_t kOpcodeCheckLimits =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCheckLimits);
inline constexpr std::uint16_t kOpcodeCheckRole =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCheckRole);
inline constexpr std::uint16_t kOpcodeCheckRoleIndirect =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCheckRoleIndirect);
inline constexpr std::uint16_t kOpcodeNullCheck =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeNullCheck);
inline constexpr std::uint16_t kOpcodeAssert =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeAssert);
inline constexpr std::uint16_t kOpcodeCall =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCall);
inline constexpr std::uint16_t kOpcodeCallIndirect =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCallIndirect);
inline constexpr std::uint16_t kOpcodeOutputInt =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeOutputInt);
inline constexpr std::uint16_t kOpcodeOutputChar =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeOutputChar);
inline constexpr std::uint16_t kOpcodePrintLine =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodePrintLine);
inline constexpr std::uint16_t kOpcodeInputInt =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeInputInt);
inline constexpr std::uint16_t kOpcodeInputChar =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeInputChar);
inline constexpr std::uint16_t kOpcodeVerifyEcc =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeVerifyEcc);
inline constexpr std::uint16_t kOpcodeCopyMemory =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCopyMemory);
inline constexpr std::uint16_t kOpcodeCompareMemory =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeCompareMemory);
inline constexpr std::uint16_t kOpcodeRole =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeRole);
inline constexpr std::uint16_t kOpcodeOperatorInput =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeOperatorInput);
inline constexpr std::uint16_t kOpcodeReturn =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeReturn);
inline constexpr std::uint16_t kOpcodeHalt =
    static_cast<std::uint16_t>((static_cast<std::uint16_t>(TORTURE_PLATFORM_ID) << 8) | kLocalOpcodeHalt);

}  // namespace torture::vm::platform_bytecode
