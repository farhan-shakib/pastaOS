#include "memory.h"

// Static kernel heap 
static uint8_t kernel_heap[KERNEL_HEAP_SIZE];

// Current allocation offset 
static uint32_t heap_offset = 0;

// Stack region grows from top of heap downward 
static uint32_t stack_offset = KERNEL_HEAP_SIZE;

// Initialize memory manager 
void memory_init(void)
{
    heap_offset = 0;
    stack_offset = KERNEL_HEAP_SIZE;
}

//Align size to 4 bytes 
static uint32_t align4(uint32_t size)
{
    return (size + 3) & ~3;
}

//Allocate memory from heap (bottom-up) 
void* kmalloc(uint32_t size)
{
    size = align4(size);

    if (heap_offset + size >= stack_offset) {
        return 0;  // Out of memory 
    }

    void *ptr = &kernel_heap[heap_offset];
    heap_offset += size;
    return ptr;
}

// Free heap memory (not implemented yet) 
void kfree(void *ptr)
{
    (void)ptr;
}

//Allocate a process stack (top-down) 
void* alloc_stack(void)
{
    if (stack_offset < heap_offset + KERNEL_STACK_SIZE) {
        return 0;  
    }

    stack_offset -= KERNEL_STACK_SIZE;
    return &kernel_heap[stack_offset];
}

// Free process stack (not implemented yet)
void free_stack(void *stack)
{
    (void)stack;
}
