#include <core/lab_sched.h>

#include <core/time.h>
#include <core/sched.h>
#include <core/thread.h>
#include <core/partition.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

uint32_t pok_lab_sched_part_prio(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                 const uint32_t current_thread) {
    return IDLE_THREAD;
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
