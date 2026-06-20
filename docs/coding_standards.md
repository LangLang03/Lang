# Lang 企业级编码规范

本规范适用于 Lang 项目（Torture 语言工具链）的全部 C++ 源代码、头文件、测试代码与应用入口。所有贡献者在提交代码前应确保其改动符合本规范。

---

## 1. 目录结构规范

项目采用分层架构，自下而上依次为：基础设施层、编译器层、虚拟机层、应用层。各层职责清晰、依赖方向单一。

### 1.1 `src/common/` 基础设施层

存放被上层共用的基础组件，不依赖编译器层与虚拟机层。当前包含：

- `diagnostic.h` / `diagnostic.cpp`：诊断信息收集与输出，提供 `Diagnostics`、`Diagnostic`、`SourceLocation` 等类型。
- `source.h` / `source.cpp`：源文件加载与抽象，提供 `SourceFile`。
- `sha256.h` / `sha256.cpp`：SHA256 摘要计算，供字节码哈希校验使用。
- 其余通用工具类型按需加入本目录。

### 1.2 `src/compiler/` 编译器层

实现从源代码到字节码的前端流水线，依赖 `src/common/`。当前包含：

- `token.h`：词法单元定义。
- `lexer.h` / `lexer.cpp`：词法分析。
- `indentation.h` / `indentation.cpp`：缩进格式检查。
- `ast.h` / `ast.cpp`：抽象语法树节点。
- `parser.h` / `parser.cpp`：语法分析。
- `sema.h` / `sema.cpp`：语义分析。
- `bytecode_compiler.h` / `bytecode_compiler.cpp`：字节码生成。

### 1.3 `src/vm/` 虚拟机层

实现字节码格式定义与虚拟机执行，依赖 `src/common/`。当前包含：

- `bytecode_format.h`：字节码二进制格式常量（魔数、版本、上限等）。
- `opcode.h` / `opcode.cc`：操作码枚举与指令构造。
- `bytecode.h` / `bytecode.cpp`：字节码程序结构、序列化与反序列化。
- `environment.h` / `environment.cpp`：运行环境指纹。
- `language_symbols.h`：语言内置符号常量。
- `platform.h` / `platform.cpp`：平台识别与平台 ID。
- `platform_bytecode.h`：平台字节码公共接口。
- `platform_vm.h`：平台虚拟机公共接口。
- `vm.h` / `vm.cpp`：虚拟机执行入口 `runBytecode`。

### 1.4 `src/vm/platforms/` 平台特化实现

存放 12 个目标平台（Linux / Windows / Android 各自的 x86、x64、arm32、arm64）的特化代码。命名规则：

- `platform_bytecode_<os>_<arch>.h` / `.cpp`：平台字节码常量与魔数。
- `platform_vm_<os>_<arch>.cpp`：平台虚拟机辅助实现。
- `bytecode_binary_<os>_<arch>.cc`：平台字节码二进制生成。

平台特化代码只可被 `src/vm/` 内部通过公共接口调用，不直接暴露给编译器层与应用层。

### 1.5 `apps/` 应用层

存放可执行程序入口，依赖 `src/common/`、`src/compiler/`、`src/vm/`。当前包含：

- `apps/torturec/`：编译器命令行入口 `torturec`，支持 `lex`、`parse`、`check`、`compile` 子命令。
- `apps/torturevm/`：虚拟机命令行入口 `torturevm`，用于加载并执行字节码。

应用层不得包含业务逻辑，仅负责参数解析、流水线编排与诊断输出。

### 1.6 `tests/` 测试代码

存放单元测试与集成测试。当前包含：

- `test_lexer.cpp`：词法分析测试。
- `test_parser_sema.cpp`：语法与语义分析测试。
- `test_vm_smoke.cpp`：虚拟机冒烟测试。

测试代码可依赖被测各层，但被测代码不得反向依赖测试代码。

### 1.7 `fixtures/` 测试用例文件

存放 Torture 语言测试源文件，分为：

- `fixtures/valid/`：合法用例（如 `helloworld.torture`、`reference.torture`、`smoke.torture`、`gradebook.torture`）。
- `fixtures/invalid/`：非法用例（如 `badindent.torture`），用于验证诊断能力。

### 1.8 `docs/` 文档目录

存放项目文档。当前包含：

- `language_tutorial_zh.md`：Torture 语言中文教程。
- `coding_standards.md`：本编码规范。

### 1.9 分层架构依赖规则

依赖方向严格自上而下，禁止反向依赖与跨层依赖：

```
apps/  ───────────────►  src/compiler/  ──►  src/common/
   │                        │
   └──────────────────────► │ src/vm/      ──►  src/common/
                            │     │
                            │     └─► src/vm/platforms/
                            │
                            └─► src/vm/platforms/  ──►  src/common/
```

具体规则：

1. `src/common/` 不依赖任何上层。
2. `src/compiler/` 与 `src/vm/` 仅依赖 `src/common/`，二者之间若需协作须经由公共类型（如 `Diagnostics`、`BytecodeProgram`）。
3. `src/vm/platforms/` 仅依赖 `src/common/` 与 `src/vm/` 内部公共头文件。
4. `apps/` 可依赖所有 `src/` 层，但不得被任何 `src/` 层依赖。
5. `tests/` 可依赖所有 `src/` 层与 `apps/`，但不得被生产代码依赖。

违反依赖方向的提交不予合并。

---

## 2. 命名约定

### 2.1 文件名

使用 `snake_case`，全部小写，单词以下划线分隔。

示例：

- `bytecode_compiler.cpp`
- `diagnostic.h`
- `platform_bytecode_linux_x64.cpp`

### 2.2 类名与结构体名

使用 `PascalCase`，每个单词首字母大写。

示例：

- `BytecodeProgram`
- `Interpreter`
- `Diagnostics`
- `FunctionBytecode`

### 2.3 函数名

使用 `camelCase`，首单词小写，后续单词首字母大写。

示例：

- `runBytecode`
- `writeBytecode`
- `compileToBytecode`
- `currentEnvironmentFingerprint`

### 2.4 变量名

使用 `camelCase`。

示例：

- `environmentFingerprint`
- `maxStackSize`
- `platformId`

### 2.5 常量名

使用 `kPascalCase`，即前缀 `k` 加 `PascalCase`。适用于 `constexpr` 与 `const` 常量、枚举值。

示例：

- `kMagicBytes`
- `kPlatformId`
- `kVersion`
- `kMaxStringBytes`

### 2.6 命名空间

使用 `snake_case`，可嵌套。

示例：

- `torture`
- `torture::vm`
- `torture::compiler`
- `torture::vm::bytecode_format`

### 2.7 枚举值

使用 `PascalCase`，每个单词首字母大写，不加 `k` 前缀。

示例：

- `ValueKind::Integer`
- `ValueKind::String`
- `DiagnosticSeverity::Error`
- `DiagnosticSeverity::Warning`

### 2.8 私有成员变量

使用 `camelCase` 并在尾部加下划线，以区分局部变量与参数。

示例：

- `program_`
- `diagnostics_`
- `options_`

---

## 3. 头文件规范

### 3.1 头文件保护

统一使用 `#pragma once` 作为头文件保护，不使用传统 `#ifndef` / `#define` / `#endif` 守卫。

示例：

```cpp
#pragma once

#include <string>

namespace torture {
// ...
} // namespace torture
```

### 3.2 include 顺序

按以下顺序分组包含头文件，各组之间以空行分隔：

1. 系统头文件与标准库头文件，使用尖括号，按字母序排列（如 `<iostream>`、`<string>`、`<vector>`）。
2. 项目头文件，使用双引号，按字母序排列（如 `"common/diagnostic.h"`、`"vm/bytecode.h"`）。

示例：

```cpp
#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include "common/diagnostic.h"
#include "vm/bytecode.h"
```

### 3.3 前向声明

优先使用前向声明减少编译依赖。当头文件仅需引用某类型的指针或引用时，使用前向声明而非包含其头文件。

示例：

```cpp
#pragma once

namespace torture::vm {

class BytecodeProgram; // 前向声明

bool runBytecode(const BytecodeProgram& program, /* ... */);

} // namespace torture::vm
```

### 3.4 声明与实现分离

头文件（`.h`）只声明类型与接口，实现放入对应的 `.cpp` 文件。以下例外允许在头文件中实现：

- 内联函数（`inline`）。
- 模板函数与模板类。
- `constexpr` 变量与简单访问器。

---

## 4. 错误处理规范

### 4.1 诊断收集优先

常规错误传播使用 `Diagnostics` 类收集错误，不使用 C++ 异常。编译器各阶段（词法、语法、语义、字节码生成）产生的错误均通过 `Diagnostics` 上报，调用方通过 `hasErrors()` 判断是否继续。

示例：

```cpp
Diagnostics diagnostics;
auto program = torture::compiler::parseTokens(tokens, diagnostics);
if (diagnostics.hasErrors()) {
    torture::printDiagnostics(std::cerr, diagnostics);
    return 1;
}
```

### 4.2 虚拟机内部异常

虚拟机（VM）内部可使用异常处理运行时错误（如字节码写入失败、I/O 错误），但必须在 VM 边界捕获并转换为诊断信息，不得将异常抛出到应用层。

示例：

```cpp
try {
    torture::vm::writeBytecode(out, *bytecode);
} catch (const std::exception& error) {
    diagnostics.error("bytecode", "write-failed", location, error.what());
    return 1;
}
```

### 4.3 Result 类型

新增 `Result<T>` 类型用于需要返回值或错误的场景。`Result<T>` 携带成功值或错误信息，调用方显式检查结果状态，避免遗漏错误处理。

示例：

```cpp
Result<BytecodeProgram> result = readBytecode(stream, name, diagnostics);
if (!result) {
    // 处理错误
}
auto program = std::move(result).value();
```

### 4.4 错误码分类

错误码使用 `ErrorCode` 枚举分类，按模块与严重程度组织。诊断码字符串（如 `"bad-magic"`、`"truncated"`、`"unknown-opcode"`）使用 `snake_case`，并集中在对应模块的常量中定义。

示例：

```cpp
namespace torture::vm::bytecode_diagnostic {

inline constexpr std::string_view kBadMagic = "bad-magic";
inline constexpr std::string_view kTruncated = "truncated";
inline constexpr std::string_view kUnknownOpcode = "unknown-opcode";

} // namespace torture::vm::bytecode_diagnostic
```

---

## 5. 注释规范

### 5.1 源文件首行说明

所有源文件（`.h`、`.cpp`、`.cc`）首行须包含中文文件说明注释，简述该文件用途。

示例：

```cpp
// VM 值类型定义头文件
#pragma once

// ...
```

```cpp
// 字节码编译器实现文件
#include "compiler/bytecode_compiler.h"

// ...
```

### 5.2 公开接口注释

公开类、结构体与函数须包含中文注释，说明用途、参数含义与返回值。注释置于声明上方。

示例：

```cpp
// 运行字节码程序，将输入输出绑定到指定流。
// program: 已加载的字节码程序。
// input: 输入流。
// output: 输出流。
// diagnostics: 诊断收集器，运行期错误通过它上报。
// options: 虚拟机运行选项（步数上限、ECC 校验开关等）。
// 返回值: 程序退出码，0 表示成功。
bool runBytecode(const BytecodeProgram& program, std::istream& input, std::ostream& output, Diagnostics& diagnostics, VmOptions options = {});
```

### 5.3 复杂逻辑行内注释

复杂逻辑、平台特化判断、位运算等处须包含中文行内注释说明意图，解释“为什么”而非“做什么”。

示例：

```cpp
// opcode 整数值 = (TORTURE_PLATFORM_ID << 8) | 局部编号，
// 由 platform_bytecode 提供的 constexpr 变量赋值，保证 12 平台枚举值两两不同。
enum class Opcode : std::uint16_t {
    kVerify = platform_bytecode::kOpcodeVerify,
    // ...
};
```

### 5.4 注释风格

统一使用 `//` 行注释，不使用 `/* */` 块注释。多行说明使用连续的 `//` 行。

示例：

```cpp
// 同一分支下，按 __aarch64__ / __arm__ / __x86_64__ / __i386__ 区分位数。
// 未知架构触发 #error。
#if defined(__ANDROID__)
// ...
#endif
```

---

## 附则

- 本规范自发布之日起生效，既有代码应逐步迁移至符合规范。
- 规范的修订须经项目维护者评审通过。
- 与本规范冲突的旧约定以本规范为准。
