#pragma once

// 标准库与 FFI 复杂化公共库：SHA-512 摘要。
// 头文件仅对外暴露一个轻量入口 `sha512Hex(std::string_view)`，把任意字符串转换为小写十六进制摘要。
// 该函数被编译器与虚拟机共同依赖：编译期用于 FFI 链式签名校验，运行期用于再次校验。

#include <cstdint>
#include <string>
#include <string_view>

namespace torture::common {

// 计算输入字符串的 SHA-512 摘要，并返回 128 个小写十六进制字符。
std::string sha512Hex(std::string_view text);

}  // namespace torture::common
