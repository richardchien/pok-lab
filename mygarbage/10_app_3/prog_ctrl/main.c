#include <libc/stdio.h>
#include <libc/string.h>
#include <core/thread.h>
#include <core/partition.h>
#include <core/semaphore.h>
#include <types.h>


static void task();

int main() {
    uint32_t tid;
    pok_thread_attr_t tattr;
    memset(&tattr, 0, sizeof(pok_thread_attr_t));

    // tattr.priority = 40;
    tattr.time_capacity = 80;
    // tattr.time_capacity = 100;
    // tattr.period = 2000;
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
        // printf("[APP_3 Prog_Ctrl]: Control!\n");
    }
}
