// VM 内存管理实现

#include "vm/memory.h"

#include <stdexcept>

namespace torture::vm {

const std::string& Memory::ensureAddress(const std::string& name, const std::string& framePrefix) {
    // 生成完整地址：帧前缀 + 名称
    std::string address = framePrefix + name;
    // 若地址不存在则创建并初始化为 0，已存在则不覆盖
    auto [it, inserted] = storage_.emplace(address, integerValue(0));
    return it->first;
}

Value Memory::load(const std::string& name, const std::string& framePrefix) {
    // 确保地址存在后返回对应值
    return storage_[ensureAddress(name, framePrefix)];
}

void Memory::store(const std::string& name, const std::string& framePrefix, Value value) {
    // 确保地址存在后写入新值
    storage_[ensureAddress(name, framePrefix)] = std::move(value);
}

Value Memory::ref(const std::string& name, const std::string& framePrefix) {
    // 基于完整地址创建引用值
    return refValue(ensureAddress(name, framePrefix));
}

Value Memory::loadRef(const Value& ref) {
    // 引用类型校验
    if (ref.kind != ValueKind::Ref) {
        throw std::runtime_error("cannot dereference non-reference value");
    }
    return storage_[ref.string];
}

void Memory::storeRef(const Value& ref, Value value) {
    // 引用类型校验
    if (ref.kind != ValueKind::Ref) {
        throw std::runtime_error("cannot assign through non-reference value");
    }
    storage_[ref.string] = std::move(value);
}

} // namespace torture::vm
