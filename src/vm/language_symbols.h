#pragma once

#include <string_view>

namespace torture::vm {

namespace builtin {

inline constexpr std::string_view kOutputInt = "outputint";
inline constexpr std::string_view kOutputChar = "outputchar";
inline constexpr std::string_view kPrintLine = "println";
inline constexpr std::string_view kInputInt = "inputint";
inline constexpr std::string_view kInputChar = "inputchar";
inline constexpr std::string_view kVerifyMemoryIntegrity = "verifymemoryintegrity";
inline constexpr std::string_view kCopyMemory = "copymemory";
inline constexpr std::string_view kCompareMemory = "comparememory";
inline constexpr std::string_view kAlwaysApprove = "alwaysapprove";
inline constexpr std::string_view kAlwaysDeny = "alwaysdeny";
inline constexpr std::string_view kRootAuthority = "root";

}  // namespace builtin

namespace role_operation {

inline constexpr std::string_view kGrant = "grant";
inline constexpr std::string_view kDelegate = "delegate";
inline constexpr std::string_view kRevoke = "revoke";
inline constexpr std::string_view kAssume = "assume";
inline constexpr std::string_view kTrace = "trace";

}  // namespace role_operation

namespace function_category {

inline constexpr std::string_view kCallable = "callable";
inline constexpr std::string_view kUncallable = "uncallable";
inline constexpr std::string_view kApproval = "approval";

}  // namespace function_category

namespace source_function {

inline constexpr std::string_view kMain = "main";

}  // namespace source_function

namespace source_literal {

inline constexpr std::string_view kVoidType = "void";
inline constexpr std::string_view kTrue = "true";
inline constexpr std::string_view kYes = "yes";
inline constexpr std::string_view kNull = "null";
inline constexpr std::string_view kReadable = "readable";
inline constexpr std::string_view kComputational = "computational";
inline constexpr std::string_view kRootObject = "object";

}  // namespace source_literal

namespace overflow_policy {

inline constexpr std::string_view kTrap = "trap";
inline constexpr std::string_view kWrap = "wrap";
inline constexpr std::string_view kSaturate = "saturate";
inline constexpr std::string_view kExtend = "extend";

}  // namespace overflow_policy

namespace gate_operation {

inline constexpr std::string_view kAnd = "and";
inline constexpr std::string_view kOr = "or";
inline constexpr std::string_view kXor = "xor";
inline constexpr std::string_view kNand = "nand";
inline constexpr std::string_view kNor = "nor";
inline constexpr std::string_view kNot = "not";

}  // namespace gate_operation

namespace class_pattern {

inline constexpr std::string_view kAbstractFactory = "abstractfactory";
inline constexpr std::string_view kStrategy = "strategy";
inline constexpr std::string_view kDependencyInjection = "dependencyinjection";

}  // namespace class_pattern

namespace class_method {

inline constexpr std::string_view kInit = "init";
inline constexpr std::string_view kDestroy = "destroy";
inline constexpr std::string_view kPrint = "print";

}  // namespace class_method

}  // namespace torture::vm
