# OSNotes

操作系统课程笔记，记录自[绿导师原谅你了](https://space.bilibili.com/202224425/channel/collectiondetail?sid=192498)

环境与预先执行的命令
- 环境为 `WSL2 Ubuntu on Windows 10`
  ```
  sudo apt install python3
  pip3 install -r requirements.txt
  sudo apt install graphviz
  sudo apt install qemu-system
  sudo apt install gcc-multilib
  ```
  安装perf: [csdn-link](https://blog.csdn.net/qq_17743307/article/details/123081487)

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
  ```
  python3 model-checker.py mutex-bad.py | python3 visualize.py > a.html
  python3 model-checker.py peterson-flag.py | python3 visualize.py -t > a.html
  python3 model-checker.py dekker.py | python3 visualize.py -r > a.html
  ```

## 5. 并发控制：互斥

实现互斥的根本困难：不能同时读/写共享内存（并发的假设）
- `load` 时不能写，看到的东西马上过时了
- `store` 时不能读，只能闭着眼睛动手
- 原子指令模型
  - 保证之前的 `store` 都写入内存
  - 保证 `load` / `store` 不与原子指令乱序

自旋锁 (`Spin Lock`)
- `xchg` 从硬件层面实现原子的交换指令
- 使用 `xchg` 实现互斥：多个学生上厕所问题
  - 初始时，厕所门口桌子上放把🔑
  - 想上厕所的同学
    - 闭眼使用 `xchg` 用手里的🔞交换桌子上的东西
    - 睁眼看到手头有🔑才能进厕所
  - 出厕所的同学
    - 把🔑放到桌上
- 缺陷
  - 会触发处理器间的缓存同步，延迟增加
  - 获得自旋锁的线程被系统切换出去，100%资源浪费
- 使用场景
  - 进入临界区的时间很短
  - 持有自旋锁时禁止执行流切换
- 实现
  - 等待时让给其他线程
  - 把锁的实现放到操作系统
    - `syscall(SYSCALL_lock, &lk)`
      - 如果失败，切换到其他线程
    - `syscall(SYSCALL_unlock, &lk)`
      - 释放锁，如果其他线程在等待则唤醒

RISC-V如何保证内存一致性：Load-Reserved/Store-Reserved
- LR: 读的时候加个Reserved标记，如果有中断/其他处理器访问则此标记被取消
- SR: 当Reserved标记存在时可以写入

Scalability: 性能的新维度
- 随着线程增加导致的性能变差问题，见示例代码[`sum-scalability.c`](sum-scalability.c)

互斥锁的分类
- 自旋锁（线程直接共享locked）
  - 更快的fast path
    - `xchg` 成功`->`立即进入临界区，开销小
  - 更慢的slow path
    - `xchg` 失败`->`浪费CPU自旋等待
- 睡眠锁（通过系统调用访问locked）
  - 更快的slow path
    - 上锁失败时线程不再占用CPU
  - 更慢的fast path
    - 即使上锁成功也要进出内核 `syscall`

`Futex=Spin+Mutex`
  - 使用自旋锁和睡眠锁
  - Fast path: 上锁成功立即返回
  - Slow path: 如果前面上锁失败，执行系统调用睡眠
  - 见示例代码[`sum-scalability.c`](sum-scalability.c)，修改为 `#define FUTEX`

## 6. 并发控制：同步

**问题：如何在多处理器上协同多个线程完成任务？**

条件变量（Conditional Variables，CV）
- `wait(cv, mutex)`
  - 调用时必须保证已经获得mutex
  - 释放mutex，进入睡眠状态
- `signal/notify(cv)`
  - 如果有线程正在等待cv，则唤醒其中一个线程
- `broadcast/notifyAll(cv)`
  - 唤醒全部正在等待cv的线程
- 示例代码见[`pc-cv.c`](pc-cv.c)
- 正确打开方式
    ```
    mutex_lock(&lk);
    while (!cond) {
      wait(&cv, &lk); // 等待cv并释放lk
    }
    assert(cond);
    broadcast(&cv); // 唤醒其他可能满足条件的线程
    mutex_unlock(&lk);
    ```

信号量
- 示例代码见[`pc-sem.c`](pc-sem.c)
- 在“一单位资源”明确的问题上更好用

哲学家吃饭问题
- 当左右手叉子都空闲时才获取

分布式系统中常见的解决思路
- 服务员统一管理资源的分配
- 任务队列可以实现几乎任何并行算法

## 7. 真实世界的并发编程

高性能计算的并发编程
- 以计算为中心，例如预测、模拟、AI、挖矿
- 计算任务的分解（**注重任务分解**）
  - 计算图-拓扑排序
  - 信息传递接口MPI、OpenMP等
  - 并发画图，示例代码见[`mandelbrot.c`](mandelbrot.c)

数据中心中的并发编程
- 以数据(存储)为中心（**注重系统调用**）
- 主要挑战：多副本情况下的高可靠、低延迟数据访问
  - 数据要保持一致
  - 时刻保持可用
  - 容忍机器离线
- Go和Goroutine：并行和并发全都要
  - 每个CPU绑定一个线程，每个线程有任意多的协程
  - 每个线程执行到blocking API时（例如sleep，scanf）
    - 成功`->`立即继续执行当前协程
    - 失败`->`立即yield到另一个可运行的协程
  - 使CPU利用率达到100%
  - 使用stream流实现生产者-消费者模型
    ```
    stream <- x
    y := <- stream
    ```

人机交互程序
- 不太复杂，既没太多计算，也没太多IO（**注重易用性**）
- 网页浏览器/移动应用采用异步事件模型
  - 是一种并发模型，确保线程安全
  - API比如httpget仍然可以并行
  - Async-Await
    - async function
      - 总是返回一个Promise object
    - await promise
      ```
      `promise.then(...)`
      ```

## 8. 并发BUG和应对

应对并发BUG的方法
- 软件是需求在计算机数字世界中的投影
- BUG多的根本原因：编程语言的缺陷
- 实在的方法：防御性编程
  - 加断言，加检查

并发BUG：死锁（Deadlock）
- 出现线程互相等待的情况
  - AA-Deadlock
    - 一个线程自己等待自己
    - 在不同的函数中持有并等待
  - ABBA-Deadlock
    - 上锁顺序导致持有并等待，例如哲学家吃饭
- 避免死锁
  - 死锁产生的必要条件
    - 互斥：一个资源每次只能被一个进程使用
    - 请求与保持：请求阻塞时，不释放已经获得的资源
    - 不剥夺：已获得资源不能被强行剥夺
    - 循环等待：进程间形成头尾相接的循环等待资源关系
  - AA-Deadlock
    - 示例代码[`spinlock-xv6.c`](spinlock-xv6.c)中的防御性编程
      ```
      push_off(); // 禁止中断避免死锁
      if(holding(lk)) panic("acquire");
      ```
  - ABBA-Deadlock
    - 严格按照固定顺序获取锁
    - 示例代码见[`lock-ordering.py`](lock-ordering.py)
      ```
      python3 model-checker.py lock-ordering.py | python3 visualize.py > a.html
      ```

并发BUG：数据竞争（Data Race）
- 不同线程同时访问同一段内存，且至少有一个写
- 用互斥锁保护好共享数据
- 实现并发控制的工具
  - 互斥锁（lock/unlock）：原子性
  - 条件变量（wait/signal）：同步
- 错误类型
  - 忘记上锁——原子性违反（Atomicity Violation，AV）
    ```
    Thread1                   Thread2
    S1: if (t->info) -------↘
    {                         S3: t->info = NULL;
      S2: do(t->info); ←----↙
    }
    ```
  - 忘记同步——顺序违反（Order Violation，OV）
    ```
    Thread1                   Thread2
    S1: call_thread2(); ----↘
                              S4: done = True;
    S2: done = False; ←-----↙
    S3: while (!done) {}
    ```

## 9. 操作系统的状态机模型

Bare-metal与程序员的约定
- 为了让计算机能运行任何程序，一定存在软件/硬件的约定
  - CPU reset后，处理器处于某个确定的状态
    - PC指针能读到一条有效的指令
- x86 Family：CPU Reset行为
  - 寄存器会有初始状态
    - EIP = 0x0000fff0（PC指针）
    - CR0 = 0x60000010（16-bit非保护模式）
    - EFLAGS = 0x00000002（关闭中断）
- CPU Reset之后
  - 从PC（CS:IP）指针处取指令、译码、执行...
  - 从firmware开始执行，fff0通常是向firmware跳转的jmp指令
- Legacy BOIS：约定
  - Firmware必须提供机制，将用户数据载入内存
  - 把第一个可引导设备的第一个扇区加载到物理内存的0x7c00位置
    - 此时处理器处于16-bit模式
    - 规定CS:IP = 0x7c00，即R[CS] << 4) | R[IP] = 0x7c00
    - 加载512字节到内存，然后退出
- 操作系统的状态机启动
  - Firmware和boot loader共同完成操作系统的加载
    - 初始化全局变量和栈并分配堆区
    - 为main函数传递参数
    - 最小的多线程系统，见示例代码[`thread-os.c`](thread-os.c)
      ```
      编译thread-os.c的步骤（Ubuntu WSL2）：
      1. 下载 git clone https://github.com/NJU-ProjectN/abstract-machine.git，比如存放目录是/mnt/f/OSNotes/abstract-machine
      2. 设置 export AM_HOME=/mnt/f/OSNotes/abstract-machine
      3. 设置架构，比如 export ARCH=x86_64-qemu
      4. 下载 git clone https://github.com/NJU-ProjectN/am-kernels.git
      5. 进入目录 am-kernels/kernels/thread-os
      6. 可能需要额外安装库 sudo apt install gcc-multilib
      7. 编译 make
      8. 运行并使用GDB调试
        qemu-system-x86_64 -machine accel=tcg -smp 1 -S -s -drive format=raw,file=am-kernels/kernels/thread-os/build/thread-os-$ARCH &
      ```
      查看编译命令的方法
      ```
      1. make -nB | grep -ve '^\(\#\|echo\|mkdir\|make\)' | sed "s#$AM_HOME#\$AM_HOME#g" | sed "s#$PWD#.#g" | vim -
      2. 进入vim后，输入 :%s/ /\r /g
      ```
- GDB调试QEMU
  - 打开模拟器，暂停在初始状态，并打开调试端口
    ```
    qemu-system-x86_64 -S -s
    ```
  - GDB连接调试端口
    ```
    gdb
    target remote localhost:1234
    info registers
    watch *0x7c00 # 硬件断点
    ```

## 10. 状态机模型的应用

理解编译器和现代CPU
- 编译器
  - 源代码$S$（状态机）$\rightarrow$ 二进制代码$C$（状态机）
- 编译的正确性
  - $S$ 与 $C$ 的可观测行为严格一致
- 超标量/乱序执行处理器
  - 允许在状态机上“跳跃”
  - 示例代码见[`ilp-demo.c`](ilp-demo.c)，由于超标量，执行速度远远大于主频

查看状态机执行
- Trace和调试器
  - `strace` / `gdb`
    ```
    跟踪系统调用 strace -T ./a.out |& vim -
    其中-T表示跟踪系统调用的执行时间
    ```
  - Time-Traval Debugging
    - 在时间上“后退”
    - gdb的记录与回溯功能
      - 开始记录 `record full`
      - 结束记录 `record stop`
      - 回溯调试 `reverse-step` / `reverse-stepi`
    - 示例代码见[`rdrand.c`](rdrand.c)
      ```
      gdb ./a.out
      layout src
      start
      s
      p val
      record full
      s
      p val
      layout asm
      si
      si
      p val
      rsi // 后退一步
      rsi // 后退一步
      ```
  - Record & Play
    - 记录程序执行状态与参数，结束后可以重现程序的行为
    - 只需要记录non-deterministic的指令的效果

采样状态机执行（得到执行的summary）
- Profiler和性能摘要
  - 测试[`ilp-demo.c`](ilp-demo.c)的性能
    ```
    perf stat ./a.out
    perf record ./a.out
    perf report
    ```
Model Checker / Verifier
- 检查并发程序
- 检查non-deterministic的状态机
