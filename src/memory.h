#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

void memory_init(uint32_t heap_start);
void* kmalloc(uint32_t size);
void kfree(void* ptr);

void* kalloc_stack(uint32_t size);
void kfree_stack(void* ptr);


#endif
