#include <libc/stdio.h>
#include <libc/string.h>
#include <core/thread.h>
#include <core/partition.h>
#include <types.h>

static void task();

int main() {
    uint8_t tid;
    pok_thread_attr_t tattr;
    memset(&tattr, 0, sizeof(pok_thread_attr_t));

    tattr.period = 1000;
    tattr.time_capacity = 100;
    tattr.deadline = 1000;
    tattr.entry = task;
    pok_thread_create(&tid, &tattr);

    tattr.period = 800;
    tattr.time_capacity = 200;
    tattr.deadline = 600;
    tattr.entry = task;
    pok_thread_create(&tid, &tattr);

    tattr.period = 1000;
    tattr.time_capacity = 300;
    tattr.deadline = 500;
    tattr.entry = task;
    pok_thread_create(&tid, &tattr);

    pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite();
    return 0;
}

static void task() {
    for (;;) {
    }
}
