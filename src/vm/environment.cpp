#include "vm/environment.h"

#include "common/sha256.h"
#include "vm/platform.h"

#include <bit>
#include <sstream>

#ifndef TORTURE_BYTECODE_FORMAT_VERSION
#define TORTURE_BYTECODE_FORMAT_VERSION 3
#endif

#ifndef TORTURE_BUILD_COMPILER_ID
#define TORTURE_BUILD_COMPILER_ID "unknown"
#endif

#ifndef TORTURE_BUILD_COMPILER_VERSION
#define TORTURE_BUILD_COMPILER_VERSION "unknown"
#endif

#ifndef TORTURE_BUILD_SYSTEM_NAME
#define TORTURE_BUILD_SYSTEM_NAME "unknown"
#endif

#ifndef TORTURE_BUILD_SYSTEM_PROCESSOR
#define TORTURE_BUILD_SYSTEM_PROCESSOR "unknown"
#endif

#ifndef TORTURE_BUILD_CONFIGURATION
#define TORTURE_BUILD_CONFIGURATION "unknown"
#endif

#ifndef TORTURE_BUILD_ENVIRONMENT_BINDING
#define TORTURE_BUILD_ENVIRONMENT_BINDING "unbound"
#endif

namespace torture::vm {
namespace {

constexpr auto kRuntimeContract = "TortureLang/bytecode-environment/v1";
constexpr auto kLittleEndian = "little";
constexpr auto kBigEndian = "big";
constexpr auto kMixedEndian = "mixed";

std::string endianName() {
    if constexpr (std::endian::native == std::endian::little) {
        return kLittleEndian;
    } else if constexpr (std::endian::native == std::endian::big) {
        return kBigEndian;
    } else {
        return kMixedEndian;
    }
}

std::string compilerMacroSummary() {
    std::ostringstream out;
#if defined(__clang__)
    out << "clang=" << __clang_major__ << '.' << __clang_minor__ << '.' << __clang_patchlevel__ << '\n';
#elif defined(__GNUC__)
    out << "gcc=" << __GNUC__ << '.' << __GNUC_MINOR__ << '.' << __GNUC_PATCHLEVEL__ << '\n';
#elif defined(_MSC_VER)
    out << "msvc=" << _MSC_VER << '\n';
#else
    out << "compiler-macro=unknown\n";
#endif
#ifdef __VERSION__
    out << "compiler-version-string=" << __VERSION__ << '\n';
#endif
    return out.str();
}

} // namespace

std::string environmentSummary() {
    std::ostringstream out;
    out << "contract=" << kRuntimeContract << '\n';
    out << "bytecode-format=" << TORTURE_BYTECODE_FORMAT_VERSION << '\n';
    out << "platform-id=" << static_cast<unsigned>(torture::vm::kPlatformId) << '\n';
    out << "platform-name=" << torture::vm::kPlatformName << '\n';
    out << "platform-byte-order=" << torture::vm::kPlatformByteOrderLabel << '\n';
    out << "compiler-id=" << TORTURE_BUILD_COMPILER_ID << '\n';
    out << "compiler-version=" << TORTURE_BUILD_COMPILER_VERSION << '\n';
    out << "system-name=" << TORTURE_BUILD_SYSTEM_NAME << '\n';
    out << "system-processor=" << TORTURE_BUILD_SYSTEM_PROCESSOR << '\n';
    out << "configuration=" << TORTURE_BUILD_CONFIGURATION << '\n';
    out << "build-binding=" << TORTURE_BUILD_ENVIRONMENT_BINDING << '\n';
    out << "cplusplus=" << __cplusplus << '\n';
    out << "pointer-bytes=" << sizeof(void*) << '\n';
    out << "size-bytes=" << sizeof(std::size_t) << '\n';
    out << "endian=" << endianName() << '\n';
    // std/FFI 元信息：版本指纹 v1。该字符串会进入环境指纹，使得任何修改 std/FFI 行为
    // 的提交都会让已编译的字节码被环境检查拒绝。
    out << "std-ffi-magic=TortureLang/std-ffi-binding/v1\n";
    out << compilerMacroSummary();
    return out.str();
}

std::string currentEnvironmentFingerprint() {
    return torture::sha256Hex(environmentSummary());
}

bool matchesCurrentEnvironment(std::string_view fingerprint) {
    return fingerprint == currentEnvironmentFingerprint();
}

std::string shortFingerprint(std::string_view fingerprint) {
    constexpr std::size_t kShortFingerprintLength = 12;
    if (fingerprint.size() <= kShortFingerprintLength) {
        return std::string{fingerprint};
    }
    return std::string{fingerprint.substr(0, kShortFingerprintLength)};
}

} // namespace torture::vm
