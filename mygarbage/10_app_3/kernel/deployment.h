#pragma once

#define POK_NEEDS_DEBUG 1
#define POK_NEEDS_SCHED_VERBOSE 1

#define POK_NEEDS_THREADS 1
#define POK_NEEDS_PARTITIONS 1
#define POK_NEEDS_SCHED 1
#define POK_NEEDS_TIME 1
#define POK_NEEDS_CONSOLE 1
#define POK_NEEDS_LOCKOBJECTS 1

#define POK_NEEDS_THREAD_SUSPEND 1
#define POK_NEEDS_THREAD_SLEEP 1

#define POK_CONFIG_NB_PARTITIONS 3
#define POK_CONFIG_PARTITIONS_SIZE \
    { 120 * 1024, 120 * 1024, 120 * 1024 };
#define POK_CONFIG_PARTITIONS_WEIGHT \
    { 1, 1, 1 }
#define POK_CONFIG_SCHEDULING_MAJOR_FRAME 300

#define POK_CONFIG_NB_THREADS 6
#define POK_CONFIG_PARTITIONS_NTHREADS \
    { 2, 2, 2 }

#include <core/schedvalues.h>
#define POK_CONFIG_PARTITIONS_SCHEDULER \
    { POK_LAB_SCHED_EDF, POK_LAB_SCHED_EDF, POK_LAB_SCHED_EDF }

#define POK_CONFIG_NB_LOCKOBJECTS 0
#define POK_CONFIG_PARTITIONS_NLOCKOBJECTS \
    { 0, 0, 0 }




#if 0

// time unit: ms
#define POK_CONFIG_SCHEDULING_SLOTS \
    { 200, 4000, 100, 1000 }
#define POK_CONFIG_SCHEDULING_MAJOR_FRAME 5300
#define POK_CONFIG_SCHEDULING_SLOTS_ALLOCATION \
    { 0, 1, 0, 1 }
#define POK_CONFIG_SCHEDULING_NBSLOTS 4


#define POK_CONFIG_PROGRAM_NAME \
    { "prog_ctrl/prog_ctrl.elf", \
    "prog_net/prog_net.elf", \
    "prog_video/prog_video.elf" };

typedef enum {
    pok_part_identifier_pr1 = 0,
    pok_part_identifier_pr2 = 1,
    pok_part_identifier_pr3 = 2
} pok_part_identifiers_t;

typedef enum {
    pok_part_id_pr1 = 0,
    pok_part_id_pr2 = 1,
    pok_part_id_pr3 = 2
} pok_part_id_t;

typedef enum {
    pok_nodes_node1 = 0
} pok_node_id_t;

#endif
