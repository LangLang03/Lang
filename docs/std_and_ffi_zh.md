# TortureLang 标准库（std）与 FFI 复杂化使用文档（中文）

本文档解释 TortureLang 中 `std` 包与外部函数接口（FFI）的引入方式、关键字族、校验链与示例。所有改动遵循「越繁琐越安全」的设计哲学：每一个 std 引用与 FFI 调用都必须走完一长串关键字手续，才能得到 sema 与运行期的认可。

## 1. 设计动机

- **防止误用**：复杂的关键字与描述子句让任何 std/FFI 调用都成为一段可审计的「行政文书」。
- **跨环境安全**：所有 std/FFI 元信息进入 `currentEnvironmentFingerprint()`，环境或工具链一旦变动，已编译字节码会被拒绝。
- **跨平台安全**：FFI 绑定必须显式声明 arch、sys 与库绝对路径，运行期再算一次 SHA-512 链。

## 2. 引入 std 包

### 2.1 顶部 require std;

```torture
require ecc;
require std;
stddecl literal "io demo" because literal "introduce std for io";
```

- `require std;` 是顶层声明，必须在所有 `function` / `struct` / `class` / `external` 之前。
- `stddecl` 是「std 引入声明」：必须给出一段字面量说明（`literal "..."`），再用 `because literal "..."` 给出理由，最后以 `;` 收尾。
- 可选追加 `stdbinding` / `stdpurposestatement` / `stdinfrastructurenote` 元说明子句，每条都需要 `literal + because literal + ;`。

### 2.2 调用 std 函数

调用任何 `std::xxx` 函数都必须显式带 `with from namespace std` 子句：

```torture
authorize invocation of std::io_stdout_write_line
    at security level 1
    with memory limit 128
    with timeout 1000
    with io operation because literal "declared io operation"
    with justification "print"
    with arguments { value by value = literal "Hello, std!" }
    discarding return
    predictstackdepth 4
    with authority chain admin
    with approval of alwaysapprove
    with approval justification "ok"
    with approval timeout 1000
    with from namespace std;
```

sema 阶段会校验：
- 必须存在 `require std;` 与 `stddecl` 元说明子句。
- `target` 以 `std::` 开头时，必须同时有 `fromNamespace == "std"`。

## 3. 引入 FFI

### 3.1 external 声明

```torture
external arch x64 sys linux lib "/usr/lib/libdemo.so" symbol add as addbinding
    signature sha512 "A" "<sha512(A)>" "<sha512(sha512(A))>"
    declaredescription literal "bind c add" because literal "ffi binding for c add" ;
```

字段顺序（缺一即报错 `ffi-missing-segment-N`）：
1. `arch <archid>` —— 11 个白名单之一：`x86 x64 arm32 arm64 riscv32 riscv64 mips32 mips64 sparc32 sparc64 powerpc64`。
2. `sys <sysid>` —— 7 个白名单之一：`linux windows macos android freebsd openbsd uefi`。
3. `lib "<abspath>"` —— 必须是字符串字面量，且以 `/` 开头（绝对路径）。
4. `symbol <name>` —— C 函数符号名。
5. `as <bindname>` —— 绑定到 Lang 端的别名，供 `apply external <bindname>` 引用。
6. `signature sha512 "<seg1>" "<seg2>" "<seg3>"` —— 必须是 3 段字符串字面量，满足
   `seg2 == sha512(seg1)` 且 `seg3 == sha512(seg2)`，任何一段不匹配都报 `ffi-sha512-chain-broken`。
7. `declaredescription literal "..." because literal "...";` —— 末段描述子句，缺则报 `description-missing-because-literal`。

### 3.2 apply 调用

```torture
proceed apply external addbinding
    at security level 1
    with memory limit 64
    with timeout 1000
    with justification "call c add"
    with arguments { value by value = literal "1", value by value = literal "2" }
    with from namespace std
    discarding return
    predictstackdepth 2
    with authority chain user
    with approval of alwaysapprove
    with approval justification "approve ffi call"
    with approval timeout 500
    with architecture x64
    with system linux
    with library path "/usr/lib/libdemo.so"
    with symbol add;
```

调用 `apply external` 必须凑齐 14 段手续（任何一段缺失会得到 `apply-missing-clause-N`）：

| # | 段 | 关键字片段 |
|---|----|------------|
| 1 | 绑定名 | `apply external <bindname>` |
| 2 | 安全级别 | `at security level N` |
| 3 | 内存上限 | `with memory limit M` |
| 4 | 超时 | `with timeout T` |
| 5 | IO 声明（可选） | `with io operation because literal "..."` |
| 6 | 调用理由 | `with justification "..."` |
| 7 | 参数列表 | `with arguments { ... }` |
| 7.5 | 命名空间（可选） | `with from namespace <ns>` |
| 8 | 返回值处理 | `discarding return` 或 `receiving return into <var>` |
| 9 | 预测栈深 | `predictstackdepth P` |
| 10 | 授权链 | `with authority chain <role>` |
| 11 | 审批函数 | `with approval of <approverfn>` |
| 12 | 审批理由 | `with approval justification "..."` |
| 13 | 审批超时 | `with approval timeout K` |
| 14 | 重新声明 arch/sys/lib/symbol | `with architecture <arch>` ... `with symbol <name>` |

### 3.3 运行期校验

vm.cpp 在执行 `APPLY_<platform>` 指令时会再次检查：
- `bindname` 必须在 `program.externalBindings` 中存在。
- `binding->arch` / `binding->sys` 与当前平台（`linux_x64` 等）匹配，apply 语句中重新声明的 arch/sys 也必须一致。
- 重新计算 `sha512(sha512Chain[0])` 与 `sha512(sha512Chain[1])`，比对后续两段；不匹配抛 `ffi-runtime-sha512-mismatch`。
- 库路径必须以 `/` 开头，且文件存在；否则抛 `ffi-runtime-library-missing`。

## 4. 位宽关键字（widthliteral）

旧写法 `int:32` 已被 sema 拒绝（报 `width-must-use-bit-literal`）。新写法：

```torture
declare mutable readable writable purpose computational scope local int of width bit32 x = 0;
```

- `int of width` 是必须的引导短语。
- `bitN` 必须是 `bit8` / `bit16` / `bit32` / `bit64` / `bit128` 之一。
- 同样适用于 `uint` / `float` / `char` / `bool`。

## 5. 描述子句族（description）

任何 `function` / `variable` / `parameter` / `return value` / `type definition` / `literal` 在 `require std;` 之后都必须附 1 条或多条「描述子句」：

| 子句 | 适用位置 | 关键字 |
|------|----------|--------|
| `declaredescription` | 函数、外部绑定 | `declaredescription literal "..." because literal "..." ;` |
| `parameterdescription` | 函数形参 | `parameterdescription literal "..." because literal "..." ;` |
| `returndescription` | 函数返回 | `returndescription literal "..." because literal "..." ;` |
| `variabledescription` | 局部变量 | `variabledescription literal "..." because literal "..." ;` |
| `typedefinitiondescription` | struct/class | `typedefinitiondescription literal "..." because literal "..." ;` |
| `literaldescription` | 字面量 | `literaldescription literal "..." because literal "..." ;` |

任何描述子句后必须紧跟 `because literal "...";`，缺则报 `description-missing-because-literal`。

## 6. 示例 fixture

- `fixtures/valid/stdhello.torture` —— 最小的 `require std;` 演示。
- `fixtures/valid/ffibind.torture` —— 完整的 `external` + `apply external` 演示。
- `fixtures/invalid/badsha.torture` —— 故意把 SHA-512 链第二段写错，断言 sema 报 `ffi-sha512-chain-broken`。

## 7. 常见诊断码

| 诊断码 | 含义 |
|--------|------|
| `ffi-sha512-chain-broken` | SHA-512 链 `seg1/seg2/seg3` 不满足 `seg2 = sha512(seg1)` 等 |
| `ffi-sha512-chain-wrong-size` | SHA-512 段数 ≠ 3 |
| `ffi-sha512-segment-must-be-string` | SHA-512 段不是字符串字面量 |
| `ffi-unsupported-architecture` | arch 不在 11 个白名单中 |
| `ffi-unsupported-system` | sys 不在 7 个白名单中 |
| `ffi-library-path-must-be-absolute` | lib 路径不以 `/` 开头 |
| `ffi-duplicate-binding` | 同一 bindname 出现多次 |
| `apply-missing-clause-N` | apply 14 段中第 N 段缺失 |
| `apply-missing-approval` | 缺少 `with approval of ...` |
| `apply-missing-return-clause` | 缺少 `discarding return` 或 `receiving return into` |
| `std-not-required` | 调用 `std::xxx` 但文件未 `require std;` |
| `std-namespace-required` | `std::xxx` 调用未带 `with from namespace std` |
| `width-must-use-bit-literal` | 旧写法 `int:32`，应改用 `int of width bit32` |
| `description-missing-because-literal` | 描述子句缺 `because literal` 后缀 |
