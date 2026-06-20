// VM 指令处理器接口定义头文件

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "vm/memory.h"
#include "vm/value.h"

namespace torture::vm {

// 前向声明
struct Instruction;
struct FunctionBytecode;
class Interpreter;

// 指令处理器执行结果
enum class HandlerResult {
    Continue,   // 继续执行下一条指令
    Return,     // 函数返回（RET/HALT）
    NotMatched  // 不匹配本处理器
};

// 指令执行上下文，提供处理器需要的所有运行时状态
struct ExecutionContext {
    std::vector<Value>& stack;
    Memory& memory;
    const std::string& framePrefix;
    const Instruction& instruction;
    const FunctionBytecode& function;
    const std::string& caller;
    // pc 引用允许处理器修改程序计数器
    std::size_t& pc;
    // Interpreter 引用，用于访问 program_、input_、output_ 等成员
    class Interpreter& interpreter;
    // 返回值，RET/HALT 时由处理器设置
    Value returnValue;
};

// 指令处理器抽象接口（策略模式）
class InstructionHandler {
public:
    virtual ~InstructionHandler() = default;
    // 判断处理器是否能处理该 opcode
    virtual bool canHandle(const std::string& op) const = 0;
    // 执行指令，返回执行结果
    virtual HandlerResult execute(ExecutionContext& ctx) = 0;
};

} // namespace torture::vm
