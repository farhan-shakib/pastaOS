#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

/* Size of kernel memory pool (64 KB) */
#define KERNEL_HEAP_SIZE  (64 * 1024)

/* Stack size per process (4 KB) */
#define KERNEL_STACK_SIZE 4096

/* Initialize memory manager */
void memory_init(void);

/* Heap allocation */
void* kmalloc(uint32_t size);

/* Heap free (stub for now) */
void kfree(void *ptr);

/* Stack allocation for processes */
void* alloc_stack(void);

/* Stack deallocation (stub for now) */
void free_stack(void *stack);

#endif