#include <libc/stdio.h>
#include <libc/string.h>
#include <core/thread.h>
#include <core/partition.h>
#include <types.h>


static void task_ctrl();
static void task_net();
static void task_video();

int main() {
    uint32_t tid;
    pok_thread_attr_t tattr;
    memset(&tattr, 0, sizeof(pok_thread_attr_t));

    tattr.priority = 40;
    // tattr.period = 1000;
    tattr.period = 3000;
    // tattr.time_capacity = 100;
    tattr.time_capacity = 300;
    // tattr.deadline = 1000;
    tattr.deadline = 500;
    tattr.entry = task_ctrl;
    pok_thread_create(&tid, &tattr);

    tattr.priority = 30;
    // tattr.period = 100;
    tattr.period = 1000;
    // tattr.time_capacity = 10;
    tattr.time_capacity = 200;
    // tattr.deadline = 20;
    tattr.deadline = 1000;
    tattr.entry = task_net;
    pok_thread_create(&tid, &tattr);

    tattr.priority = 10;
    // tattr.period = 500;
    tattr.period = 2000;
    // tattr.time_capacity = 200;
    tattr.time_capacity = 1000;
    // tattr.deadline = 300;
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
