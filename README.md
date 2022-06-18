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
  - 使用steam流实现生产者-消费者模型

人机交互程序
- 不太复杂，既没太多计算，也没太多IO（**注重易用性**）
- 网页浏览器采用异步事件模型
  - 是一种并发模型，确保线程安全
  - API比如httpget仍然可以并行
  - Async-Await
    - async function
      - 总是返回一个Promise object
    - await promise
      - `promise.then(...)`

