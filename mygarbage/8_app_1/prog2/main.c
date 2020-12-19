#include <libc/stdio.h>
#include <core/thread.h>
#include <core/partition.h>
#include <types.h>

static void *job1();

int main() {
    uint32_t tid;
    int ret;
    pok_thread_attr_t tattr;

    tattr.priority = 42;
    tattr.entry = job1;

    ret = pok_thread_create(&tid, &tattr);
    printf("[APP_1 P2] thread create returns=%d\n", ret);

    pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite();

    return (1);
}

static void *job1() {
    while (1) {
        printf("[APP_1 P2T1]: begin of task\n");
        pok_thread_sleep(500);
    }

    return NULL;
}
