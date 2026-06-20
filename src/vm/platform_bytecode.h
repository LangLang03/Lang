#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

#include "vm/platform.h"

// 平台特化字节码常量：魔数、版本、flags、平台 ID 字段、opcode 编号表。
// 同一份 .h 在 12 个目标平台分别编译出不同的 constexpr 值。
namespace torture::vm::platform_bytecode {

// 4 字节魔数 + 版本号 + flags + 4 字母后缀（按 TORTURE_PLATFORM_ID 静态分支）
#if TORTURE_PLATFORM_ID == TORTURE_PLATFORM_LINUX_X64
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'L', '6', '4'};
inline constexpr std::string_view kPlatformSuffix = "LX64";
inline constexpr std::uint16_t kVersion = 0x0100;
inline constexpr std::uint16_t kFlags   = 0x0001;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "LX64"
#elif TORTURE_PLATFORM_ID == TORTURE_PLATFORM_LINUX_X32
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'L', 'X', '2'};
inline constexpr std::string_view kPlatformSuffix = "LX32";
inline constexpr std::uint16_t kVersion = 0x0101;
inline constexpr std::uint16_t kFlags   = 0x0002;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "LX32"
#elif TORTURE_PLATFORM_ID == TORTURE_PLATFORM_LINUX_ARM32
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'L', '3', 'A'};
inline constexpr std::string_view kPlatformSuffix = "LA32";
inline constexpr std::uint16_t kVersion = 0x0102;
inline constexpr std::uint16_t kFlags   = 0x0003;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "LA32"
#elif TORTURE_PLATFORM_ID == TORTURE_PLATFORM_LINUX_ARM64
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'L', 'A', '8'};
inline constexpr std::string_view kPlatformSuffix = "LA64";
inline constexpr std::uint16_t kVersion = 0x0103;
inline constexpr std::uint16_t kFlags   = 0x0004;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "LA64"
#elif TORTURE_PLATFORM_ID == TORTURE_PLATFORM_WINDOWS_X86
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'W', '3', '2'};
inline constexpr std::string_view kPlatformSuffix = "WX86";
inline constexpr std::uint16_t kVersion = 0x0200;
inline constexpr std::uint16_t kFlags   = 0x0005;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "WX86"
#elif TORTURE_PLATFORM_ID == TORTURE_PLATFORM_WINDOWS_X64
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'W', '6', '4'};
inline constexpr std::string_view kPlatformSuffix = "WX64";
inline constexpr std::uint16_t kVersion = 0x0201;
inline constexpr std::uint16_t kFlags   = 0x0006;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "WX64"
#elif TORTURE_PLATFORM_ID == TORTURE_PLATFORM_WINDOWS_ARM32
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'W', '3', 'A'};
inline constexpr std::string_view kPlatformSuffix = "WA32";
inline constexpr std::uint16_t kVersion = 0x0202;
inline constexpr std::uint16_t kFlags   = 0x0007;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "WA32"
#elif TORTURE_PLATFORM_ID == TORTURE_PLATFORM_WINDOWS_ARM64
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'W', 'A', '8'};
inline constexpr std::string_view kPlatformSuffix = "WA64";
inline constexpr std::uint16_t kVersion = 0x0203;
inline constexpr std::uint16_t kFlags   = 0x0008;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "WA64"
#elif TORTURE_PLATFORM_ID == TORTURE_PLATFORM_ANDROID_X86
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'A', '3', '2'};
inline constexpr std::string_view kPlatformSuffix = "AX86";
inline constexpr std::uint16_t kVersion = 0x0300;
inline constexpr std::uint16_t kFlags   = 0x0009;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "AX86"
#elif TORTURE_PLATFORM_ID == TORTURE_PLATFORM_ANDROID_X64
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'A', '6', '4'};
inline constexpr std::string_view kPlatformSuffix = "AX64";
inline constexpr std::uint16_t kVersion = 0x0301;
inline constexpr std::uint16_t kFlags   = 0x000A;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "AX64"
#elif TORTURE_PLATFORM_ID == TORTURE_PLATFORM_ANDROID_ARM32
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'A', '3', 'A'};
inline constexpr std::string_view kPlatformSuffix = "AA32";
inline constexpr std::uint16_t kVersion = 0x0302;
inline constexpr std::uint16_t kFlags   = 0x000B;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "AA32"
#elif TORTURE_PLATFORM_ID == TORTURE_PLATFORM_ANDROID_ARM64
inline constexpr std::array<char, 4> kMagicBytes = {'T', 'A', 'A', '8'};
inline constexpr std::string_view kPlatformSuffix = "AA64";
inline constexpr std::uint16_t kVersion = 0x0303;
inline constexpr std::uint16_t kFlags   = 0x000C;
#define TORTURE_PLATFORM_SUFFIX_LITERAL "AA64"
#else
  #error "TORTURE_PLATFORM_ID does not match any of the 12 supported platforms"
#endif

// 平台 ID 字段：写入字节流第 5 字节
inline constexpr std::uint8_t kPlatformId = TORTURE_PLATFORM_ID;

// 12 平台共享的 opcode 局部编号 (0x01..0x42)
inline constexpr std::uint16_t kLocalOpcodeVerify             = 0x01;
inline constexpr std::uint16_t kLocalOpcodePushInteger        = 0x02;
inline constexpr std::uint16_t kLocalOpcodePushString         = 0x03;
inline constexpr std::uint16_t kLocalOpcodePushNull           = 0x04;
inline constexpr std::uint16_t kLocalOpcodePushReference      = 0x05;
inline constexpr std::uint16_t kLocalOpcodeDereference        = 0x06;
inline constexpr std::uint16_t kLocalOpcodeFieldReference     = 0x07;
inline constexpr std::uint16_t kLocalOpcodeLoad               = 0x08;
inline constexpr std::uint16_t kLocalOpcodeStore              = 0x09;
inline constexpr std::uint16_t kLocalOpcodeStoreDereference   = 0x0A;
inline constexpr std::uint16_t kLocalOpcodeStorePointer       = 0x0B;
inline constexpr std::uint16_t kLocalOpcodePop                = 0x0C;
inline constexpr std::uint16_t kLocalOpcodeAdd                = 0x0D;
inline constexpr std::uint16_t kLocalOpcodeAddTrap            = 0x0E;
inline constexpr std::uint16_t kLocalOpcodeAddWrap            = 0x0F;
inline constexpr std::uint16_t kLocalOpcodeAddSaturate        = 0x10;
inline constexpr std::uint16_t kLocalOpcodeAddExtend          = 0x11;
inline constexpr std::uint16_t kLocalOpcodeSub                = 0x12;
inline constexpr std::uint16_t kLocalOpcodeSubTrap            = 0x13;
inline constexpr std::uint16_t kLocalOpcodeSubWrap            = 0x14;
inline constexpr std::uint16_t kLocalOpcodeSubSaturate        = 0x15;
inline constexpr std::uint16_t kLocalOpcodeSubExtend          = 0x16;
inline constexpr std::uint16_t kLocalOpcodeMul                = 0x17;
inline constexpr std::uint16_t kLocalOpcodeMulTrap            = 0x18;
inline constexpr std::uint16_t kLocalOpcodeMulWrap            = 0x19;
inline constexpr std::uint16_t kLocalOpcodeMulSaturate        = 0x1A;
inline constexpr std::uint16_t kLocalOpcodeMulExtend          = 0x1B;
inline constexpr std::uint16_t kLocalOpcodeDiv                = 0x1C;
inline constexpr std::uint16_t kLocalOpcodeDivTrap            = 0x1D;
inline constexpr std::uint16_t kLocalOpcodeDivWrap            = 0x1E;
inline constexpr std::uint16_t kLocalOpcodeDivSaturate        = 0x1F;
inline constexpr std::uint16_t kLocalOpcodeDivExtend          = 0x20;
inline constexpr std::uint16_t kLocalOpcodeEqual              = 0x21;
inline constexpr std::uint16_t kLocalOpcodeNotEqual           = 0x22;
inline constexpr std::uint16_t kLocalOpcodeLess               = 0x23;
inline constexpr std::uint16_t kLocalOpcodeLessEqual          = 0x24;
inline constexpr std::uint16_t kLocalOpcodeGreater            = 0x25;
inline constexpr std::uint16_t kLocalOpcodeGreaterEqual       = 0x26;
inline constexpr std::uint16_t kLocalOpcodeAnd                = 0x27;
inline constexpr std::uint16_t kLocalOpcodeOr                 = 0x28;
inline constexpr std::uint16_t kLocalOpcodeXor                = 0x29;
inline constexpr std::uint16_t kLocalOpcodeNand               = 0x2A;
inline constexpr std::uint16_t kLocalOpcodeNor                = 0x2B;
inline constexpr std::uint16_t kLocalOpcodeNot                = 0x2C;
inline constexpr std::uint16_t kLocalOpcodeJump               = 0x2D;
inline constexpr std::uint16_t kLocalOpcodeJumpIfZero         = 0x2E;
inline constexpr std::uint16_t kLocalOpcodeCheckApproval      = 0x2F;
inline constexpr std::uint16_t kLocalOpcodeCheckLimits        = 0x30;
inline constexpr std::uint16_t kLocalOpcodeCheckRole          = 0x31;
inline constexpr std::uint16_t kLocalOpcodeCheckRoleIndirect  = 0x32;
inline constexpr std::uint16_t kLocalOpcodeNullCheck          = 0x33;
inline constexpr std::uint16_t kLocalOpcodeAssert             = 0x34;
inline constexpr std::uint16_t kLocalOpcodeCall               = 0x35;
inline constexpr std::uint16_t kLocalOpcodeCallIndirect       = 0x36;
inline constexpr std::uint16_t kLocalOpcodeOutputInt          = 0x37;
inline constexpr std::uint16_t kLocalOpcodeOutputChar         = 0x38;
inline constexpr std::uint16_t kLocalOpcodePrintLine          = 0x39;
inline constexpr std::uint16_t kLocalOpcodeInputInt           = 0x3A;
inline constexpr std::uint16_t kLocalOpcodeInputChar          = 0x3B;
inline constexpr std::uint16_t kLocalOpcodeVerifyEcc          = 0x3C;
inline constexpr std::uint16_t kLocalOpcodeCopyMemory         = 0x3D;
inline constexpr std::uint16_t kLocalOpcodeCompareMemory      = 0x3E;
inline constexpr std::uint16_t kLocalOpcodeRole               = 0x3F;
inline constexpr std::uint16_t kLocalOpcodeOperatorInput      = 0x40;
inline constexpr std::uint16_t kLocalOpcodeReturn             = 0x41;
inline constexpr std::uint16_t kLocalOpcodeHalt               = 0x42;

// 12 平台特化的 opcode 整数值 = (TORTURE_PLATFORM_ID << 8) | 局部编号
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

// 跨平台查找：把任意 uint16_t opcode id 翻译为带平台后缀的名字（仅当它属于本平台时）
std::string_view opcodeNameForOpcodeId(std::uint16_t id);

// 反向查找：接受带后缀的新名或旧别名（不带后缀视为当前平台）
std::optional<std::uint16_t> opcodeIdFromName(std::string_view name);

}  // namespace torture::vm::platform_bytecode
