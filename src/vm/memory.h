// VM 内存管理抽象头文件

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "vm/value.h"

namespace torture::vm {

// VM 内存管理类，封装地址生成与存储操作
class Memory {
public:
    // 确保指定名称的地址存在，不存在则创建并初始化为 0
    const std::string& ensureAddress(const std::string& name, const std::string& framePrefix);

    // 加载指定名称的值
    Value load(const std::string& name, const std::string& framePrefix);

    // 存储值到指定名称
    void store(const std::string& name, const std::string& framePrefix, Value value);

    // 创建指定名称的引用
    Value ref(const std::string& name, const std::string& framePrefix);

    // 通过引用加载值
    Value loadRef(const Value& ref);

    // 通过引用存储值
    void storeRef(const Value& ref, Value value);

private:
    // 底层存储，键为完整地址（framePrefix + name），值为存储的 Value
    std::unordered_map<std::string, Value> storage_;
};

} // namespace torture::vm
