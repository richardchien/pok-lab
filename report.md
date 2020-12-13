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
...
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

主要逻辑就是遍历当前分区的所有线程，找到优先级最高的线程执行，这里只是粗略实现，因此直接循环遍历，实际应用场景下可以使用堆或红黑树等结构实现优先级队列。

使用此调度函数再次运行测试程序，输出如下：

```
Thread 0.1 scheduled at 20
Thread 0.1 running at 40
...
Thread 0.1 finished at 120, next activation: 1000
Thread 0.2 scheduled at 120
Thread 0.2 running at 140
...
Thread 0.2 finished at 320, next activation: 800
Thread 0.3 scheduled at 320
Thread 0.3 running at 340
...
Thread 0.3 finished at 620, next activation: 1000
Idle at 620...
Thread 0.2 activated at 800
Thread 0.2 scheduled at 800
Thread 0.2 running at 820
...
Thread 0.1 activated at 1000
Thread 0.3 activated at 1000
Thread 0.2 running at 1000
Thread 0.2 finished at 1000, next activation: 1600
Thread 0.1 scheduled at 1000
Thread 0.1 running at 1020
...
Thread 0.1 finished at 1100, next activation: 2000
Thread 0.3 scheduled at 1100
Thread 0.3 running at 1120
...
```

关注第 1000 tick 处，可以看到优先级最高的线程 1 被正确地调度了。

### 抢占式 EDF 调度

由于 POK 中原先计算 deadline 的逻辑与我们的预期不符，于是给线程结构体增加了 `current_deadline` 属性，在每次线程 activate 的时候与 `next_activation` 属性一同更新，相关代码在 `pok_elect_thread` 函数，重点如下：

```cpp
if ((thread->state == POK_STATE_WAIT_NEXT_ACTIVATION) && (thread->next_activation <= now)) {
    uint64_t activation = thread->next_activation;
    uint64_t deadline = activation + thread->deadline;
    printf("Thread %u.%u activated at %u, deadline at %u\n",
            (unsigned)pok_current_partition,
            (unsigned)i,
            (unsigned)activation,
            (unsigned)deadline);
    thread->state = POK_STATE_RUNNABLE;
    thread->remaining_time_capacity = thread->time_capacity;
    thread->current_deadline = deadline;
    thread->next_activation = activation + thread->period;
}
```

编写测试程序，创建三个线程模拟不同周期、执行时间、deadline 的任务：

```cpp
// 线程 1
tattr.period = 1000;
tattr.time_capacity = 100;
tattr.deadline = 1000;
tattr.entry = task;
pok_thread_create(&tid, &tattr);

// 线程 2
tattr.period = 800;
tattr.time_capacity = 200;
tattr.deadline = 600;
tattr.entry = task;
pok_thread_create(&tid, &tattr);

// 线程 3
tattr.period = 1000;
tattr.time_capacity = 300;
tattr.deadline = 500;
tattr.entry = task;
pok_thread_create(&tid, &tattr);
```

使用默认调度函数进行调度，输出如下：

```
Thread 0.1 scheduled at 20
Thread 0.1 running at 40
...
Thread 0.1 finished at 120, deadline met, next activation: 1000
Thread 0.2 scheduled at 120
Thread 0.2 running at 140
...
Thread 0.2 finished at 320, deadline met, next activation: 800
Thread 0.3 scheduled at 320
Thread 0.3 running at 340
...
Thread 0.3 finished at 620, deadline miss, next activation: 1000
Idle at 620...
Thread 0.2 activated at 800, deadline at 1400
Thread 0.2 scheduled at 800
Thread 0.2 running at 820
...
Thread 0.1 activated at 1000, deadline at 2000
Thread 0.3 activated at 1000, deadline at 1500
Thread 0.2 running at 1000
Thread 0.2 finished at 1000, deadline met, next activation: 1600
...
```

可以看到线程 3 的第一个任务 deadline 要求 500 tick 完成，但实际到 620 tick 才完成，miss 了 deadline，但实际上这组任务是调度可行的，按 EDF 策略调度顺序为 3、2、1，三个线程都可以达到 deadline 要求。

要实现 EDF 调度，实际上就是在每次选择线程时选择当前 deadline 最近的线程，由于此逻辑与优先级调度非常接近，都是从所有线程中选择“最XX”的，于是提取了一个 `select_thread_by_property` 函数，通过传入比较函数来控制选择策略，然后利用此函数来实现 EDF 调度函数 `pok_lab_sched_part_edf`，具体实现如下：

```cpp
typedef int thread_comparator_fn(uint32_t t1, uint32_t t2);

static uint32_t select_thread_by_property(thread_comparator_fn property_cmp, const uint32_t index_low,
                                          const uint32_t index_high, const uint32_t prev_thread,
                                          const uint32_t current_thread) {
    uint32_t t, from;
    uint32_t max_property_thread = IDLE_THREAD;

    if (current_thread == IDLE_THREAD) {
        from = t = prev_thread;
    } else {
        from = t = current_thread;
    }

    /*
     * Walk through all threads of the given partition,
     * select the one with highest property (may be priority or deadline, etc).
     */
    do {
        if (pok_threads[t].state == POK_STATE_RUNNABLE && property_cmp(t, max_property_thread) > 0) {
            max_property_thread = t;
        }
        t = index_low + (t - index_low + 1) % (index_high - index_low);
    } while (t != from);

    return max_property_thread;
}

static int deadline_cmp(uint32_t t1, uint32_t t2) {
    /* Handle threads that don't have deadlines */
    if (pok_threads[t1].deadline == 0) return -1;
    if (pok_threads[t2].deadline == 0) return 1;
    /* Select the thread with earliest deadline */
    return pok_threads[t2].current_deadline - pok_threads[t1].current_deadline;
}

uint32_t pok_lab_sched_part_edf(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                const uint32_t current_thread) {
    return select_thread_by_property(deadline_cmp, index_low, index_high, prev_thread, current_thread);
}
```

使用 EDF 调度函数再次运行测试程序，输出如下：

```
Thread 0.3 scheduled at 20
Thread 0.3 running at 40
...
Thread 0.3 finished at 320, deadline met, next activation: 1000
Thread 0.2 scheduled at 320
Thread 0.2 running at 340
...
Thread 0.2 finished at 520, deadline met, next activation: 800
Thread 0.1 scheduled at 520
Thread 0.1 running at 540
...
Thread 0.1 finished at 620, deadline met, next activation: 1000
Idle at 620...
Thread 0.2 activated at 800, deadline at 1400
Thread 0.2 scheduled at 800
Thread 0.2 running at 820
...
Thread 0.1 activated at 1000, deadline at 2000
Thread 0.3 activated at 1000, deadline at 1500
Thread 0.2 running at 1000
Thread 0.2 finished at 1000, deadline met, next activation: 1600
...
```

可以发现调度顺序符合 EDF 策略，所有线程的 deadline 都能够满足。

### Round-Robin 调度

POK 默认的调度函数并不是真的 RR 调度，它是调度一个线程后，让它执行完 `time_capacity` 然后再调度下一个，这里我们把 `time_capacity` 属性理解为线程的每一个任务周期中所需执行的时间，而不是时间片长度。因此，为了实现 RR 调度，我们给线程结构体添加了一个属性，称为 `rr_budget`，这个属性表示一个线程每次被调度后允许执行的时间片数量，而一个时间片就是一次调度周期（`POK_SCHED_INTERVAL`，即 20 tick）。每次线程被调度后，将给它的 `rr_budget` 重置为初始值（通过 `POK_LAB_SCHED_RR_BUDGET` 宏设置，默认为 3），时间片用完后，会切到下一个就绪线程。

和前面一样创建三个线程如下：

```cpp
// 线程 1
tattr.period = 1000;
tattr.time_capacity = 100;
tattr.entry = task;
pok_thread_create(&tid, &tattr);

// 线程 2
tattr.period = 800;
tattr.time_capacity = 200;
tattr.entry = task;
pok_thread_create(&tid, &tattr);

// 线程 3
tattr.period = 1000;
tattr.time_capacity = 300;
tattr.entry = task;
pok_thread_create(&tid, &tattr);
```

然后实现 RR 调度函数 `pok_lab_sched_part_rr` 如下（除了添加 `rr_budget` 属性外，不需要对内核其它地方做修改）：

```cpp
uint32_t pok_lab_sched_part_rr(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                               const uint32_t current_thread) {
    uint32_t res;
    uint32_t from;

    if (current_thread == IDLE_THREAD) {
        res = prev_thread;
    } else {
        if (pok_threads[current_thread].rr_budget > 0) {
            pok_threads[current_thread].rr_budget--;
        }
        res = current_thread;
    }

    if (pok_threads[current_thread].state == POK_STATE_RUNNABLE
        && pok_threads[current_thread].remaining_time_capacity > 0 && pok_threads[current_thread].rr_budget > 0) {
        /* The current thread still has budget, let it run */
        return current_thread;
    }

    from = res;
    do {
        res = index_low + (res - index_low + 1) % (index_high - index_low);
    } while ((res != from) && (pok_threads[res].state != POK_STATE_RUNNABLE));

    if ((res == from) && (pok_threads[res].state != POK_STATE_RUNNABLE)) {
        res = IDLE_THREAD;
    }

    if (res != IDLE_THREAD) {
        /* Refill RR budget for the selected thread */
        pok_threads[res].rr_budget = POK_LAB_SCHED_RR_BUDGET;
    }

    return res;
}
```

这里所做的事情是根据线程类型，递减 `rr_budget`，然后判断当前线程是否还是就绪状态并且还有时间片，如果有就让它继续执行，否则切到下一个就绪线程，并重置 `rr_budget`。

运行效果如下：

```
Thread 0.1 scheduled at 20
Thread 0.1 running at 40
Thread 0.1 running at 60
Thread 0.1 running at 80
Thread 0.2 scheduled at 80
Thread 0.2 running at 100
Thread 0.2 running at 120
Thread 0.2 running at 140
Thread 0.3 scheduled at 140
Thread 0.3 running at 160
Thread 0.3 running at 180
Thread 0.3 running at 200
Thread 0.1 scheduled at 200
Thread 0.1 running at 220
Thread 0.1 running at 240
Thread 0.1 finished at 240, next activation: 1000
Thread 0.2 scheduled at 240
Thread 0.2 running at 260
...
```

可以看出每个线程被调度后执行了三个时间片，然后切到下一个线程执行。

### Weighted Round-Robin 调度

WRR 调度算法是在 RR 调度基础上为线程加入了权重属性，为了实现 WRR 调度，我们给线程结构体添加了 `weight` 属性，并做了相应的初始化工作。

然后编写测试程序，同样和前面类似地创建三个线程，分别指定了不同的权重：

```cpp
// 线程 1
tattr.period = 1000;
tattr.time_capacity = 100;
tattr.weight = 1;
tattr.entry = task;
pok_thread_create(&tid, &tattr);

// 线程 2
tattr.period = 800;
tattr.time_capacity = 200;
tattr.weight = 2;
tattr.entry = task;
pok_thread_create(&tid, &tattr);

// 线程 3
tattr.period = 1000;
tattr.time_capacity = 300;
tattr.weight = 3;
tattr.entry = task;
pok_thread_create(&tid, &tattr);
```

根据 WRR 算法的预期，这三个线程的时间片长度比例应该是 1:2:3。

接着实现 WRR 调度算法，由于该算法的逻辑与 RR 几乎相同，只是在重置 `rr_budget` 时所设置的值不同，于是提取了函数 `select_thread_rr`，接受一个函数指针参数用于计算重置 `rr_budget` 时应设置的值，然后可使 RR 和 WRR 算法复用代码，具体实现如下：

```cpp
typedef uint64_t rr_budget_getter_fn(uint32_t t);

static uint32_t select_thread_rr(rr_budget_getter_fn budget_getter, const uint32_t index_low, const uint32_t index_high,
                                 const uint32_t prev_thread, const uint32_t current_thread) {
    uint32_t res;
    uint32_t from;

    if (current_thread == IDLE_THREAD) {
        res = prev_thread;
    } else {
        if (pok_threads[current_thread].rr_budget > 0) {
            pok_threads[current_thread].rr_budget--;
        }
        res = current_thread;
    }

    if (pok_threads[current_thread].state == POK_STATE_RUNNABLE
        && pok_threads[current_thread].remaining_time_capacity > 0 && pok_threads[current_thread].rr_budget > 0) {
        /* The current thread still has budget, let it run */
        return current_thread;
    }

    from = res;
    do {
        res = index_low + (res - index_low + 1) % (index_high - index_low);
    } while ((res != from) && (pok_threads[res].state != POK_STATE_RUNNABLE));

    if ((res == from) && (pok_threads[res].state != POK_STATE_RUNNABLE)) {
        res = IDLE_THREAD;
    }

    if (res != IDLE_THREAD) {
        /* Refill RR budget for the selected thread */
        pok_threads[res].rr_budget = budget_getter(res);
    }

    return res;
}

static uint64_t wrr_budget_getter(uint32_t t) {
    return pok_threads[t].weight * POK_LAB_SCHED_RR_BUDGET;
}

uint32_t pok_lab_sched_part_wrr(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                const uint32_t current_thread) {
    return select_thread_rr(wrr_budget_getter, index_low, index_high, prev_thread, current_thread);
}
```

运行测试程序效果如下：

```
Thread 0.1 scheduled at 20
Thread 0.1 running at 40
Thread 0.1 running at 60
Thread 0.1 running at 80
Thread 0.2 scheduled at 80
Thread 0.2 running at 100
Thread 0.2 running at 120
Thread 0.2 running at 140
Thread 0.2 running at 160
Thread 0.2 running at 180
Thread 0.2 running at 200
Thread 0.3 scheduled at 200
Thread 0.3 running at 220
Thread 0.3 running at 240
Thread 0.3 running at 260
Thread 0.3 running at 280
Thread 0.3 running at 300
Thread 0.3 running at 320
Thread 0.3 running at 340
Thread 0.3 running at 360
Thread 0.3 running at 380
Thread 0.1 scheduled at 380
Thread 0.1 running at 400
Thread 0.1 running at 420
Thread 0.1 finished at 420, next activation: 1000
Thread 0.2 scheduled at 420
Thread 0.2 running at 440
...
```

可以发现每次被调度后，线程 1 执行了 3 个时间片，线程 2 执行了 6 个时间片，线程 3 执行了 9 个时间片，结果符合预期。

## 多分区调度

> 作业 1，多分区调度

### Weight Round-Robin 调度

POK 默认的分区调度策略是通过 `deployment.h` 中的宏定义分区调度槽，根据每个槽的时间长度和指定的分区号来调度，我们这里选择实现一个 WRR 调度算法作为不一样的调度策略，在 `deployment.h` 中不再需要定义调度槽，而是需要定义每个分区的权重，通过 `POK_CONFIG_PARTITIONS_WEIGHT` 宏给出，然后仍然像默认一样需要定义 major frame 长度 `POK_CONFIG_SCHEDULING_MAJOR_FRAME`，通过这两个宏，我们在调度器初始化时为每个分区计算出了其在一个 major frame 中允许被调度的时间长度（复用了原来的 `pok_sched_slots` 和 `pok_sched_slots_allocation` 全局变量）。

具体地，我们在 `pok_sched_init` 函数中根据宏定义存在与否情况的不同，做了区别处理，以保证完全兼容此前依赖默认调度策略的程序，主要代码如下：

```cpp
void pok_sched_init(void) {
#ifdef POK_NEEDS_PARTITIONS
#if defined(POK_NEEDS_ERROR_HANDLING) || defined(POK_NEEDS_DEBUG)

#if defined(POK_CONFIG_PARTITIONS_WEIGHT) && (POK_CONFIG_SCHEDULING_NBSLOTS == POK_CONFIG_NB_PARTITIONS)
    /*
     * Calculate scheduling slots according to POK_CONFIG_PARTITIONS_WEIGHT
     */
    uint64_t total_weight = 0;
    for (uint8_t i = 0; i < POK_CONFIG_NB_PARTITIONS; i++) {
        if (pok_sched_weights[i] == 0) pok_sched_weights[i] = 1;
        total_weight += pok_sched_weights[i];
        pok_sched_slots_allocation[i] = i;
    }
    uint64_t major_frame_remain = POK_CONFIG_SCHEDULING_MAJOR_FRAME;
    for (uint8_t i = 0; i < POK_CONFIG_NB_PARTITIONS - 1; i++) {
        pok_sched_slots[i] = pok_sched_weights[i] * POK_CONFIG_SCHEDULING_MAJOR_FRAME / total_weight;
        major_frame_remain -= pok_sched_slots[i];
    }
    pok_sched_slots[POK_CONFIG_NB_PARTITIONS - 1] = major_frame_remain;
#else
    // ... 省略 POK 默认策略进行的相关检查
#endif

#endif
#endif

    pok_sched_current_slot = 0;
    pok_sched_next_major_frame = POK_CONFIG_SCHEDULING_MAJOR_FRAME;
    pok_sched_next_deadline = pok_sched_slots[0];
    pok_sched_next_flush = 0;
    pok_current_partition = pok_sched_slots_allocation[0];
}
```

这里对 `pok_sched_slots` 和 `pok_sched_slots_allocation` 依照 `POK_CONFIG_PARTITIONS_WEIGHT` 进行配置后，可以完全复用 POK 默认的分区调度逻辑。

编写测试程序，创建三个分区，分区分别有 2、1、1 个线程，代码如下：

```cpp
// prog1/main.c 分区 1

// 线程 1.1
tattr.period = 1000;
tattr.time_capacity = 100;
tattr.entry = task;
pok_thread_create(&tid, &tattr);

// 线程 1.2
tattr.period = 500;
tattr.time_capacity = 300;
tattr.entry = task;
pok_thread_create(&tid, &tattr);

// prog2/main.c 分区 2

// 线程 2.1
tattr.period = 1000;
tattr.time_capacity = 500;
tattr.entry = task;
pok_thread_create(&tid, &tattr);

// prog3/main.c 分区 3

// 线程 3.1
tattr.period = 800;
tattr.time_capacity = 200;
tattr.entry = task;
pok_thread_create(&tid, &tattr);
```

在 `deployment.h` 中配置如下：

```cpp
#define POK_CONFIG_NB_PARTITIONS 3
#define POK_CONFIG_PARTITIONS_WEIGHT \
    { 2, 2, 1 }
#define POK_CONFIG_SCHEDULING_MAJOR_FRAME 5000

#define POK_CONFIG_NB_THREADS 9
#define POK_CONFIG_PARTITIONS_NTHREADS \
    { 3, 2, 2 }

#include <core/schedvalues.h>
#define POK_CONFIG_PARTITIONS_SCHEDULER \
    { POK_LAB_SCHED_RR, POK_LAB_SCHED_RR, POK_LAB_SCHED_RR }
```

这里设置了 major frame 为 3000 tick，三个分区的权重比例为 2:2:1，因此每个 major frame 内三个分区分别可以执行 1200、1200、600 tick，每个分区内部都使用 RR 调度算法。

运行程序效果如下：

```
Thread 1.1 scheduled at 20
Thread 1.1 running at 40
Thread 1.1 running at 60
Thread 1.1 running at 80
Thread 1.2 scheduled at 80
Thread 1.2 running at 100
Thread 1.2 running at 120
Thread 1.2 running at 140
Thread 1.1 scheduled at 140
Thread 1.1 running at 160
Thread 1.1 running at 180
Thread 1.1 finished at 180, next activation: 1000
Thread 1.2 scheduled at 180
Thread 1.2 running at 200
...
Thread 1.2 finished at 420, next activation: 500
Idle at 420...
Thread 1.2 activated at 500
Thread 1.2 scheduled at 500
Thread 1.2 running at 520
...
Thread 1.2 finished at 800, next activation: 1000
Idle at 800...
Thread 1.1 activated at 1000
Thread 1.2 activated at 1000
Thread 1.1 scheduled at 1000
Thread 1.1 running at 1020
Thread 1.1 running at 1040
Thread 1.1 running at 1060
Thread 1.2 scheduled at 1060
Thread 1.2 running at 1080
Thread 1.2 running at 1100
Thread 1.2 running at 1120
Thread 1.1 scheduled at 1120
Thread 1.1 running at 1140
Thread 1.1 running at 1160
Thread 1.1 finished at 1160, next activation: 2000
Thread 1.2 scheduled at 1160
Thread 1.2 running at 1180
Partition 2 scheduled at 1200
Thread 2.1 scheduled at 1201
Thread 2.1 running at 1220
...
Thread 2.1 finished at 1700, next activation: 1000
Idle at 1700...
Thread 2.1 activated at 1000
Thread 2.1 scheduled at 1720
Thread 2.1 running at 1740
...
Thread 2.1 finished at 2220, next activation: 2000
Idle at 2220...
Thread 2.1 activated at 2000
Thread 2.1 scheduled at 2240
Thread 2.1 running at 2260
...
Thread 2.1 running at 2380
Partition 3 scheduled at 2400
Thread 3.1 scheduled at 2401
Thread 3.1 running at 2420
...
Thread 3.1 finished at 2600, next activation: 800
Idle at 2600...
Thread 3.1 activated at 800
Thread 3.1 scheduled at 2620
Thread 3.1 running at 2640
...
Thread 3.1 finished at 2820, next activation: 1600
Idle at 2820...
Thread 3.1 activated at 1600
Thread 3.1 scheduled at 2840
Thread 3.1 running at 2860
...
Thread 3.1 running at 2980
Partition 1 scheduled at 3000
...
```

可以发现从分区 1 开始调度，分区 1 内部线程 1.1 和 1.2 采用 RR 策略交替执行；到第 1200 tick，分区 2 被调度；再到第 2400 tick，分区 3 被调度；一个 major frame 结束后，重新回到分区 1 执行。

## 动态创建线程

> 作业 3

首先，将原先使用的 `POK_CONFIG_PARTITIONS_NTHREADS` 重新理解为分区内允许创建的最大线程数量，未使用的线程结构体初始状态为 `POK_STATE_STOPPED`。`pok_partition_thread_create` 函数中原先不允许 `NORMAL` 模式的分区创建线程，需要改成允许，为了不干扰之前的其它程序，这里给 `pok_thread_attr_t` 引入了一个新的属性 `dynamic`，用于标记该线程为动态创建的线程，然后在 `pok_partition_thread_create` 做个判断：

```cpp
if ((pok_partitions[partition_id].mode != POK_PARTITION_MODE_INIT_COLD)
    && (pok_partitions[partition_id].mode != POK_PARTITION_MODE_INIT_WARM) && (!attr->dynamic)) {
    return POK_ERRNO_MODE;
}
```

在这里允许动态创建线程后，其它逻辑可以直接复用，无需更改。

接着编写测试程序，实现了一个简易的 shell 函数，可以通过输入 1、2、3 来控制创建三种预设属性的线程（当然这里可以通过解析用户输入内容来做更复杂的创建，为了简化没有做）：

```cpp
static void init_thread() {
    char buf[512];
    for (;;) {
        int buf_idx = 0;
        printf(">>> ");
        for (;;) {
            int ch = getc();
            putc(ch);
            if (ch == '\r' || ch == '\n') {
                printf("\r\n");
                buf[buf_idx] = '\0';
                break;
            } else {
                buf[buf_idx++] = (char)ch;
            }
        }
        if (buf_idx == 0) {
            continue;
        }

        if (0 == strcmp("quit", buf)) {
            break;
        }

        if (0 == strcmp("1", buf)) {
            create_task(1000, 100);
        } else if (0 == strcmp("2", buf)) {
            create_task(800, 400);
        } else if (0 == strcmp("3", buf)) {
            create_task(600, 200);
        }
    }

    printf("You quited.\n");
    pok_thread_wait_infinite();
}

static void create_task(uint64_t period, uint64_t time_capacity) {
    pok_thread_attr_t tattr;
    memset(&tattr, 0, sizeof(pok_thread_attr_t));

    tattr.dynamic = TRUE;
    tattr.period = period;
    tattr.time_capacity = time_capacity;
    tattr.entry = task;

    uint8_t tid;
    pok_ret_t ret;
    ret = pok_thread_create(&tid, &tattr);
    if (ret == POK_ERRNO_OK) {
        printf("Thread %u created, period: %u, time capacity: %u.\n",
               (unsigned)tid,
               (unsigned)period,
               (unsigned)time_capacity);
    } else if (ret == POK_ERRNO_TOOMANY) {
        printf("Error: too many thread.\n");
    } else {
        printf("Unknown error occurred.\n");
    }
}
```

在分区的主线程中首先启动一个 init 线程作为 shell，执行 `init_thread` 函数：

```cpp
tattr.period = -1;
tattr.time_capacity = -1;
tattr.entry = init_thread;
pok_thread_create(&tid, &tattr);
```

此时运行测试程序，就可以通过输入 1、2、3 来动态创建线程了，由于之前的实验中在调度函数中输出了一些调试信息，这里显得比较混乱，于是加了一个宏 `POK_NEEDS_SCHED_VERBOSE` 用于控制调度的调试信息的输出，在本题实验的测试程序中不需要开启。

为了更好的查看线程的创建和运行情况，添加了一个系统调用 `POK_SYSCALL_TOP` 用于输出当前所有线程的状态信息，外部包装为 `pok_top` 函数，然后在 shell 程序中添加一个命令用于调用 `pok_top` 函数：

```cpp
// init_thread 函数

if (0 == strcmp("1", buf)) {
    create_task(1000, 100);
} else if (0 == strcmp("2", buf)) {
    create_task(800, 400);
} else if (0 == strcmp("3", buf)) {
    create_task(600, 200);
} else if (0 == strcmp("top", buf)) {
    pok_top();
}

// pok_top 函数在内核中的实现

void pok_top() {
    uint8_t last_pid = (uint8_t)-1;
    for (uint32_t i = 0; i < POK_CONFIG_NB_THREADS; i++) {
        pok_thread_t *thread = &pok_threads[i];
        if (thread->state == POK_STATE_STOPPED) {
            continue;
        }
        if (thread->partition != last_pid) {
            printf("Partition %u:\n", (unsigned)(thread->partition + 1));
            last_pid = thread->partition;
        }
        printf("  Thread %u, state: %s\n",
               (unsigned)(i - pok_partitions[thread->partition].thread_index_low),
               state_to_string(thread->state));
    }
}
```

最终运行效果如下：

```
>>> top
Partition 1:
  Thread 1, state: RUNNABLE
>>> 1
Thread 2 created, period: 1000, time capacity: 100.
>>> 1
Thread 3 created, period: 1000, time capacity: 100.
>>> top
Partition 1:
  Thread 1, state: RUNNABLE
  Thread 2, state: WAIT_NEXT_ACTIVATION
  Thread 3, state: WAIT_NEXT_ACTIVATION
>>> 3
Thread 4 created, period: 600, time capacity: 200.
>>> top
Partition 1:
  Thread 1, state: RUNNABLE
  Thread 2, state: RUNNABLE
  Thread 3, state: RUNNABLE
  Thread 4, state: RUNNABLE
```

可以看到成功实现了线程的运行时动态创建。
