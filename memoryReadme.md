# kacchiOS - Memory Manager Implementation

This is an educational minimal baremetal operating system (`kacchiOS`) project.  

This branch adds a **basic memory manager** with heap and stack allocation, deallocation, and tests.

---

## Features Implemented

### 1. Heap Allocation
- Functions: `kmalloc(size)` and `kfree(ptr)`
- Supports:
  - Allocating memory dynamically from the heap.
  - Freeing memory and reusing freed blocks.
  - Splitting large blocks and merging adjacent free blocks to reduce fragmentation.

### 2. Stack Allocation
- Functions: `kalloc_stack(size)` and `kfree_stack(ptr)`
- Supports:
  - Allocating simple stacks (LIFO order) for temporary storage.
  - Freeing stack memory in reverse order of allocation.

### 3. Memory Tests
- All features are tested in `kernel.c`:
  - Heap allocation and deallocation.
  - Stack allocation and deallocation.
  - Serial console output confirms correctness.

**Sample Output in QEMU Serial Console:**

