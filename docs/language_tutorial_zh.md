# TortureLang 教学文档

本文档面向第一次接触 TortureLang 的开发者。目标是让你能读懂现有程序、写出能通过编译和语义检查的程序，并理解这个语言中和安全相关的手续。

TortureLang 是一个刻意繁琐、偏形式主义、带安全仪式感的教学语言。它不是 C、Rust、Python 那种追求短写法的语言。它要求开发者显式写出权限、理由、资源限制、IO 声明、判断授权、运算符授权和类型位宽。很多语法看起来像手续，这是语言的核心风格。

**目录**

1. 工具链和执行流程
2. 文件结构和缩进规则
3. 最小可运行程序
4. 函数声明
5. 类型系统
6. 变量声明和访问权限
7. 字面量和 UTF-8 字符串
8. 表达式和运算符授权
9. 赋值和溢出策略
10. 授权调用
11. IO 操作
12. 判断语句
13. 循环和形式化条款
14. 证明语句
15. 角色和权限管理
16. 函数指针
17. 结构体
18. 类
19. 内置函数
20. 字节码和运行时安全
21. 完整示例：成绩分级程序
22. 常见错误
23. 写代码时的检查清单

## 1. 工具链和执行流程

仓库会构建两个主要命令：

```sh
build/torturec lex 文件.torture
build/torturec parse 文件.torture
build/torturec check 文件.torture
build/torturec compile 文件.torture -o 文件.tbc
build/torturevm inspect 文件.tbc
build/torturevm run 文件.tbc
```

常用流程：

```sh
cmake --build build
build/torturec check fixtures/valid/smoke.torture
build/torturec compile fixtures/valid/smoke.torture -o /tmp/smoke.tbc
build/torturevm run /tmp/smoke.tbc
```

编译器分阶段工作：

1. `lex`：词法分析，检查标识符、字符串、符号。
2. `parse`：语法分析，生成 AST。
3. `check`：语义检查，检查类型、权限、IO 声明、资源限制、角色、判断手续。
4. `compile`：生成二进制字节码。
5. `run`：虚拟机执行字节码。

## 2. 文件结构和缩进规则

每个源文件必须以如下语句开头：

```torture
require ecc;
```

缩进规则：

1. 只能使用空格缩进。
2. 每一级缩进是 4 个空格。
3. 非空行缩进必须是 4 的倍数。
4. 普通代码块的第一条可执行语句必须以 `proceed` 开头。
5. `class`、`struct`、`implement` 这类声明容器不要求内部第一行写 `proceed`，但函数体、判断体、循环体需要。

合法的代码块：

```torture
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
}
```

不合法的代码块：

```torture
require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    verifyfunctionidentity();
}
```

原因：函数体第一条可执行语句没有写 `proceed`。

## 3. 最小可运行程序

完整文件：

```torture
require ecc;
function public callable returnable void main() code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
}
```

这个程序不输出任何内容，但可以通过检查和编译。

逐项解释：

```torture
require ecc;
```

源文件必须声明启用 ECC 要求。

```torture
function public callable returnable void main()
```

声明一个公开、可调用、可返回、返回类型为 `void` 的 `main` 函数。

```torture
code size 128 max stack size 64
```

声明函数的代码大小预算和最大栈预算。

```torture
requires security level 1 allowed roles admin
```

声明调用安全级别要求和允许角色。

```torture
proceed verifyfunctionidentity();
```

每个函数的第一条可执行语句必须验证函数身份。

## 4. 函数声明

普通函数形式：

```torture
function public callable returnable readable int:32 addone(readable int:32 x) code size 128 max stack size 64 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    return compute (x + 1) with overflow trap;
}
```

无返回值函数形式：

```torture
function public callable void printscore(readable int:32 score) code size 256 max stack size 128 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    authorize invocation of outputint at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "输出分数属于IO操作" with justification "print score" with arguments { value by value = score } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出分数" with approval timeout 1000;
}
```

函数头可以包含这些条款：

```torture
code size 128
max stack size 64
locals 2
authorized by root
requires security level 1
requires grantor root
allowed roles admin, viewer
where callable from broker
```

含义：

1. `code size N`：声明函数预计代码大小。
2. `max stack size N`：声明函数最大栈大小。
3. `locals N`：声明本函数局部变量数量，若声明则必须精确匹配。
4. `authorized by root`：声明函数定义由谁授权。
5. `requires security level N`：声明调用此函数的最低安全级别。
6. `requires grantor root`：声明调用时必须满足授权者要求。
7. `allowed roles admin, viewer`：声明哪些角色可以调用。
8. `where callable from broker`：声明只允许特定调用者路径。

## 5. 类型系统

类型必须显式写位宽。省略位宽会语义错误。

合法类型：

```torture
int:8
int:16
int:32
int:64
uint:8
uint:16
uint:32
uint:64
float:32
float:64
char:8
char:32
char:8[]
char:32[]
bool:1
chain
ptr<readable, int:32>
ptr<writable, int:32>
ptr<readable writable, int:32>
ref<readable, int:32>
fptr<return:int:32, params:(int:32), security:1, maxstack:64, codesize:128>
```

非法类型：

```torture
int
uint
float
char
bool
ptr<readable, int>
```

比较规则：

1. `int`、`uint`、`float` 可以互相比较，但位宽必须相同。
2. `int:32` 和 `uint:32` 可以比较。
3. `int:32` 和 `int:64` 不可以比较。
4. `char:8` 只能和 `char:8` 比较。
5. `char:32` 只能和 `char:32` 比较。
6. `bool:1` 只允许 `==` 和 `!=`，不允许 `<`、`>`、`<=`、`>=`。
7. 指针和引用只允许 `==` 和 `!=`，并且类型、指针种类、访问权限、指向类型必须完全一致。
8. 结构体和类值不能直接用运算符比较，必须调用 `compare` 或 `equals` 方法，并走授权调用流程。

## 6. 变量声明和访问权限

变量声明形式：

```torture
declare mutable readable writable purpose computational scope local int:32 x = 41;
```

字段说明：

1. `mutable` 或 `immutable`：是否允许后续赋值。
2. `readable`：允许读取。
3. `writable`：允许写入。
4. `purpose computational`：用途标签。
5. `scope local`：作用域标签。
6. `int:32`：显式类型。
7. `x`：变量名。
8. `= 41`：可选初始化。

只读变量：

```torture
declare mutable readable purpose computational scope local int:32 x = 1;
```

这个变量可读但不可写，后续 `assign x = 2;` 会报错。

只写变量：

```torture
declare mutable writable purpose computational scope local int:32 x = 1;
```

这个变量可写但不可读，把它传给 `outputint` 会报错。

不可变变量：

```torture
declare immutable readable writable purpose computational scope local int:32 x = 1;
```

这个变量初始化后不能再次赋值。

用途规则：

```torture
declare mutable readable writable purpose computational scope local int:32 src = 1;
declare mutable readable writable purpose storage scope local int:32 dst = 0;
assign dst = src;
```

上面不合法，因为跨 `purpose` 赋值必须用 `compute` 包装：

```torture
assign dst = compute (src) with overflow trap;
```

## 7. 字面量和 UTF-8 字符串

整数：

```torture
declare mutable readable writable purpose computational scope local int:32 score = 90;
```

布尔：

```torture
declare mutable readable writable purpose computational scope local bool:1 ok = true;
declare mutable readable writable purpose computational scope local bool:1 failed = false;
```

普通字符串表达式：

```torture
"HelloWorld"
```

声明过的字面量字符串：

```torture
literal "成绩管理系统"
```

`println` 要求参数必须是 `literal "..."` 或保存了声明字面量的变量。

UTF-8 字符串是支持的：

```torture
authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "输出中文标题属于IO操作" with justification "title" with arguments { value by value = literal "成绩管理系统" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出标题" with approval timeout 1000;
```

字符串中不允许无效 UTF-8。

## 8. 表达式和运算符授权

普通算术：

```torture
compute (x + 1) with overflow trap
```

加减乘除：

```torture
x + y
x - y
x * y
x / y
```

逻辑：

```torture
a and b
a or b
not a
a && b
a || b
!a
```

条件表达式：

```torture
flag ? 100 : 1
```

比较运算符必须显式授权使用。正确写法：

```torture
x authorize use operator == y because literal "确实需要比较两个数是否相等"
```

可授权的比较运算符：

```torture
==
!=
<
>
<=
>=
```

合法例子：

```torture
declare mutable readable writable purpose computational scope local int:32 left = 1;
declare mutable readable writable purpose computational scope local int:32 right = 2;
declare mutable readable writable purpose computational scope local bool:1 same = left authorize use operator == right because literal "业务需要确认两个值是否相同";
declare mutable readable writable purpose computational scope local bool:1 smaller = left authorize use operator < right because literal "业务需要确认左值是否更小";
```

非法例子：

```torture
declare mutable readable writable purpose computational scope local bool:1 same = left == right;
```

原因：没有写 `authorize use operator ==` 和 `because literal "..."`。

## 9. 赋值和溢出策略

普通赋值：

```torture
assign x = 1;
```

复合赋值：

```torture
assign x += 1;
assign x -= 1;
assign x *= 2;
assign x /= 2;
```

带溢出策略的计算：

```torture
assign x = compute (x + 1) with overflow trap;
assign x = compute (x + 1) with overflow wrap;
assign x = compute (x + 1) with overflow saturate;
assign x = compute (x + 1) with overflow extend;
```

策略说明：

1. `trap`：溢出时报错。
2. `wrap`：按机器整数包装。
3. `saturate`：饱和到边界值。
4. `extend`：允许更宽范围的中间值。

## 10. 授权调用

函数调用必须使用 `authorize invocation`。

无返回值调用：

```torture
authorize invocation of printscore at security level 1 with memory limit 128 with timeout 1000 with justification "打印分数" with arguments { score by value = 91 } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许打印分数" with approval timeout 1000;
```

有返回值调用：

```torture
declare mutable readable writable purpose computational scope local int:32 y = authorize invocation of addone at security level 1 with memory limit 128 with timeout 1000 with justification "计算加一" with arguments { x by value = 40 } predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许调用加一函数" with approval timeout 1000;
```

调用必须写的安全手续：

1. `at security level N`：调用安全级别。
2. `with memory limit N`：内存预算。
3. `with timeout N`：超时预算。
4. `predictstackdepth N`：预测栈深度。
5. `with authority chain role`：使用哪条权限链。
6. `with approval of approval_function`：使用哪个审批函数。
7. `with approval justification "..."`：审批理由。
8. `with approval timeout N`：审批超时。
9. 返回值不用时必须写 `discarding return`。

传参方式：

```torture
with arguments { x by value = value }
with arguments { target by reference = &score }
```

## 11. IO 操作

IO 内置函数包括：

```torture
outputint
outputchar
println
inputint
inputchar
operatorinput
```

所有 IO 操作都必须声明：

```torture
with io operation because literal "这里解释为什么要进行IO"
```

输出整数：

```torture
authorize invocation of outputint at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出整数结果" with justification "print number" with arguments { value by value = x } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出整数" with approval timeout 1000;
```

输出字符串：

```torture
authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出中文提示" with justification "print title" with arguments { value by value = literal "请输入成绩" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出提示" with approval timeout 1000;
```

读取整数：

```torture
assign score = authorize invocation of inputint at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要读取用户输入的成绩" with justification "read score" with arguments { } predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许读取成绩" with approval timeout 1000;
```

审批函数中的操作员输入：

```torture
operatorinput prompt "approve" timeout 1000 into decision with io operation because literal "需要操作员输入审批结果";
```

## 12. 判断语句

判断语句不只是 `if`，它必须声明：

1. 谁授权了判断。
2. 为什么要判断。
3. 预计有几个 `else if`。
4. 预计有几个 `else`。

基本形式：

```torture
if (score authorize use operator >= 90 because literal "需要判断是否达到优秀线") judging authorized by root because literal "教务处授权进行成绩等级判断" expects elseifs 1 else 1 {
    proceed authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出优秀等级" with justification "excellent" with arguments { value by value = literal "优秀" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出优秀等级" with approval timeout 1000;
} else if (score authorize use operator >= 60 because literal "需要判断是否达到及格线") judging authorized by root because literal "教务处授权进行及格等级判断" expects elseifs 0 else 1 {
    proceed authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出及格等级" with justification "pass" with arguments { value by value = literal "及格" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出及格等级" with approval timeout 1000;
} else judging authorized by root because literal "教务处授权进入不及格兜底分支" {
    proceed authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出不及格等级" with justification "fail" with arguments { value by value = literal "不及格" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出不及格等级" with approval timeout 1000;
}
```

更整活的条件入口也支持：

```torture
convene conditional tribunal over (score authorize use operator >= 90 because literal "需要启动条件法庭判断优秀线") judging authorized by root because literal "教务处授权召开条件法庭" expects appealroutes 0 defaultdockets 0 {
    proceed authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出法庭判断结果" with justification "tribunal" with arguments { value by value = literal "条件法庭认为成绩优秀" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出法庭判断" with approval timeout 1000;
}
```

`expects elseifs N else M` 和 `expects appealroutes N defaultdockets M` 等价。

## 13. 循环和形式化条款

`while`：

```torture
while (running authorize use operator == true because literal "需要判断循环是否继续") {
    proceed assign running = false;
}
```

`do until`：

```torture
do invariant (x authorize use operator >= 0 because literal "循环中x必须非负") because literal "非负性是循环不变量" decreases (4 - x) because literal "剩余次数必须下降" {
    proceed assign x += 1;
} until (x authorize use operator >= 4 because literal "需要判断循环是否结束");
```

`for`：

```torture
for (; x authorize use operator < 3 because literal "需要判断循环次数是否未满"; x) {
    proceed assign x += 1;
}
```

形式化条款：

```torture
invariant (表达式) because literal "解释不变量为什么成立"
decreases (表达式) because literal "解释度量为什么下降"
```

## 14. 证明语句

证明语句不会替代真正的数学证明，但会在运行时编译为断言类检查。

定理：

```torture
prove theorem finalscore: require (score authorize use operator >= 0 because literal "证明前提授权使用大于等于运算符") therefore (score authorize use operator >= 0 because literal "证明结论授权使用大于等于运算符") because literal "成绩保持非负";
```

公理：

```torture
prove axiom zerononnegative: assume (zero authorize use operator >= 0 because literal "公理授权使用大于等于运算符") because literal "零被行政登记为非负";
```

证明语句必须带 `because literal "..."`。

## 15. 角色和权限管理

角色语句：

```torture
grant viewer to student via root;
grant editor to teacher via root;
delegate viewer to assistant with teacher;
revoke editor from teacher;
assume viewer with student;
trace to student;
```

函数头用 `allowed roles` 声明谁能调用：

```torture
function public callable void viewscore(readable int:32 score) code size 256 max stack size 128 requires security level 1 allowed roles viewer {
    proceed verifyfunctionidentity();
    authorize invocation of outputint at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "查看分数需要输出" with justification "view" with arguments { value by value = score } discarding return predictstackdepth 4 with authority chain viewer with approval of alwaysapprove with approval justification "允许查看" with approval timeout 1000;
}
```

调用时使用权限链：

```torture
authorize invocation of viewscore at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "调用查看函数会触发输出" with justification "student view" with arguments { score by value = score } discarding return predictstackdepth 8 with authority chain student with approval of alwaysapprove with approval justification "允许学生查看" with approval timeout 1000;
```

如果 `student` 没有被授予 `viewer`，运行时会拒绝调用。

## 16. 函数指针

函数指针类型：

```torture
fptr<return:int:32, params:(int:32), security:1, maxstack:64, codesize:128>
```

赋值：

```torture
authorize fptr assignment of fp to addone with authority chain root;
```

间接调用必须写 `nullcheck`：

```torture
declare mutable readable writable purpose computational scope local int:32 y = authorize invocation via fp at security level 1 with memory limit 128 with timeout 1000 with justification "通过函数指针调用加一函数" with arguments { x by value = 9 } predictstackdepth 4 nullcheck true with authority chain admin with approval of alwaysapprove with approval justification "允许函数指针调用" with approval timeout 1000;
```

函数指针的返回类型、参数类型、安全级别、最大栈、代码大小必须和目标函数匹配。

## 17. 结构体

结构体声明：

```torture
struct record = struct {
    declare mutable readable writable purpose storage scope local int:32 amount;
} implement {
    method public callable returnable void init(readable writable ptr<readable writable, record> self) code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
        assign *self.amount = 12;
    }
    method public callable returnable void destroy() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void copy() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void move() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void serialize() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void deserialize() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void tostring() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void fromstring() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void hash() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void compare() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void equals() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void getproperty() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void setproperty() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void invoke() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void getsize() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void alignof() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void defaultvalue() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void validate() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void mutate() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void clone() code size 128 max stack size 64 requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
}
```

普通 `struct` 必须实现这些方法：

```torture
init
destroy
copy
move
serialize
deserialize
tostring
fromstring
hash
compare
equals
getproperty
setproperty
invoke
getsize
alignof
defaultvalue
validate
mutate
clone
```

结构体值不能直接用运算符比较。要比较结构体，写 `compare` 或 `equals` 方法，并使用 `authorize invocation of value.equals` 这类授权调用。

## 18. 类

类比结构体更严格。类必须声明：

1. 字段数量。
2. 方法数量。
3. 定义授权者。
4. 定义理由。
5. 可读内存权限。
6. 继承来源和继承理由。
7. 设计模式 `abstractfactory strategy dependencyinjection`。
8. 注入依赖。
9. 方法 `init`、`destroy`、`print`。
10. 每个类方法必须声明 `locals N authorized by root`。

最小类：

```torture
class printer fields 0 methods 3 authorized by root because literal "需要一个可打印对象" memory readable writable inherits object because literal "继承根对象是对象系统要求" patterns abstractfactory strategy dependencyinjection injects console {
} implement {
    method public callable returnable void init() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void destroy() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
    method public callable returnable void print() code size 128 max stack size 64 locals 0 authorized by root requires security level 1 allowed roles admin {
        proceed verifyfunctionidentity();
    }
}
```

实例化类：

```torture
instantiate printer p authorized by root memory readable writable from printerfactory using strategy characterstrategy injecting console because literal "创建对象需要记录授权和设计模式来源";
```

## 19. 内置函数

内置函数：

```torture
outputint(value: int:64) -> void
outputchar(value: char:8) -> void
println(value: char:8[]) -> void
inputint() -> int:64
inputchar() -> char:8
alwaysapprove(details: char:8[]) -> bool:1
alwaysdeny(details: char:8[]) -> bool:1
verifymemoryintegrity() -> bool:1
copymemory(dst: ptr<readablewritable, int:64>, src: ptr<readable, int:64>, size: int:64) -> void
comparememory(left: ptr<readable, int:64>, right: ptr<readable, int:64>, size: int:64) -> int:32
```

注意：

1. IO 内置函数必须声明 `with io operation because literal "..."`。
2. `println` 的参数必须是声明字面量。
3. 非审批函数里的授权调用必须带审批函数。
4. `alwaysapprove` 和 `alwaysdeny` 是内置审批函数。

## 20. 字节码和运行时安全

编译产物是二进制字节码。当前字节码格式版本为 v3。

查看字节码：

```sh
build/torturevm inspect /tmp/program.tbc
```

运行字节码：

```sh
build/torturevm run /tmp/program.tbc
```

安全相关行为：

1. 字节码带环境绑定信息。不同编译环境或运行环境可能被拒绝。
2. 虚拟机会检查调用目标是否存在。
3. 虚拟机会检查角色链。
4. 虚拟机会检查审批函数。
5. 虚拟机会检查资源限制。
6. 间接调用必须有 `nullcheck`。
7. 溢出策略为 `trap` 时，整数溢出会导致运行错误。
8. 证明、循环不变量和递减度量会生成断言类检查。

这不是操作系统级沙箱。它的安全目标是语言层的手续、类型和授权约束，不等同于防御所有宿主环境攻击。

## 21. 完整示例：成绩分级程序

这个程序读取一个内置成绩值并输出等级。它展示了变量声明、IO 声明、判断声明、运算符授权和审批。

```torture
require ecc;
function public callable returnable void main() code size 2048 max stack size 256 requires security level 1 allowed roles admin {
    proceed verifyfunctionidentity();
    declare mutable readable writable purpose computational scope local int:32 score = 92;
    authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出系统标题" with justification "title" with arguments { value by value = literal "成绩分级系统" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出标题" with approval timeout 1000;
    authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出分数标签" with justification "score label" with arguments { value by value = literal "分数如下" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出分数标签" with approval timeout 1000;
    authorize invocation of outputint at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出具体分数" with justification "score value" with arguments { value by value = score } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出分数" with approval timeout 1000;
    if (score authorize use operator >= 90 because literal "需要判断是否达到优秀等级") judging authorized by root because literal "教务处授权判断优秀等级" expects elseifs 1 else 1 {
        proceed authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出优秀等级" with justification "excellent" with arguments { value by value = literal "等级：优秀" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出优秀等级" with approval timeout 1000;
    } else if (score authorize use operator >= 60 because literal "需要判断是否达到及格等级") judging authorized by root because literal "教务处授权判断及格等级" expects elseifs 0 else 1 {
        proceed authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出及格等级" with justification "pass" with arguments { value by value = literal "等级：及格" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出及格等级" with approval timeout 1000;
    } else judging authorized by root because literal "教务处授权进入不及格兜底分支" {
        proceed authorize invocation of println at security level 1 with memory limit 128 with timeout 1000 with io operation because literal "需要输出不及格等级" with justification "fail" with arguments { value by value = literal "等级：不及格" } discarding return predictstackdepth 4 with authority chain admin with approval of alwaysapprove with approval justification "允许输出不及格等级" with approval timeout 1000;
    }
}
```

保存为 `/tmp/score.torture` 后可以运行：

```sh
build/torturec check /tmp/score.torture
build/torturec compile /tmp/score.torture -o /tmp/score.tbc
build/torturevm run /tmp/score.tbc
```

期望输出：

```text
成绩分级系统
分数如下
92
等级：优秀
```

仓库中还有完整交互式成绩管理系统：

```text
fixtures/valid/gradebook.torture
```

它展示了学生查看、教师修改、角色授权、中文输出、输入读取、IO 声明和判断授权。

## 22. 常见错误

缺少 `require ecc;`：

```text
missing-ecc
```

函数第一条语句不是 `verifyfunctionidentity();`：

```text
missing-verifyfunctionidentity
```

代码块第一条可执行语句没有 `proceed`：

```text
missing-proceed
```

变量不可读：

```text
read-not-allowed
```

变量不可写：

```text
write-not-allowed
```

IO 没声明：

```text
missing-io-declaration
```

`println` 参数不是声明字面量：

```text
println-requires-literal
```

比较运算符没授权：

```text
missing-operator-authorization
```

数值比较位宽不一致：

```text
comparison-width-mismatch
```

布尔值做大小比较：

```text
bool-order-comparison
```

指针比较权限或类型不一致：

```text
pointer-comparison-type-mismatch
```

结构体直接比较：

```text
struct-comparison-requires-method
```

授权调用缺审批：

```text
missing-approval
```

间接调用缺少空指针检查：

```text
missing-nullcheck
```

## 23. 写代码时的检查清单

写函数时检查：

1. 文件第一行是否是 `require ecc;`。
2. 函数是否声明 `code size`。
3. 函数是否声明 `max stack size`。
4. 函数是否声明 `requires security level`。
5. 函数是否声明 `allowed roles`。
6. 函数体第一条语句是否是 `proceed verifyfunctionidentity();`。

写变量时检查：

1. 是否写了 `mutable` 或 `immutable`。
2. 是否写了 `readable` 或 `writable`。
3. 是否写了 `purpose`。
4. 是否写了 `scope`。
5. 类型是否有位宽。
6. 跨用途赋值是否使用 `compute`。

写判断时检查：

1. 比较运算符是否写了 `authorize use operator`。
2. 运算符授权是否写了 `because literal "..."`。
3. `if` 是否写了 `judging authorized by ...`。
4. 判断是否写了 `because literal "..."`。
5. `expects elseifs N else M` 的数量是否和实际一致。
6. `else` 是否也写了 judging 子句。

写调用时检查：

1. 是否使用 `authorize invocation`。
2. 是否写了 `at security level`。
3. 是否写了 `with memory limit`。
4. 是否写了 `with timeout`。
5. 是否写了 `predictstackdepth`。
6. 是否写了 `with authority chain`。
7. 是否写了 `with approval of`。
8. 是否写了 `with approval justification`。
9. 是否写了 `with approval timeout`。
10. 返回值不用时是否写了 `discarding return`。
11. 调用 IO 内置函数时是否写了 `with io operation because literal "..."`。

写结构体或类时检查：

1. 结构体是否实现所有 required methods。
2. 类是否声明字段数和方法数。
3. 类是否声明 `authorized by` 和定义理由。
4. 类内存是否至少 `readable`。
5. 类是否声明继承理由。
6. 类是否声明三个设计模式。
7. 类是否声明注入依赖。
8. 类方法是否声明 `locals N authorized by root`。

TortureLang 的核心原则是：能省的都不要省，能声明的都要声明，能解释的都必须解释。
