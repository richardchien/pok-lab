# 测试报告

## 多线程调度

> 作业 1，多线程调度

### 抢占式优先级调度

编写测试程序 `mygarbage/2_multi_thread_prio`，创建三个线程模拟三个不同优先级、周期、执行时间的任务：

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

使用抢占式优先级调度函数再次运行测试程序，输出如下：

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

编写测试程序 `mygarbage/3_multi_thread_edf`，创建三个线程模拟不同周期、执行时间、deadline 的任务：

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

编写测试程序 `mygarbage/4_multi_thread_rr`，创建三个线程如下：

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

编写测试程序 `mygarbage/5_multi_thread_wrr`，同样和前面类似地创建三个线程，分别指定了不同的权重：

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

编写测试程序 `mygarbage/6_multi_partition`，创建三个分区，分区分别有 2、1、1 个线程，代码如下：

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

## 测试实际应用场景

> 作业 2

多分区单线程

测试结果：满足了实时性要求
```
POK kernel initialized
Thread 1.1 scheduled at 20
[APP_3 Prog_Ctrl]: Control!
Thread 1.1 running at 40
Thread 1.1 running at 60
Thread 1.1 running at 80
Partition 2 scheduled at 100
Thread 2.1 scheduled at 100
[APP_3 Prog_Net]: Net!
Thread 2.1 running at 120
Thread 2.1 running at 140
Thread 2.1 running at 160
Thread 2.1 running at 180
Partition 3 scheduled at 200
Partition 1 scheduled at 300
Thread 1.1 running at 320
Thread 1.1 finished at 320, deadline met, next activation: 1220
Idle at 320...
Partition 2 scheduled at 400
Thread 2.1 running at 420
Thread 2.1 running at 440
Thread 2.1 running at 460
Thread 2.1 running at 480
Partition 3 scheduled at 500
Partition 1 scheduled at 600
Partition 2 scheduled at 700
Thread 2.1 running at 720
Thread 2.1 running at 740
Thread 2.1 running at 760
Thread 2.1 running at 780
Partition 3 scheduled at 800
Partition 1 scheduled at 900
Partition 2 scheduled at 1000
Thread 2.1 running at 1020
Thread 2.1 running at 1040
Thread 2.1 running at 1060
Thread 2.1 running at 1080
Partition 3 scheduled at 1100
Partition 1 scheduled at 1200
Thread 1.1 activated at 1220, deadline at 1520
Thread 1.1 scheduled at 1220
Thread 1.1 running at 1240
Thread 1.1 running at 1260
Thread 1.1 running at 1280
Partition 2 scheduled at 1300
Thread 1.1 running at 1300
Thread 1.1 finished at 1300, deadline met, next activation: 2420
```

单分区多线程
    
RR：优先级最高的控制进程(prog_ctrl)出现了DDL miss，可见RR在满足实时性方面令人难以满意。
```
POK kernel initialized
Thread 1.1 scheduled at 20
[APP_4 task_ctrl]: Control!
Thread 1.1 running at 40
Thread 1.1 running at 60
Thread 1.1 running at 80
Thread 1.2 scheduled at 80
[APP_4 task_net]: Net!
Thread 1.2 running at 100
Thread 1.2 running at 120
Thread 1.2 running at 140
Thread 1.3 scheduled at 140
[APP_4 task_video]: Video!
Thread 1.3 running at 160
Thread 1.3 running at 180
Thread 1.3 running at 200
Thread 1.1 scheduled at 200
Thread 1.1 running at 220
Thread 1.1 running at 240
Thread 1.1 running at 260
Thread 1.2 scheduled at 260
Thread 1.2 running at 280
Thread 1.2 running at 300
Thread 1.2 running at 320
Thread 1.3 scheduled at 320
Thread 1.3 running at 340
Thread 1.3 running at 360
Thread 1.3 running at 380
Thread 1.1 scheduled at 380
Thread 1.1 running at 400
Thread 1.1 running at 420
Thread 1.1 running at 440
Thread 1.2 scheduled at 440
Thread 1.2 running at 460
Thread 1.2 running at 480
Thread 1.2 running at 500
Thread 1.3 scheduled at 500
Thread 1.3 running at 520
Thread 1.3 running at 540
Thread 1.3 running at 560
Thread 1.1 scheduled at 560
Thread 1.1 running at 580
Thread 1.1 running at 600
Thread 1.1 running at 620
Thread 1.2 scheduled at 620
Thread 1.2 running at 640
Thread 1.2 finished at 640, deadline met, next activation: 1020
Thread 1.3 scheduled at 640
Thread 1.3 running at 660
Thread 1.3 running at 680
Thread 1.3 running at 700
Thread 1.1 scheduled at 700
Thread 1.1 running at 720
Thread 1.1 running at 740
Thread 1.1 running at 760
Thread 1.1 finished at 760, deadline miss, next activation: 3020
Thread 1.3 scheduled at 760
Thread 1.3 running at 780
Thread 1.3 running at 800
Thread 1.3 running at 820
Thread 1.3 running at 840
Thread 1.3 running at 860
Thread 1.3 running at 880
Thread 1.3 running at 900
Thread 1.3 running at 920
Thread 1.3 running at 940
Thread 1.3 running at 960
Thread 1.3 running at 980
Thread 1.3 running at 1000
Thread 1.2 activated at 1020, deadline at 2020
Thread 1.3 running at 1020
Thread 1.3 running at 1040
Thread 1.3 running at 1060
Thread 1.2 scheduled at 1060
Thread 1.2 running at 1080
Thread 1.2 running at 1100
Thread 1.2 running at 1120
Thread 1.3 scheduled at 1120
Thread 1.3 running at 1140
Thread 1.3 running at 1160
Thread 1.3 running at 1180
Thread 1.2 scheduled at 1180
Thread 1.2 running at 1200
Thread 1.2 running at 1220
Thread 1.2 running at 1240
Thread 1.3 scheduled at 1240
Thread 1.3 running at 1260
Thread 1.3 running at 1280
Thread 1.3 running at 1300
Thread 1.2 scheduled at 1300
Thread 1.2 running at 1320
Thread 1.2 running at 1340
Thread 1.2 running at 1360
Thread 1.3 scheduled at 1360
Thread 1.3 running at 1380
Thread 1.3 running at 1400
Thread 1.3 running at 1420
Thread 1.2 scheduled at 1420
Thread 1.2 running at 1440
Thread 1.2 finished at 1440, deadline met, next activation: 2020
Thread 1.3 scheduled at 1440
Thread 1.3 running at 1460
Thread 1.3 running at 1480
Thread 1.3 running at 1500
Thread 1.3 running at 1520
Thread 1.3 running at 1540
Thread 1.3 running at 1560
Thread 1.3 running at 1580
Thread 1.3 running at 1600
Thread 1.3 running at 1620
Thread 1.3 running at 1640
Thread 1.3 running at 1660
Thread 1.3 running at 1680
Thread 1.3 running at 1700
Thread 1.3 running at 1720
Thread 1.3 finished at 1720, deadline met, next activation: 2020
```
优先级：在我们设计的场景中，优先级调度满足了实时性要求。
```
POK kernel initialized
Thread 1.1 scheduled at 20
[APP_4 task_ctrl]: Control!
Thread 1.1 running at 40
Thread 1.1 running at 60
Thread 1.1 running at 80
Thread 1.1 running at 100
Thread 1.1 running at 120
Thread 1.1 running at 140
Thread 1.1 running at 160
Thread 1.1 running at 180
Thread 1.1 running at 200
Thread 1.1 running at 220
Thread 1.1 running at 240
Thread 1.1 running at 260
Thread 1.1 running at 280
Thread 1.1 running at 300
Thread 1.1 running at 320
Thread 1.1 finished at 320, deadline met, next activation: 3020
Thread 1.2 scheduled at 320
[APP_4 task_net]: Net!
Thread 1.2 running at 340
Thread 1.2 running at 360
Thread 1.2 running at 380
Thread 1.2 running at 400
Thread 1.2 running at 420
Thread 1.2 running at 440
Thread 1.2 running at 460
Thread 1.2 running at 480
Thread 1.2 running at 500
Thread 1.2 running at 520
Thread 1.2 finished at 520, deadline met, next activation: 1020
Thread 1.3 scheduled at 520
[APP_4 task_video]: Video!
Thread 1.3 running at 540
Thread 1.3 running at 560
Thread 1.3 running at 580
Thread 1.3 running at 600
Thread 1.3 running at 620
Thread 1.3 running at 640
Thread 1.3 running at 660
Thread 1.3 running at 680
Thread 1.3 running at 700
Thread 1.3 running at 720
Thread 1.3 running at 740
Thread 1.3 running at 760
Thread 1.3 running at 780
Thread 1.3 running at 800
Thread 1.3 running at 820
Thread 1.3 running at 840
Thread 1.3 running at 860
Thread 1.3 running at 880
Thread 1.3 running at 900
Thread 1.3 running at 920
Thread 1.3 running at 940
Thread 1.3 running at 960
Thread 1.3 running at 980
Thread 1.3 running at 1000
Thread 1.2 activated at 1020, deadline at 2020
Thread 1.3 running at 1020
Thread 1.2 scheduled at 1020
Thread 1.2 running at 1040
Thread 1.2 running at 1060
Thread 1.2 running at 1080
Thread 1.2 running at 1100
Thread 1.2 running at 1120
Thread 1.2 running at 1140
Thread 1.2 running at 1160
Thread 1.2 running at 1180
Thread 1.2 running at 1200
Thread 1.2 running at 1220
Thread 1.2 finished at 1220, deadline met, next activation: 2020
Thread 1.3 scheduled at 1220
Thread 1.3 running at 1240
Thread 1.3 running at 1260
Thread 1.3 running at 1280
Thread 1.3 running at 1300
Thread 1.3 running at 1320
Thread 1.3 running at 1340
Thread 1.3 running at 1360
Thread 1.3 running at 1380
Thread 1.3 running at 1400
Thread 1.3 running at 1420
Thread 1.3 running at 1440
Thread 1.3 running at 1460
Thread 1.3 running at 1480
Thread 1.3 running at 1500
Thread 1.3 running at 1520
Thread 1.3 running at 1540
Thread 1.3 running at 1560
Thread 1.3 running at 1580
Thread 1.3 running at 1600
Thread 1.3 running at 1620
Thread 1.3 running at 1640
Thread 1.3 running at 1660
Thread 1.3 running at 1680
Thread 1.3 running at 1700
Thread 1.3 running at 1720
Thread 1.3 finished at 1720, deadline met, next activation: 2020
Idle at 1720...
Thread 1.2 activated at 2020, deadline at 3020
Thread 1.3 activated at 2020, deadline at 4020
Thread 1.2 scheduled at 2020
Thread 1.2 running at 2040
Thread 1.2 running at 2060
Thread 1.2 running at 2080
Thread 1.2 running at 2100
Thread 1.2 running at 2120
Thread 1.2 running at 2140
Thread 1.2 running at 2160
Thread 1.2 running at 2180
Thread 1.2 running at 2200
Thread 1.2 running at 2220
Thread 1.2 finished at 2220, deadline met, next activation: 3020
```
EDF：在我们设计的场景中，EDF调度满足了实时性要求。
```
POK kernel initialized
Thread 1.1 scheduled at 20
[APP_4 task_ctrl]: Control!
Thread 1.1 running at 40
Thread 1.1 running at 60
Thread 1.1 running at 80
Thread 1.1 running at 100
Thread 1.1 running at 120
Thread 1.1 running at 140
Thread 1.1 running at 160
Thread 1.1 running at 180
Thread 1.1 running at 200
Thread 1.1 running at 220
Thread 1.1 running at 240
Thread 1.1 running at 260
Thread 1.1 running at 280
Thread 1.1 running at 300
Thread 1.1 running at 320
Thread 1.1 finished at 320, deadline met, next activation: 3020
Thread 1.2 scheduled at 320
[APP_4 task_net]: Net!
Thread 1.2 running at 340
Thread 1.2 running at 360
Thread 1.2 running at 380
Thread 1.2 running at 400
Thread 1.2 running at 420
Thread 1.2 running at 440
Thread 1.2 running at 460
Thread 1.2 running at 480
Thread 1.2 running at 500
Thread 1.2 running at 520
Thread 1.2 finished at 520, deadline met, next activation: 1020
Thread 1.3 scheduled at 520
[APP_4 task_video]: Video!
Thread 1.3 running at 540
Thread 1.3 running at 560
Thread 1.3 running at 580
Thread 1.3 running at 600
Thread 1.3 running at 620
Thread 1.3 running at 640
Thread 1.3 running at 660
Thread 1.3 running at 680
Thread 1.3 running at 700
Thread 1.3 running at 720
Thread 1.3 running at 740
Thread 1.3 running at 760
Thread 1.3 running at 780
Thread 1.3 running at 800
Thread 1.3 running at 820
Thread 1.3 running at 840
Thread 1.3 running at 860
Thread 1.3 running at 880
Thread 1.3 running at 900
Thread 1.3 running at 920
Thread 1.3 running at 940
Thread 1.3 running at 960
Thread 1.3 running at 980
Thread 1.3 running at 1000
Thread 1.2 activated at 1020, deadline at 2020
Thread 1.3 running at 1020
Thread 1.3 running at 1040
Thread 1.3 running at 1060
Thread 1.3 running at 1080
Thread 1.3 running at 1100
Thread 1.3 running at 1120
Thread 1.3 running at 1140
Thread 1.3 running at 1160
Thread 1.3 running at 1180
Thread 1.3 running at 1200
Thread 1.3 running at 1220
Thread 1.3 running at 1240
Thread 1.3 running at 1260
Thread 1.3 running at 1280
Thread 1.3 running at 1300
Thread 1.3 running at 1320
Thread 1.3 running at 1340
Thread 1.3 running at 1360
Thread 1.3 running at 1380
Thread 1.3 running at 1400
Thread 1.3 running at 1420
Thread 1.3 running at 1440
Thread 1.3 running at 1460
Thread 1.3 running at 1480
Thread 1.3 running at 1500
Thread 1.3 running at 1520
Thread 1.3 finished at 1520, deadline met, next activation: 2020
Thread 1.2 scheduled at 1520
Thread 1.2 running at 1540
Thread 1.2 running at 1560
Thread 1.2 running at 1580
Thread 1.2 running at 1600
Thread 1.2 running at 1620
Thread 1.2 running at 1640
Thread 1.2 running at 1660
Thread 1.2 running at 1680
Thread 1.2 running at 1700
Thread 1.2 running at 1720
Thread 1.2 finished at 1720, deadline met, next activation: 2020
Idle at 1720...
Thread 1.2 activated at 2020, deadline at 3020
Thread 1.3 activated at 2020, deadline at 4020
Thread 1.2 scheduled at 2020
Thread 1.2 running at 2040
Thread 1.2 running at 2060
Thread 1.2 running at 2080
Thread 1.2 running at 2100
Thread 1.2 running at 2120
Thread 1.2 running at 2140
Thread 1.2 running at 2160
Thread 1.2 running at 2180
Thread 1.2 running at 2200
Thread 1.2 running at 2220
Thread 1.2 finished at 2220, deadline met, next activation: 3020
```

## 动态创建线程

> 作业 3

编写测试程序 `mygarbage/7_dynamic_thread`，实现了一个简易的 shell 函数，可以通过输入 1、2、3 来控制创建三种预设属性的线程（当然这里可以通过解析用户输入内容来做更复杂的创建，为了简化没有做）：

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

此时运行测试程序，就可以通过输入 1、2、3 来动态创建线程了。

为了更好地查看效果，在 shell 程序中添加一个命令用于调用 `pok_top` 函数打印线程信息：

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
