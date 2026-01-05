#include "scheduler.h"
#include "process.h"
#include "io.h"

int time_quantum = DEFAULT_QUANTUM;

int ready_queue[MAX_PROCESSES];
int ready_count = 0;

void ready_queue_insert(int pid)
{
    if (ready_count >= MAX_PROCESSES)
        return;

    int insert_pos = ready_count;

    for (int i = 0; i < ready_count; i++)
    {
        if (process_table[ready_queue[i]].priority > process_table[pid].priority)
        {
            insert_pos = i;
            break;
        }
    }

    for (int i = ready_count; i > insert_pos; i--)
    {
        ready_queue[i] = ready_queue[i - 1];
    }

    ready_queue[insert_pos] = pid;
    ready_count++;
}

void ready_queue_remove(int pid)
{
    for (int i = 0; i < ready_count; i++)
    {
        if (ready_queue[i] == pid)
        {
            for (int j = i; j < ready_count - 1; j++)
            {
                ready_queue[j] = ready_queue[j + 1];
            }
            ready_count--;
            return;
        }
    }
}

int ready_queue_get_highest()
{
    if (ready_count == 0)
        return -1;
    return ready_queue[0];
}

// XINU-Style Scheduler(resched)

void schedule()
{
    // Aging
    for (int i = 0; i < ready_count; i++)
    {
        int pid = ready_queue[i];
        process_table[pid].age++;

        if (process_table[pid].age >= AGE_THRESHOLD && process_table[pid].priority > MIN_PRIORITY)
        {
            process_table[pid].age = 0;
            process_table[pid].priority--;

            ready_queue_remove(pid);
            ready_queue_insert(pid);
        }
    }

    if (current_pid != -1 && process_table[current_pid].state == PROCESS_CURRENT)
    {
        process_table[current_pid].quantum--;
    }

    int next_pid = ready_queue_get_highest();

    if (current_pid != -1 && process_table[current_pid].state == PROCESS_CURRENT)
    {
        if (process_table[current_pid].quantum > 0)
        {
            if (next_pid == -1 || process_table[current_pid].priority <= process_table[next_pid].priority)
            {
                return;
            }
        }
    }

    // Handle idle state (no ready processes)
    if (next_pid == -1)
    {
        if (current_pid != -1 && process_table[current_pid].state == PROCESS_CURRENT)
        {
            process_table[current_pid].quantum = time_quantum;
            return;
        }

        // NULL process
        while (1)
        {
            // CPU halt instruction here
        }
    }

    PCB *old_proc = (current_pid != -1) ? &process_table[current_pid] : NULL;
    PCB *new_proc = &process_table[next_pid];

    if (old_proc && old_proc->state == PROCESS_CURRENT)
    {
        old_proc->state = PROCESS_READY;
        ready_queue_insert(current_pid);
    }

    ready_queue_remove(next_pid);
    new_proc->state = PROCESS_CURRENT;
    new_proc->quantum = time_quantum;
    new_proc->age = 0;

    current_pid = next_pid;

    // Switch CPU context
    context_switch(old_proc ? &old_proc->sp : NULL, &new_proc->sp);
}

typedef struct
{
    uint32_t esp;
    uint32_t ebp;
    uint32_t eflags;
} context_t;

// CPU Abstraction Layer
static inline uint32_t cpu_get_esp(void)
{
    // Read current stack pointer
    return 0;
}
static inline uint32_t cpu_get_ebp(void)
{
    // Read current base pointer
    return 0;
}
static inline uint32_t cpu_get_eflags(void)
{
    // Read CPU flags register
    return 0;
}
static inline void cpu_set_esp(uint32_t esp)
{
    // Load new stack pointer

    return 0;
}
static inline void cpu_set_ebp(uint32_t ebp)
{
    // Load new base pointer

    return 0;
}

void context_switch(uint32_t **old_sp, uint32_t **new_sp)
{
    // Save current process context
    if (old_sp != NULL)
    {
        context_t *old_ctx = (context_t *)(*old_sp);
        old_ctx->esp = cpu_get_esp();
        old_ctx->ebp = cpu_get_ebp();
        old_ctx->eflags = cpu_get_eflags();
    }

    // Restore new process context
    if (new_sp != NULL && *new_sp != NULL)
    {
        context_t *new_ctx = (context_t *)(*new_sp);
        cpu_set_ebp(new_ctx->ebp);
        cpu_set_esp(new_ctx->esp);
    }
}
