# OSNotes

操作系统课程笔记，记录自[绿导师原谅你了](https://space.bilibili.com/202224425/channel/collectiondetail?sid=192498)

环境与预先执行的命令
- 环境为 `WSL2 Ubuntu on Windows 10`
- `sudo apt install python3`
- `sudo apt install graphviz`
- `pip3 install -r requirements.txt`

## 1. 操作系统概述

操作系统=对象+API(应用视角/设计)=C程序(硬件视角/实现)

## 2. 程序和编译器

程序就是状态机，例如汉诺塔

C程序的状态机模型
- 状态=stack frame=ASM(内存M,寄存器R)
- 函数调用=创建新的栈帧
- 函数返回=删除栈顶的栈帧

程序运行流程：取指令，译码，执行，生成新的状态

指令有两种：计算和 `syscall`

构造最小的 `Hello World`

- `objdump -d ${PROG}` 反汇编命令
- `gdb` 指令
  - `starti` 从第一行执行
  - `layout asm` 切换到汇编视图
- 示例代码 [`minimum_hello.S`](minimum_hello.S)

编译器优化的正确性
- 除了不可优化的部分，都可以优化
- 不可优化指全局变量、`volatile`、汇编等

程序的初始状态
- `gdb` 的 `starti` 命令打印的第一条信息，例如 `0x0000000000401000 in _start ()`
- `info proc mappings` 打印进程内存
- `strace ${PROG}` 跟踪程序所有的系统调用

应用视角的操作系统
- 就是一条 `syscall` 指令

## 3. 多处理器编程

并发的基本单位：线程
- 共享内存的多个执行流（拥有独立的栈帧）
- 示例代码 [`multi_thread.c`](multi_thread.c)

实现原子性
- 问题：多发射与乱序执行导致状态机无法到达的状态
  - 示例代码 [`mem-ordering.c`](mem-ordering.c)，见`18`和`30`行
  - 汇编代码被处理器编译为更小的$\mu\rm{ops}$，每个$\mu\rm{op}$包含Fetch, Issue, Execute, Commit四个阶段
  - 每一周期尽可能多的补充$\mu\rm{op}$，指令按有向无环图的拓扑序进行乱序执行、按序提交

## 4. 并发程序执行与模型检验

正确性不明的奇怪尝试（Peterson算法）
- A和B争用厕所包间，示例代码[`peterson-simple.c`](peterson-simple.c)会出错，正确的代码[`peterson-barrier.c`](peterson-barrier.c)
  - 想进入包间之前，A和B都要先举起自己的旗子
    - A确认旗子举好后，往厕所门贴上“B正在使用”的标签
    - B确认旗子举好后，往厕所门贴上“A正在使用”的标签
  - 然后，如果对方旗子举起，且门上名字不是自己
    - 等待
    - 否则进入包间
  - 出包间后，放下自己的旗子
- 正确的前提是使用Sequential内存模型

年轻人的第一个Model Checker
- `python3 model-checker.py mutex-bad.py | python3 visualize.py > a.html`
- `python3 model-checker.py peterson-flag.py | python3 visualize.py -t > a.html`
- `python3 model-checker.py dekker.py | python3 visualize.py -r > a.html`

