// 在包含 platform.h 之前先定义守卫宏，避免将 inline constexpr 定义引入
// 本翻译单元；随后本 TU 中以非 inline constexpr 形式提供 external-linkage
// 版本供 VM 模块链接。两个版本的实现体取值完全一致，ODR 兼容。
#define TORTURE_PLATFORM_NO_INLINE_IMPL
#include "vm/platform.h"
#undef TORTURE_PLATFORM_NO_INLINE_IMPL

namespace torture::vm {

// 暴露给 VM 模块的 external-linkage 实现（同时也是 Task 1 .cpp 要求）
constexpr std::uint8_t currentPlatformId() {
    return TORTURE_PLATFORM_ID;
}

constexpr const char* currentPlatformNameCString() {
    return TORTURE_PLATFORM_NAME_STRING;
}

}  // namespace torture::vm
