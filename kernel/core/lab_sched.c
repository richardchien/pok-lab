#include <core/lab_sched.h>

#include <core/time.h>
#include <core/sched.h>
#include <core/thread.h>
#include <core/partition.h>

typedef int thread_comparator_fn(uint32_t t1, uint32_t t2);

static uint32_t select_thread_by_property(thread_comparator_fn property_cmp, const uint32_t index_low,
                                          const uint32_t index_high, const uint32_t prev_thread,
                                          const uint32_t current_thread) {
    uint32_t t, from;
    uint32_t max_property_thread = IDLE_THREAD;

    if (current_thread == IDLE_THREAD) {
        from = t = prev_thread;
    } else {
        from = t = current_thread;
    }

    /*
     * Walk through all threads of the given partition,
     * select the one with highest property (may be priority or deadline, etc).
     */
    do {
        if (pok_threads[t].state == POK_STATE_RUNNABLE && property_cmp(t, max_property_thread) > 0) {
            max_property_thread = t;
        }
        t = index_low + (t - index_low + 1) % (index_high - index_low);
    } while (t != from);

    return max_property_thread;
}

static int priority_cmp(uint32_t t1, uint32_t t2) {
    /* Select the thread with highest priority */
    return pok_threads[t1].priority - pok_threads[t2].priority;
}

uint32_t pok_lab_sched_part_prio(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                 const uint32_t current_thread) {
    return select_thread_by_property(priority_cmp, index_low, index_high, prev_thread, current_thread);
}

static int deadline_cmp(uint32_t t1, uint32_t t2) {
    /* Handle threads that don't have deadlines */
    if (pok_threads[t1].deadline == 0) return -1;
    if (pok_threads[t2].deadline == 0) return 1;
    /* Select the thread with earliest deadline */
    return pok_threads[t2].current_deadline - pok_threads[t1].current_deadline;
}

uint32_t pok_lab_sched_part_edf(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                const uint32_t current_thread) {
    return select_thread_by_property(deadline_cmp, index_low, index_high, prev_thread, current_thread);
}

uint32_t pok_lab_sched_part_rr(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                               const uint32_t current_thread) {
    uint32_t res;
    uint32_t from;

    if (current_thread == IDLE_THREAD) {
        res = prev_thread;
    } else {
        if (pok_threads[current_thread].rr_budget > 0) {
            pok_threads[current_thread].rr_budget--;
        }
        res = current_thread;
    }

    if (pok_threads[current_thread].state == POK_STATE_RUNNABLE
        && pok_threads[current_thread].remaining_time_capacity > 0 && pok_threads[current_thread].rr_budget > 0) {
        /* The current thread still has budget, let it run */
        return current_thread;
    }

    from = res;
    do {
        res = index_low + (res - index_low + 1) % (index_high - index_low);
    } while ((res != from) && (pok_threads[res].state != POK_STATE_RUNNABLE));

    if ((res == from) && (pok_threads[res].state != POK_STATE_RUNNABLE)) {
        res = IDLE_THREAD;
    }

    if (res != IDLE_THREAD) {
        /* Refill RR budget for the selected thread */
        pok_threads[res].rr_budget = POK_LAB_SCHED_RR_BUDGET;
    }

    return res;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

uint32_t pok_lab_sched_part_wrr(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                const uint32_t current_thread) {
    return IDLE_THREAD;
}

#pragma GCC diagnostic pop
