#include <libc/stdio.h>
#include <libc/string.h>
#include <core/thread.h>
#include <core/partition.h>
#include <types.h>

static void init_thread();
static void task();
static void create_task(uint64_t period, uint64_t time_capacity);

int main() {
    uint8_t tid;
    pok_thread_attr_t tattr;
    memset(&tattr, 0, sizeof(pok_thread_attr_t));

    tattr.period = -1;
    tattr.time_capacity = -1;
    tattr.entry = init_thread;
    pok_thread_create(&tid, &tattr);

    pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite();
    return 0;
}

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
        } else if (0 == strcmp("top", buf)) {
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
        printf("Thread %u created.\n", (unsigned)tid);
    } else if (ret == POK_ERRNO_TOOMANY) {
        printf("Error: too many thread.\n");
    } else {
        printf("Unknown error occurred.\n");
    }
}

static void task() {
    for (;;) {
    }
}
