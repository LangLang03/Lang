#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <string_view>

namespace torture::vm {

// 12 个目标平台的 ID 枚举常量（仅在 namespace 内提供 constexpr 形式）
inline constexpr std::uint8_t TORTURE_PLATFORM_LINUX_X64 = 0x01;
inline constexpr std::uint8_t TORTURE_PLATFORM_LINUX_X32 = 0x02;
inline constexpr std::uint8_t TORTURE_PLATFORM_LINUX_ARM32 = 0x03;
inline constexpr std::uint8_t TORTURE_PLATFORM_LINUX_ARM64 = 0x04;
inline constexpr std::uint8_t TORTURE_PLATFORM_WINDOWS_X86 = 0x05;
inline constexpr std::uint8_t TORTURE_PLATFORM_WINDOWS_X64 = 0x06;
inline constexpr std::uint8_t TORTURE_PLATFORM_WINDOWS_ARM32 = 0x07;
inline constexpr std::uint8_t TORTURE_PLATFORM_WINDOWS_ARM64 = 0x08;
inline constexpr std::uint8_t TORTURE_PLATFORM_ANDROID_X86 = 0x09;
inline constexpr std::uint8_t TORTURE_PLATFORM_ANDROID_X64 = 0x0A;
inline constexpr std::uint8_t TORTURE_PLATFORM_ANDROID_ARM32 = 0x0B;
inline constexpr std::uint8_t TORTURE_PLATFORM_ANDROID_ARM64 = 0x0C;

// 同一分支下，按 __aarch64__ / __arm__ / __x86_64__ / __i386__ 区分位数
// 未知架构触发 #error
#if defined(__ANDROID__)
  #if defined(__aarch64__)
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_ANDROID_ARM64
    #define TORTURE_PLATFORM_NAME_STRING "android-arm64"
  #elif defined(__arm__)
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_ANDROID_ARM32
    #define TORTURE_PLATFORM_NAME_STRING "android-arm32"
  #elif defined(__x86_64__)
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_ANDROID_X64
    #define TORTURE_PLATFORM_NAME_STRING "android-x64"
  #elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_ANDROID_X86
    #define TORTURE_PLATFORM_NAME_STRING "android-x86"
  #else
    #error "unsupported target platform: unknown Android architecture"
  #endif
#elif defined(_WIN32) || defined(_WIN64)
  // Windows 平台：MSVC 走 _M_* 宏，MinGW/Clang-cl 走 __* 宏
  #if defined(_M_ARM64) || (defined(__aarch64__) && defined(_WIN32))
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_WINDOWS_ARM64
    #define TORTURE_PLATFORM_NAME_STRING "windows-arm64"
  #elif defined(_M_ARM) || (defined(__arm__) && defined(_WIN32))
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_WINDOWS_ARM32
    #define TORTURE_PLATFORM_NAME_STRING "windows-arm32"
  #elif defined(_M_X64) || defined(__x86_64__)
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_WINDOWS_X64
    #define TORTURE_PLATFORM_NAME_STRING "windows-x64"
  #elif defined(_M_IX86) || defined(__i386__)
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_WINDOWS_X86
    #define TORTURE_PLATFORM_NAME_STRING "windows-x86"
  #else
    #error "unsupported target platform: unknown Windows architecture"
  #endif
#elif defined(__linux__)
  #if defined(__aarch64__)
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_LINUX_ARM64
    #define TORTURE_PLATFORM_NAME_STRING "linux-arm64"
  #elif defined(__arm__)
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_LINUX_ARM32
    #define TORTURE_PLATFORM_NAME_STRING "linux-arm32"
  #elif defined(__x86_64__)
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_LINUX_X64
    #define TORTURE_PLATFORM_NAME_STRING "linux-x64"
  #elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
    #define TORTURE_PLATFORM_ID TORTURE_PLATFORM_LINUX_X32
    #define TORTURE_PLATFORM_NAME_STRING "linux-x32"
  #else
    #error "unsupported target platform: unknown Linux architecture"
  #endif
#else
  #error "unsupported target platform: only Android / Windows / Linux are supported"
#endif

static_assert(
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_LINUX_X64 ||
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_LINUX_X32 ||
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_LINUX_ARM32 ||
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_LINUX_ARM64 ||
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_WINDOWS_X86 ||
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_WINDOWS_X64 ||
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_WINDOWS_ARM32 ||
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_WINDOWS_ARM64 ||
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_ANDROID_X86 ||
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_ANDROID_X64 ||
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_ANDROID_ARM32 ||
    TORTURE_PLATFORM_ID == TORTURE_PLATFORM_ANDROID_ARM64,
    "TORTURE_PLATFORM_ID is not one of the 12 supported platform values");

inline constexpr std::uint8_t kPlatformId = TORTURE_PLATFORM_ID;
inline constexpr std::string_view kPlatformName = TORTURE_PLATFORM_NAME_STRING;

inline constexpr char kPlatformByteOrderLabel = []() constexpr {
    if constexpr (std::endian::native == std::endian::big) {
        return 'B';
    } else {
        return 'L';
    }
}();

#ifndef TORTURE_PLATFORM_NO_INLINE_IMPL
inline constexpr std::uint8_t currentPlatformId() {
    return kPlatformId;
}

inline constexpr std::string_view currentPlatformName() {
    return kPlatformName;
}

inline constexpr char currentPlatformByteOrderLabel() {
    return kPlatformByteOrderLabel;
}
#endif  // TORTURE_PLATFORM_NO_INLINE_IMPL

}  // namespace torture::vm
