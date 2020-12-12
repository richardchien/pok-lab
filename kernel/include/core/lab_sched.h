#pragma once

#include <types.h>

uint32_t pok_lab_sched_part_prio(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                 const uint32_t current_thread);
uint32_t pok_lab_sched_part_edf(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                const uint32_t current_thread);
uint32_t pok_lab_sched_part_rr(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                               const uint32_t current_thread);
uint32_t pok_lab_sched_part_wrr(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                                const uint32_t current_thread);
