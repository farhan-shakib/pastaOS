#include "scheduler.h"
#include "process.h"

/**
 * Scheduler implementation - Round-robin process scheduling
 */

// Scheduler state
static uint32_t g_quantum_ms = SCHEDULER_DEFAULT_QUANTUM;
static uint32_t g_current_quantum_remaining = 0;
static uint8_t g_need_switch = 0;

// Statistics
static scheduler_stats_t g_stats = {0, 0, 0};

void scheduler_init(uint32_t quantum_ms) {
    if (quantum_ms == 0) {
        quantum_ms = SCHEDULER_DEFAULT_QUANTUM;
    }
    
    g_quantum_ms = quantum_ms;
    g_current_quantum_remaining = g_quantum_ms;
    g_need_switch = 0;
    
    // Reset stats
    g_stats.total_context_switches = 0;
    g_stats.total_quantum_expiries = 0;
    g_stats.current_quantum_used = 0;
}

int32_t scheduler_next_process(void) {
    uint32_t next_pid;
    int32_t rc = process_readyq_dequeue(&next_pid);
    
    if (rc < 0) {
        // No ready process; return kernel process (PID 0)
        return 0;
    }
    
    // Re-enqueue for next time (round-robin)
    process_readyq_enqueue(next_pid);
    
    return (int32_t)next_pid;
}

int32_t scheduler_context_switch(void) {
    process_t* current = process_current();
    
    // If current process terminated or blocked, remove it from ready queue
    if (current && (current->state == PROCESS_STATE_TERMINATED || 
                    current->state == PROCESS_STATE_BLOCKED ||
                    current->state == PROCESS_STATE_WAITING_IPC)) {
        // Don't put it back in queue; it's not ready
    } else if (current && current->state == PROCESS_STATE_RUNNING) {
        // Move current process to READY state (preempted)
        current->state = PROCESS_STATE_READY;
    }
    
    // Get next ready process
    uint32_t next_pid;
    int32_t rc = process_readyq_dequeue(&next_pid);
    
    if (rc < 0) {
        // No ready process; fall back to kernel
        next_pid = 0;
    } else {
        // Re-enqueue for round-robin
        process_readyq_enqueue(next_pid);
    }
    
    // Switch to next process
    rc = process_set_current(next_pid);
    if (rc < 0) {
        return -1;
    }
    
    // Reset quantum for new process
    g_current_quantum_remaining = g_quantum_ms;
    g_stats.current_quantum_used = 0;
    g_stats.total_context_switches++;
    g_need_switch = 0;
    
    return (int32_t)next_pid;
}

void scheduler_on_tick(void) {
    // Decrement quantum
    if (g_current_quantum_remaining > 0) {
        g_current_quantum_remaining--;
        g_stats.current_quantum_used++;
    }
    
    // If quantum expired, request context switch
    if (g_current_quantum_remaining == 0) {
        g_need_switch = 1;
        g_stats.total_quantum_expiries++;
    }
}

int32_t scheduler_add_process(uint32_t pid) {
    process_t* p = process_get(pid);
    if (!p) {
        return -1;
    }
    
    // Only add if process is READY
    if (p->state != PROCESS_STATE_READY) {
        return -2;
    }
    
    return process_readyq_enqueue(pid);
}

int32_t scheduler_remove_process(uint32_t pid) {
    process_t* p = process_get(pid);
    if (!p) {
        return -1;
    }
    
    // Don't need to remove from queue explicitly;
    // The scheduler skips non-READY processes
    return 0;
}

scheduler_stats_t scheduler_get_stats(void) {
    return g_stats;
}

void scheduler_reset_stats(void) {
    g_stats.total_context_switches = 0;
    g_stats.total_quantum_expiries = 0;
    g_stats.current_quantum_used = 0;
}

uint32_t scheduler_get_quantum(void) {
    return g_quantum_ms;
}

void scheduler_set_quantum(uint32_t quantum_ms) {
    if (quantum_ms > 0) {
        g_quantum_ms = quantum_ms;
    }
}

uint32_t scheduler_ready_queue_size(void) {
    return process_readyq_count();
}

uint8_t scheduler_should_switch(void) {
    return g_need_switch;
}
