#include <core/lab_sched.h>

#include <core/time.h>
#include <core/sched.h>
#include <core/thread.h>
#include <core/partition.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

uint32_t pok_lab_sched_part_prio(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                 const uint32_t current_thread) {
    uint32_t t, from;
    uint8_t max_prio = 0;
    uint32_t max_prio_thread = IDLE_THREAD;

    if (current_thread == IDLE_THREAD) {
        from = t = prev_thread;
    } else {
        from = t = current_thread;
    }

    /*
     * Walk through all threads of the given partition,
     * select the one with highest priority.
     */
    do {
        if (pok_threads[t].state == POK_STATE_RUNNABLE && pok_threads[t].priority > max_prio) {
            max_prio = pok_threads[t].priority;
            max_prio_thread = t;
        }
        t = index_low + (t - index_low + 1) % (index_high - index_low);
    } while (t != from);

    return max_prio_thread;
}

uint32_t pok_lab_sched_part_edf(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                const uint32_t current_thread) {
    return IDLE_THREAD;
}

uint32_t pok_lab_sched_part_rr(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                               const uint32_t current_thread) {
    return IDLE_THREAD;
}

uint32_t pok_lab_sched_part_wrr(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                const uint32_t current_thread) {
    return IDLE_THREAD;
}

#pragma GCC diagnostic pop
