#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

// Scheduling Parameters
#define DEFAULT_QUANTUM 5
#define MAX_PRIORITY 10
#define MIN_PRIORITY 0
#define AGE_THRESHOLD 3

extern int time_quantum;

void schedule();
void context_switch(uint32_t **old_sp, uint32_t **new_sp);

void ready_queue_insert(int pid);
void ready_queue_remove(int pid);
int ready_queue_get_highest();

#endif
