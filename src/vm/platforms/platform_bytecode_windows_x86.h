#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "vm/platform.h"

// 平台后缀字面量宏，供 X-macro 拼接使用
#define TORTURE_PLATFORM_SUFFIX_LITERAL "WX86"

namespace torture::vm::platform_bytecode {

// windows_x86 平台魔数
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'W', '3', '2'};

// 平台后缀
inline constexpr std::string_view kPlatformSuffix = "WX86";

// 版本
inline constexpr std::uint16_t kVersion = 0x0200;

// flags
inline constexpr std::uint16_t kFlags = 0x0005;

// 66 个独立低 8 位编号（0x10-0x51 范围）
inline constexpr std::uint8_t kLocalOpcodeVerify = 0x10;
inline constexpr std::uint8_t kLocalOpcodePushInteger = 0x11;
inline constexpr std::uint8_t kLocalOpcodePushString = 0x12;
inline constexpr std::uint8_t kLocalOpcodePushNull = 0x13;
inline constexpr std::uint8_t kLocalOpcodePushReference = 0x14;
inline constexpr std::uint8_t kLocalOpcodeDereference = 0x15;
inline constexpr std::uint8_t kLocalOpcodeFieldReference = 0x16;
inline constexpr std::uint8_t kLocalOpcodeLoad = 0x17;
inline constexpr std::uint8_t kLocalOpcodeStore = 0x18;
inline constexpr std::uint8_t kLocalOpcodeStoreDereference = 0x19;
inline constexpr std::uint8_t kLocalOpcodeStorePointer = 0x1A;
inline constexpr std::uint8_t kLocalOpcodePop = 0x1B;
inline constexpr std::uint8_t kLocalOpcodeAdd = 0x1C;
inline constexpr std::uint8_t kLocalOpcodeAddTrap = 0x1D;
inline constexpr std::uint8_t kLocalOpcodeAddWrap = 0x1E;
inline constexpr std::uint8_t kLocalOpcodeAddSaturate = 0x1F;
inline constexpr std::uint8_t kLocalOpcodeAddExtend = 0x20;
inline constexpr std::uint8_t kLocalOpcodeSub = 0x21;
inline constexpr std::uint8_t kLocalOpcodeSubTrap = 0x22;
inline constexpr std::uint8_t kLocalOpcodeSubWrap = 0x23;
inline constexpr std::uint8_t kLocalOpcodeSubSaturate = 0x24;
inline constexpr std::uint8_t kLocalOpcodeSubExtend = 0x25;
inline constexpr std::uint8_t kLocalOpcodeMul = 0x26;
inline constexpr std::uint8_t kLocalOpcodeMulTrap = 0x27;
inline constexpr std::uint8_t kLocalOpcodeMulWrap = 0x28;
inline constexpr std::uint8_t kLocalOpcodeMulSaturate = 0x29;
inline constexpr std::uint8_t kLocalOpcodeMulExtend = 0x2A;
inline constexpr std::uint8_t kLocalOpcodeDiv = 0x2B;
inline constexpr std::uint8_t kLocalOpcodeDivTrap = 0x2C;
inline constexpr std::uint8_t kLocalOpcodeDivWrap = 0x2D;
inline constexpr std::uint8_t kLocalOpcodeDivSaturate = 0x2E;
inline constexpr std::uint8_t kLocalOpcodeDivExtend = 0x2F;
inline constexpr std::uint8_t kLocalOpcodeEqual = 0x30;
inline constexpr std::uint8_t kLocalOpcodeNotEqual = 0x31;
inline constexpr std::uint8_t kLocalOpcodeLess = 0x32;
inline constexpr std::uint8_t kLocalOpcodeLessEqual = 0x33;
inline constexpr std::uint8_t kLocalOpcodeGreater = 0x34;
inline constexpr std::uint8_t kLocalOpcodeGreaterEqual = 0x35;
inline constexpr std::uint8_t kLocalOpcodeAnd = 0x36;
inline constexpr std::uint8_t kLocalOpcodeOr = 0x37;
inline constexpr std::uint8_t kLocalOpcodeXor = 0x38;
inline constexpr std::uint8_t kLocalOpcodeNand = 0x39;
inline constexpr std::uint8_t kLocalOpcodeNor = 0x3A;
inline constexpr std::uint8_t kLocalOpcodeNot = 0x3B;
inline constexpr std::uint8_t kLocalOpcodeJump = 0x3C;
inline constexpr std::uint8_t kLocalOpcodeJumpIfZero = 0x3D;
inline constexpr std::uint8_t kLocalOpcodeCheckApproval = 0x3E;
inline constexpr std::uint8_t kLocalOpcodeCheckLimits = 0x3F;
inline constexpr std::uint8_t kLocalOpcodeCheckRole = 0x40;
inline constexpr std::uint8_t kLocalOpcodeCheckRoleIndirect = 0x41;
inline constexpr std::uint8_t kLocalOpcodeNullCheck = 0x42;
inline constexpr std::uint8_t kLocalOpcodeAssert = 0x43;
inline constexpr std::uint8_t kLocalOpcodeCall = 0x44;
inline constexpr std::uint8_t kLocalOpcodeCallIndirect = 0x45;
inline constexpr std::uint8_t kLocalOpcodeOutputInt = 0x46;
inline constexpr std::uint8_t kLocalOpcodeOutputChar = 0x47;
inline constexpr std::uint8_t kLocalOpcodePrintLine = 0x48;
inline constexpr std::uint8_t kLocalOpcodeInputInt = 0x49;
inline constexpr std::uint8_t kLocalOpcodeInputChar = 0x4A;
inline constexpr std::uint8_t kLocalOpcodeVerifyEcc = 0x4B;
inline constexpr std::uint8_t kLocalOpcodeCopyMemory = 0x4C;
inline constexpr std::uint8_t kLocalOpcodeCompareMemory = 0x4D;
inline constexpr std::uint8_t kLocalOpcodeRole = 0x4E;
inline constexpr std::uint8_t kLocalOpcodeOperatorInput = 0x4F;
inline constexpr std::uint8_t kLocalOpcodeReturn = 0x50;
inline constexpr std::uint8_t kLocalOpcodeHalt = 0x51;

// 平台特化 opcode 值 = (TORTURE_PLATFORM_ID << 8) | kLocalOpcode*
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

static_assert(kLocalOpcodeVerify == 0x10, "windows_x86 local opcode range must start at 0x10");
static_assert(kLocalOpcodeHalt == 0x51, "windows_x86 local opcode range must end at 0x51");

}  // namespace torture::vm::platform_bytecode
