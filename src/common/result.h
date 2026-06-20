// 通用结果类型定义头文件

#pragma once

#include <string>
#include <utility>
#include <variant>

#include "common/diagnostic.h"

namespace torture {

// 错误信息结构体：承载错误码、错误消息和错误来源阶段
struct Error {
    ErrorCode code = ErrorCode::Unknown;
    std::string message;
    std::string stage;  // 错误来源阶段
};

// 通用结果类型模板：封装成功值或错误信息
// 使用 std::variant 存储 T 或 Error，二者必居其一
template <typename T>
class Result {
public:
    // 成功构造：用值构造一个成功结果
    Result(T value) : data_(std::move(value)) {}

    // 失败构造：用错误信息构造一个失败结果
    Result(Error error) : data_(std::move(error)) {}

    // 判断是否为成功状态
    bool isOk() const { return std::holds_alternative<T>(data_); }

    // 判断是否为失败状态
    bool isError() const { return !isOk(); }

    // 获取成功值（只读访问）
    const T& value() const { return std::get<T>(data_); }

    // 获取成功值（可变访问）
    T& value() { return std::get<T>(data_); }

    // 获取错误信息
    const Error& error() const { return std::get<Error>(data_); }

private:
    std::variant<T, Error> data_;
};

} // namespace torture
