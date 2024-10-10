# 编译原理课程实践报告：crazy compiler

信息科学技术学院 2100013026 孙嘉旋

## 一、编译器概述

### 1.1 基本功能

本编译器基本具备如下功能：
1. 通过Koopa IR作为中间表示首先将SysY语言编译到Koopa IR。
2. 再从Koopa IR生成RISCV汇编。

### 1.2 主要特点

本编译器的主要特点是**用C++编写、前后端分离**、**使用 Flex 和 Bison 来分别生成词法分析器和语法分析器、无法判别程序合法性、只能按照特定语法规则进行编译**。

## 二、编译器设计

### 2.1 主要模块组成

sysy.l和sysy.y描述词法和语法规则，用于解析SysY程序；

ast.h & ast.cpp：实现了前端的AST架构，通过语法分析得到的AST逐步解析语法树并输出koopa IR的程序；

op.h中实现词法分析中需要的二元运算符；

visit.cpp：完成后端生成RISCV代码的过程；

### 2.2 主要数据结构

本编译器最核心的数据结构是AST结构。SysY程序经过Parser解析得到AST后，为每一个解析得到的模块设计一个AST类。每个AST类均由BaseAST派生，根据不同的AST种类实现不同的成员变量及函数。

**成员变量：**根据产生规则在不同的AST里添加了不同的指针，存储规约得到的非终结符，ident用于记录变量名相关信息，val用于记录和值相关的信息，TAG用于记录用来规约的非终结符。

**函数：**最重要的是Dump和DumpStr，主要功能均为向字符串str0中输出koopa IR代码，区别在于是否需要返回结果寄存器，虽然看起来二者可等价，但在真正实现过程中，这两个函数往往用于区别全局/局部变量等。getInitValArray函数为数组初始化做准备（填0），globalInitSearch与getElemPtr用于输出数组初始化语句 getElemPtrForLVal函数用于访问数组。

后端主要利用全局变量记录每个函数所需栈空间，每个变量存放在栈中的位置，若有递归调用则记录函数应返回的地址，利用restr存放生成的RISCV程序。后端实现整体为递归结构，利用visit的递归调用进行输出。

### 2.3 主要设计考虑及算法选择

#### 2.3.1 符号表与函数表
各类cnt用于表达式计数。

符号表使用map结构，是从符号名的string到pair<int,int>的映射，其中pair的第一个元素表示当前符号的值，第二个元素标识类型，表示变量、常量、数组、数组参数。

在实现语句块和作用域部分时，对符号表进行扩充，需要层次结构的符号表。考虑到每个符号表最多只有一个子表，因此采用不是很优雅的map结构记录每个符号表的父亲表。

对于符号表的计数有两个全局变量，symTabCnt记录当前存在的符号表数量，主要用于查找时确认最大查找深度，allsymTabCnt记录所有符号表的数量，用于为新生成的符号表分配序号，以免koopa IR中出现重复定义。

函数表记录每个函数的类型，数组表记录数组的维度。

```c
int expNumCnt=0, symTabCnt=0, allsymTabCnt=0, ifCnt=0, brctNumCnt=0, whileNumCnt=0;
std::string now_while_end="", now_while_entry="";
std::map<std::string, std::pair<int, int>> symbol_table;
std::map<std::string, std::pair<int, int>> *current_table;
std::map<std::string, std::variant<int, std::string>> *curFunvar_table;
std::map<std::map<std::string, std::pair<int, int>>*, std::map<std::string, std::pair<int, int>>*> total_table;
std::map<std::string, std::string> func_table;
std::map<std::string, std::pair<int, int>> glob_table;
```

#### 2.3.2 寄存器分配策略
没有做寄存器分配。在IR中的每个临时变量都为其在栈上开辟4字节空间存起来。使用一个Map存储变量和栈上位置。

#### 2.3.3 采用的优化策略
目前没有做任何优化。

## 三、编译器实现

### 3.1 各阶段编码细节

#### Lv1. main函数和Lv2. 初试目标代码生成

Lv1-2基础部分给出了较为详细的教程，但一开始思路并不清晰，上手过程中按照https://www.cnblogs.com/zhangleo/p/15963442.html 逐步搭建了环境，并完成了前两个 level的实现。

#### Lv3. 表达式
在第三个level 的实现中参考了 https://github.com/HowlingNorthWind/Compiler 的做法，借鉴了其中的语法分析结构和符号表结构，并以此为基础沿用至整个实现过程。

#### Lv4. 常量和变量
变量存在了栈上，每次用的时候就load到给定的寄存器里，运算完再将结果存回栈上。

对于常量的处理时，若没有访问到则先不求值，当访问该常数时利用const_value()函数求值并存下来。

栈帧的实现，目前只需要在上面存变量，所以用一个map，从 Koopa IR的 Value 映射到i32代表栈帧中的位置。

这个阶段卡了很多次的是如何处理不合法的BasicBlock，Koopa IR要求BB必须以ret、br、jump其一结尾，有一些隐藏测例不符合这种情况，进行了一些修复。

#### Lv5. 语句块和作用域

理想的符号表是一个单向链表结构，实现中采用map结构，相当于存储链表指针。初始时全局有一张符号表，用于记录全局变量。每进入一个新的基本块，则生成一张新的符号表，该符号表内的符号名称记为a_1/b_1/c_1...以区别不同的作用域。退出当前块，则删除当前符号表。

当从符号表查找符号时，从当前表向前递归查找，直到找到最近的符号定义，返回该符号在当前表中的名称。Map结构虽然丑陋，但其合法性在符号表仅记录符号名称和作用域，编译过程中不会跨层更故符号表，因此不会存在map索引失效的问题。

```
void BlockAST::Dump(std::string& str0) {
  isret = false ;

  // 每一块需要自己的符号表
  std::map<std::string, std::pair<int, int>> new_table;
  std::map<std::string, std::pair<int, int>>* record_table = current_table;
  current_table = &new_table;
  total_table[current_table] = record_table;

  std::map<std::string, int> new_array_dims_table;
  std::map<std::string, int> *tmp_array_dims_table = cur_array_dims_table;
  cur_array_dims_table = &new_array_dims_table;
  total_array_dims_table[cur_array_dims_table] =  tmp_array_dims_table;

  //记录嵌套作用域
  symTabCnt += 1;

  //一些操作

  //还原
  current_table = record_table;
  cur_array_dims_table = tmp_array_dims_table;
  symTabCnt -= 1;
}
```
#### Lv6. if语句

对于悬空else问题，按照龙书中提供的解决方案修改即可。

```
Stmt        ::= MatchedStmt | OpenStmt;
MatchedStmt ::= "return" [Exp] ";" | LVal "=" Exp ";" | [Exp] ";" | Block
                | "break" ";" | "while" "(" Exp ")" Stmt | "continue" ";"
                | "if" "(" Exp ")" MatchedStmt "else" MatchedStmtStmt ;
OpenStmt    ::= "if" "(" Exp ")" Stmt | "if" "(" Exp ")" MatchedStmt "else" OpenStmt ;
```
#### Lv7. while语句

中间代码生成时，只需按照如下格式进行生成即可。`continue` 语句可转为 `jump %while_entry`，`break` 语句可转为`jump %while_end`。

```
  jump %while_entry
%while_entry:
  %cond = Exp
  br %cond, %while_body, %while_end
%while_body:
  //! do something
  jump %while_entry
%while_end:
  //! do something
```
while之间可能会产生循环嵌套关系。为了在遇到 `continue` 或 `break` 时能确定当前应该跳到哪个 while 的对应标号，需要使用全局变量来维护循环嵌套树和当前语句在循环嵌套树中的位置。

#### Lv8. 函数和全局变量

前端还需要补上之前的短路求值，可以多用几个基本块，用一个临时变量保存表达式结果，然后表达式从左往右修改这个临时变量，同时按照短路规则，跨过不需要的块，最后判断条件时判断这个临时变量即可。如下，对于lhs||rhs

```
b1:
    %1 = alloc i32
    store %1,1
    %2 = lhs
    br %2,merge_block,rhs_block
rhs_block:
    %3 = rhs
    br %3,merge_block,false_block
false_block:
    store %1,0
    jump merge_block
merge_block:
    br %1,true,false
```

如下文法规则在 `"int" . IDENT blabla` 这个状态存在规约/规约冲突，无法确定将 `"int"` 规约为 `FuncType` 还是 `BType`。

```
FuncType      ::= "void" | "int";
BType         ::= "int";
VarDecl       ::= BType VarDef {"," VarDef} ";";
FuncDef       ::= FuncType IDENT "(" [FuncFParams] ")" Block;
```
我的处理方法是直接把类型变成终结符，即把BType直接写成“int”，FuncType拆为”int”和“void”。

```
VarDecl       ::= "int" VarDef {"," VarDef} ";";
FuncDef       ::= "int" IDENT "(" [FuncFParams] ")" Block | "void" IDENT "(" [FuncFParams] ")" Block;
```

生成目标代码有点复杂，但注意好以下几点就没问题：

1. 按照文档正确生成函数的 prologue 和 epilogue。
2. 前8个参数放置在寄存器中，其他参数放置在栈中，函数开始时要把参数复制到自己的栈帧中方便处理
3. 处理 ret 语句时注意 a0 sp 和 ra 的设置。

因为选择了不分配寄存器的策略，所以也不需要考虑调用者保存寄存器/被调用者保存寄存器的问题。

#### Lv9. 数组

数组需要额外支持`GetElemPtr`和`GetPtr`操作，分别用于数组访问和数组传参。由于加入了全局数组和全局常数组，之前的符号表可以通过value之中的int类型将三者分开。同时添加了很多函数用于支持数组初始化及访问，数组初始化按照文档的提示，首先补零，再填充。

在语法分析中，将所有和变量定义、变量使用的地方添加中括号，支持数组解析，同时由于数组仅有元素可被赋值，上一维的指针不能被复制，因此需要区分左值和右值。

感觉这块要说的其实没多少，思路清晰之后主要还是敲代码的过程比较繁琐吧，还是很痛苦的，写了很久真的不想写了，所以生成RISCV的阶段并没有非常完善，还是挂了几个点，由于时间关系没有更多的debug了。

### 3.2 测试情况说明

**【lv1-lv9 测试】**

lv1-lv7所有测试点为满分，lv8两段测试点为24/25

lv9 koopa IR为31/35，lv9 RISCV为27/35

**【final 测试】** koopa IR 95.83  RISCV 75.00

自己测试时主要是使用了2021年老版编译原理的一些测例，以及构造一些特殊情况进行测试。用了一个小的riscv模拟器运行生成的代码，以及参考文档中的方式运行koopa IR和riscv汇编代码的结果来进行测试。

## 四、实习总结

### 4.1 收获和体会

在最初的几个level经常遇到只挂了一个点的情况，对于没有测试数据的情况 debug确实是一件比较靠运气的事，也耗费了大量的时间，感谢树洞的学长学姐们留下提示帮助我debug，临近期末周的lv8和lv9由于时间原因没有做到最完美，可能诸多细节问题考虑依然不周，所以还是一个残缺的编译器。还是有一些非常明显的问题。例如符号表数据结构虽然实现，但现有的map实现并不便于阅读，也难以进行扩展，更好的实现方式可以采用树结构单独设置结构体,利用指向父亲节点的指针记录符号表的继承信息等。

### 4.2 学习过程中的难点，以及对实习过程和内容的建议

实习和理论课、甚至实践课的内容都几乎无法搭边，一方面每周都需要交一个前后端，但是编译理论在最后才讲后端代码生成，期待助教和老师们能够找到一个合理的分阶段方案，让实践是为了巩固课程，而不是实践和课程分家。以及也可以考虑搞个答疑平台，上学期黄群老师的piazza答疑平台很赞，这样有助于大家交流，共同解决问题。

### 4.3 对老师讲解内容与方式的建议

无。
