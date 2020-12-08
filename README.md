# POK 大作业

## 时钟

已经修改过，现在是 1 ms 触发一次时钟中断（1 个 tick），`POK_TIMER_QUANTUM`（20）次时钟中断后触发 `pok_sched`，如果一个线程正在运行，会给 `thread->remaining_time_capacity` 减 1（初始是 `POK_THREAD_DEFAULT_TIME_CAPACITY`，目前设置为 10），然后触发分区的调度函数。也就是说一个线程在 RR 调度算法下每次被调度后可以连续运行 200 tick。

## RR 调度

默认的 RR 调度函数（`pok_sched_part_rr`）主要逻辑如下：

```c
if ((pok_threads[current_thread].remaining_time_capacity > 0)
    && (pok_threads[current_thread].state == POK_STATE_RUNNABLE)) {
    return current_thread;
}

do {
    res++;
    if (res > index_high) {
        res = index_low;
    }
} while ((res != from) && (pok_threads[res].state != POK_STATE_RUNNABLE));

if ((res == from) && (pok_threads[res].state != POK_STATE_RUNNABLE)) {
    res = IDLE_THREAD;
}
```

如果当前线程时间片还没用完，就继续运行不切换，如果已经用完，找到下一个 `POK_STATE_RUNNABLE` 状态的线程，如果没找到，就切到 idle 线程。
