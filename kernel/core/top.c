#include <core/top.h>
#include <core/thread.h>
#include <core/partition.h>
#include <libc.h>

static const char *state_to_string(pok_state_t state) {
    switch (state) {
    case POK_STATE_RUNNABLE:
        return "RUNNABLE";
    case POK_STATE_WAITING:
        return "WAITING";
    case POK_STATE_LOCK:
        return "LOCK";
    case POK_STATE_WAIT_NEXT_ACTIVATION:
        return "WAIT_NEXT_ACTIVATION";
    case POK_STATE_DELAYED_START:
        return "DELAYED_START";
    default:
        return "UNKNOWN";
    }
}

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
