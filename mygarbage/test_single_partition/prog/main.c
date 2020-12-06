#include <libc/stdio.h>
#include <core/thread.h>
#include <core/partition.h>
#include <types.h>

static void *job1();
static void *job2();

int main() {
    uint8_t tid;
    pok_ret_t ret;
    pok_thread_attr_t tattr;

    tattr.priority = 42;
    tattr.entry = job1;
    ret = pok_thread_create(&tid, &tattr);
    printf("pok_thread_create (1) return=%d\n", ret);

    tattr.priority = 42;
    tattr.entry = job2;
    ret = pok_thread_create(&tid, &tattr);
    printf("pok_thread_create (2) return=%d\n", ret);

    pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite();
    return 1;
}

static void *job1() {
    while (1) {
        printf("T1: hello from job 1\n");
        pok_thread_sleep(1000);
    }
}

static void *job2() {
    while (1) {
        printf("T2: hello from job 2\n");
        pok_thread_sleep(2000);
    }
}
