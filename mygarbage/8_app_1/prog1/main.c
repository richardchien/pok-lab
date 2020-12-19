#include <libc/stdio.h>
#include <core/thread.h>
#include <core/partition.h>
#include <core/semaphore.h>
#include <types.h>

static pok_sem_id_t sid;

static void *job1();
static void *job2();

int main() {
    uint32_t tid;
    pok_ret_t ret;
    pok_thread_attr_t tattr;

    ret = pok_sem_create(&sid, 0, 50, POK_SEMAPHORE_DISCIPLINE_FIFO);
    printf("[APP_1 P1] pok_sem_create return=%d, mid=%d\n", ret, sid);

    tattr.priority = 42;
    tattr.entry = job1;

    ret = pok_thread_create(&tid, &tattr);
    printf("[APP_1 P1] pok_thread_create (1) return=%d\n", ret);

    tattr.priority = 42;
    tattr.entry = job2;

    ret = pok_thread_create(&tid, &tattr);
    printf("[APP_1 P1] pok_thread_create (2) return=%d\n", ret);

    pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite();

    return (0);
}

static void *job1() {
    pok_ret_t ret;
    while (1) {
        printf("[APP_1 P1T1]: I will signal semaphores\n");
        ret = pok_sem_signal(sid);
        printf("[APP_1 P1T1]: pok_sem_signal, ret=%d\n", ret);
        pok_thread_sleep(2000);
    }

    return NULL;
}

static void *job2() {
    pok_ret_t ret;
    while (1) {
        printf("[APP_1 P1T2]: I will wait for the semaphores\n");
        ret = pok_sem_wait(sid, 0);
        printf("[APP_1 P1T2]: pok_sem_wait, ret=%d\n", ret);
        ret = pok_sem_wait(sid, 0);
        printf("[APP_1 P1T2]: pok_sem_wait, ret=%d\n", ret);
        pok_thread_sleep(2000);
    }

    return NULL;
}
