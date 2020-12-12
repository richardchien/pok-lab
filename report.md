# POK 实验报告

## 熟悉 POK

首先根据 POK 官网的教程，下载代码、安装依赖、编译 semaphore 样例，QEMU 运行表现的行为和教程提供的截图基本相似，但存在一个问题，就是分区 2 在很久之后才会被调度（表现为打印出 `P2T1: begin of task`），于是研究了一下其时钟中断和调度的逻辑，发现它对时钟的设置相当具有迷惑性，时间的单位也不是很合理。

为了之后编写测试程序和调度算法更方便，对 POK 的时钟中断处理逻辑做了些简单修改，使时钟中断每 1 ms 触发一次，算作一个 tick，每经过 `POK_SCHED_INTERVAL`（20）个 tick 执行一次调度函数（`pok_sched`）；另外，把线程的 `period` `time_capacity` `remaining_time_capacity` 等属性、`pok_thread_sleep` 等函数的参数的单位都规范成了 tick，方便阅读和计算。

然后把 semaphore 样例稍作修改，称为 helloworld，运行输出如下（`Thread blah blah` 是在调度函数中输出的调试信息），证明上述修改有效：

```
[P1] pok_sem_create return=0, mid=0
[P1] pok_thread_create (1) return=0
[P1] pok_thread_create (2) return=0
Thread 0.1 scheduled at 23
P1T1: I will signal semaphores
P1T1: pok_sem_signal, ret=0
Thread 0.1 running at 25
Thread 0.2 scheduled at 25
P1T2: I will wait for the semaphores
P1T2: pok_sem_wait, ret=0
Thread 0.2 running at 28
Thread 0.6 scheduled at 28
[P2] thread create returns=0
Thread 1.1 scheduled at 200
P2T1: begin of task
Thread 1.1 running at 201
Thread 1.3 scheduled at 201
Thread 1.1 scheduled at 720
P2T1: begin of task
Thread 1.1 running at 720
Thread 1.3 scheduled at 720
Thread 1.1 scheduled at 1220
P2T1: begin of task
Thread 1.1 running at 1220
Thread 1.3 scheduled at 1220
```

为了验证 `pok_elect_thread` 函数能够正确工作，编写了一个只有一个线程（除 idle 和 kernel 线程外）的测试程序，线程无限循环，模拟一直在产生、执行任务的程序，创建线程时给定的参数如下：

```cpp
tattr.period = 500; // 任务周期 500 tick
tattr.time_capacity = 200; // 任务耗时 200 tick
tattr.deadline = 500; // 任务 deadline 500 tick
tattr.entry = task;
pok_thread_create(&tid, &tattr);
```

运行输出：

```
Thread 0.1 scheduled at 20
Thread 0.1 running at 40
Thread 0.1 running at 60
Thread 0.1 running at 80
Thread 0.1 running at 100
Thread 0.1 running at 120
Thread 0.1 running at 140
Thread 0.1 running at 160
Thread 0.1 running at 180
Thread 0.1 running at 200
Thread 0.1 running at 220
Thread 0.1 finished at 220, next activation: 500
Thread 0.3 scheduled at 220
Thread 0.1 activated at 500
Thread 0.1 scheduled at 500
Thread 0.1 running at 520
Thread 0.1 running at 540
Thread 0.1 running at 560
Thread 0.1 running at 580
Thread 0.1 running at 600
Thread 0.1 running at 620
Thread 0.1 running at 640
Thread 0.1 running at 660
Thread 0.1 running at 680
Thread 0.1 running at 700
Thread 0.1 finished at 700, next activation: 1000
```

可以看出线程 1（输出为 `Thread 0.1`）每 500 tick 调度一次，每次运行 200 tick；一个周期内，任务执行完成后，会切到 idle 线程（`Thread 0.3`），直到下一个任务到达（next activation）。

## 多线程调度

> 作业 1，多线程调度

### 抢占式优先级调度

创建三个线程模拟三个不同优先级、周期、执行时间的任务：

```cpp
// 线程 1
tattr.period = 1000;
tattr.time_capacity = 100;
tattr.priority = 99;
tattr.entry = task;
pok_thread_create(&tid, &tattr);

// 线程 2
tattr.period = 800;
tattr.time_capacity = 200;
tattr.priority = 50;
tattr.entry = task;
pok_thread_create(&tid, &tattr);

// 线程 3
tattr.period = 1000;
tattr.time_capacity = 300;
tattr.priority = 20;
tattr.entry = task;
pok_thread_create(&tid, &tattr);
```

使用默认调度函数进行调度，输出如下：

```
Thread 0.1 scheduled at 20
Thread 0.1 running at 40
... (省略)
Thread 0.1 finished at 120, next activation: 1000
Thread 0.2 scheduled at 120
Thread 0.2 running at 140
... (省略)
Thread 0.2 finished at 320, next activation: 800
Thread 0.3 scheduled at 320
Thread 0.3 running at 340
... (省略)
Thread 0.3 finished at 620, next activation: 1000
Idle at 620...
Thread 0.2 activated at 800
Thread 0.2 scheduled at 800
Thread 0.2 running at 820
... (省略)
Thread 0.1 activated at 1000
Thread 0.3 activated at 1000
Thread 0.2 running at 1000
Thread 0.2 finished at 1000, next activation: 1600
Thread 0.3 scheduled at 1000
Thread 0.3 running at 1020
... (省略)
Thread 0.3 finished at 1300, next activation: 2000
```

在 1000 tick 时，线程 1 和 3 都激活了，同时线程 2 当前任务完成，这时需从线程 1、3 中选一个调度，按默认调度策略，选择了“下一个就绪线程”即线程 3，而如果要按优先级，则需调度线程 1（线程 1 的优先级 99 大于线程 3 的优先级 20）。

实现优先级调度函数 `pok_lab_sched_part_prio` 如下：

```cpp
uint32_t pok_lab_sched_part_prio(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                 const uint32_t current_thread) {
    uint32_t t, from;
    uint8_t max_prio = 0;
    uint32_t max_prio_thread = IDLE_THREAD;

    if (current_thread == IDLE_THREAD) {
        from = t = prev_thread;
    } else {
        from = t = current_thread;
    }

    /*
     * Walk through all threads of the given partition,
     * select the one with highest priority.
     */
    do {
        if (pok_threads[t].state == POK_STATE_RUNNABLE && pok_threads[t].priority > max_prio) {
            max_prio = pok_threads[t].priority;
            max_prio_thread = t;
        }
        t = index_low + (t - index_low + 1) % (index_high - index_low);
    } while (t != from);

    return max_prio_thread;
}
```

使用此调度函数再次运行测试程序，输出如下：

```
Thread 0.1 scheduled at 20
Thread 0.1 running at 40
... (省略)
Thread 0.1 finished at 120, next activation: 1000
Thread 0.2 scheduled at 120
Thread 0.2 running at 140
... (省略)
Thread 0.2 finished at 320, next activation: 800
Thread 0.3 scheduled at 320
Thread 0.3 running at 340
... (省略)
Thread 0.3 finished at 620, next activation: 1000
Idle at 620...
Thread 0.2 activated at 800
Thread 0.2 scheduled at 800
Thread 0.2 running at 820
... (省略)
Thread 0.1 activated at 1000
Thread 0.3 activated at 1000
Thread 0.2 running at 1000
Thread 0.2 finished at 1000, next activation: 1600
Thread 0.1 scheduled at 1000
Thread 0.1 running at 1020
... (省略)
Thread 0.1 finished at 1100, next activation: 2000
Thread 0.3 scheduled at 1100
Thread 0.3 running at 1120
... (省略)
```

关注第 1000 tick 处，可以看到线程 1 被正确地调度了。
