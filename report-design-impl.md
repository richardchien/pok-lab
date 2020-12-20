# 设计与实现报告

## 熟悉 POK

首先根据 POK 官网的教程，下载代码、安装依赖、编译 semaphore 样例，QEMU 运行表现的行为和教程提供的截图基本相似，但存在一个问题，就是分区 2 在很久之后才会被调度（表现为打印出 `P2T1: begin of task`），于是研究了一下其时钟中断和调度的逻辑，发现它对时钟的设置相当具有迷惑性，时间的单位也不是很合理。

为了之后编写测试程序和调度算法更方便，对 POK 的时钟中断处理逻辑做了些简单修改，使时钟中断每 1 ms 触发一次，算作一个 tick，每经过 `POK_SCHED_INTERVAL`（20）个 tick 执行一次调度函数（`pok_sched`）；另外，把线程的 `period` `time_capacity` `remaining_time_capacity` 等属性、`pok_thread_sleep` 等函数的参数的单位都规范成了 tick，方便阅读和计算。

然后把 semaphore 样例稍作修改，称为 helloworld（`mygarbage/0_helloworld`），运行输出如下（`Thread blah blah` 是在调度函数中输出的调试信息），证明上述修改有效：

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

为了验证 `pok_elect_thread` 函数能够正确工作，编写了一个只有一个线程（除 idle 和 kernel 线程外）的测试程序（`mygarbage/1_single_thread`），线程无限循环，模拟一直在产生、执行任务的程序，创建线程时给定的参数如下：

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

### Round-Robin 调度

POK 默认的调度函数并不是真的 RR 调度，它是调度一个线程后，让它执行完 `time_capacity` 然后再调度下一个，这里我们把 `time_capacity` 属性理解为线程的每一个任务周期中所需执行的时间，而不是时间片长度。因此，为了实现 RR 调度，我们给线程结构体添加了一个属性，称为 `rr_budget`，这个属性表示一个线程每次被调度后允许执行的时间片数量，而一个时间片就是一次调度周期（`POK_SCHED_INTERVAL`，即 20 tick）。每次线程被调度后，将给它的 `rr_budget` 重置为初始值（通过 `POK_LAB_SCHED_RR_BUDGET` 宏设置，默认为 3），时间片用完后，会切到下一个就绪线程。

实现 RR 调度函数 `pok_lab_sched_part_rr` 如下（除了添加 `rr_budget` 属性外，不需要对内核其它地方做修改）：

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

### Weighted Round-Robin 调度

WRR 调度算法是在 RR 调度基础上为线程加入了权重属性，为了实现 WRR 调度，我们给线程结构体添加了 `weight` 属性，并做了相应的初始化工作。

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

## 设计实现应用场景

> 作业 2

我们设计了一个应用场景：汽车驾驶。其中包含三个实时进程：

1. prog_ctrl：控制进程，用于处理汽车的控制信号，优先级最高，有硬实时要求，绝对不能miss deadline；
2. prog_net：网络进程，用于收发诸如导航之类的网络信息，优先级次高，软实时要求，可以容许一定的miss率；
3. prog_video：视频进程，用于在屏幕上显示UI、导航等信息，优先级最低，软实时要求，但耗时较长。

我们使用多分区单线程和单分区多线程两种方式模拟了这个应用场景。

多分区单线程：

prog_ctrl（控制进程）：

```cpp
int main() {
    uint32_t tid;
    pok_thread_attr_t tattr;
    memset(&tattr, 0, sizeof(pok_thread_attr_t));

    tattr.time_capacity = 80;
    tattr.period = 1200;
    tattr.deadline = 300;
    tattr.entry = task;
    pok_thread_create(&tid, &tattr);

    pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite();
    return 0;
}

static void task() {
    printf("[APP_3 Prog_Ctrl]: Control!\n");
    for (;;) {
    }
}
```

prog_net（网络进程）：

```cpp
int main() {
    uint32_t tid;
    pok_thread_attr_t tattr;
    memset(&tattr, 0, sizeof(pok_thread_attr_t));

    tattr.time_capacity = 50;
    tattr.period = 300;
    tattr.deadline = 200;
    tattr.entry = task;
    pok_thread_create(&tid, &tattr);

    pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite();
    return 0;
}

static void task() {
    printf("[APP_3 Prog_Net]: Net!\n");
    for (;;) {
    }
}
```

prog_video（视频进程）：

```cpp
int main() {
    uint32_t tid;
    pok_thread_attr_t tattr;
    memset(&tattr, 0, sizeof(pok_thread_attr_t));

    tattr.time_capacity = 200;
    tattr.period = 900;
    tattr.deadline = 900;
    tattr.entry = task;
    pok_thread_create(&tid, &tattr);

    pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite();
    return 0;
}

static void task() {
    printf("[APP_3 Prog_Video]: Video!\n");
    for (;;) {
    }
}
```

测试结果：

由于pok系统的分区设计，只有在参数精心设计的情况下，才能满足实时性要求。否则会出现某一或某些任务周期性miss的现象。

单分区多线程：

```cpp
int main() {
    uint32_t tid;
    pok_thread_attr_t tattr;
    memset(&tattr, 0, sizeof(pok_thread_attr_t));

    tattr.priority = 40;
    tattr.period = 3000;
    tattr.time_capacity = 300;
    tattr.deadline = 500;
    tattr.entry = task_ctrl;
    pok_thread_create(&tid, &tattr);

    tattr.priority = 30;
    tattr.period = 1000;
    tattr.time_capacity = 200;
    tattr.deadline = 1000;
    tattr.entry = task_net;
    pok_thread_create(&tid, &tattr);

    tattr.priority = 10;
    tattr.period = 2000;
    tattr.time_capacity = 1000;
    tattr.deadline = 2000;
    tattr.entry = task_video;
    pok_thread_create(&tid, &tattr);

    pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite();
    return 0;
}

static void task_ctrl() {
    printf("[APP_4 task_ctrl]: Control!\n");
    for (;;) {
    }
}

static void task_net() {
    printf("[APP_4 task_net]: Net!\n");
    for (;;) {
    }
}

static void task_video() {
    printf("[APP_4 task_video]: Video!\n");
    for (;;) {
    }
}
```

测试结果：

我们调整了若干次参数以模拟不同情景下的调度，结果均能较好完成调度任务。pok的同一分区的线程之间的切换是受调度策略控制的。所以我们尝试了RR、优先级、EDF调度，发现优先级调度和EDF调度能更好地满足我们设计地应用场景下的实时性需求。

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

由于之前的实验中在调度函数中输出了一些调试信息，运行时可能显得比较混乱，于是加了一个宏 `POK_NEEDS_SCHED_VERBOSE` 用于控制调度的调试信息的输出，在本题实验的测试程序中不需要开启。

为了更好的查看线程的创建和运行情况，添加了一个系统调用 `POK_SYSCALL_TOP` 用于输出当前所有线程的状态信息，外部包装为 `pok_top` 函数，供 shell 调用。该系统调用在内核中的实现如下：

```cpp
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
