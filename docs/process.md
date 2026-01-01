# kacchiOS Process Management (Current)

This document describes the **current** process management implementation in this repo. It is intentionally simple: it supports creating and tracking processes, basic state transitions, termination, a FIFO ready queue, and a basic mailbox-style IPC.

> Note: This is *not* a full scheduler / context-switching implementation. The kernel test commands simulate “running” a process by calling its entry function directly.

---

## 1) Core Concepts

### 1.1 Process Control Block (PCB)
A process is represented by `process_t` in `src/process.h`. Each PCB contains:

- Identity: `pid`, `name`
- Lifecycle: `state`, `exit_code`
- Entry: `entry` (function pointer), `arg`
- Stack: `stack_base`, `stack_size`, `stack_top`
- IPC mailbox: ring buffer of messages (see IPC section)

### 1.2 Process Table
The process table is a **fixed-size array** of PCBs:

- Capacity: `PROCESS_MAX` (currently 32)
- Storage: internal static table in `src/process.c`

Slots are marked with:
- `used` (slot is allocated)
- `state` (including UNUSED/TERMINATED)

There is also a reserved “kernel process”:
- PID 0 is reserved as the kernel/null process.

---

## 2) Process States

The `process_state_t` state machine currently includes:

- `PROCESS_STATE_UNUSED`
  - Slot is free/uninitialized.

- `PROCESS_STATE_READY`
  - Process is created and eligible to run.

- `PROCESS_STATE_RUNNING`
  - Process is considered running (in this repo, set when the kernel test harness “dispatches” it).

- `PROCESS_STATE_BLOCKED`
  - Reserved for future use (e.g., waiting on I/O or sync primitive).

- `PROCESS_STATE_WAITING_IPC`
  - Used when a process attempts to receive IPC but its mailbox is empty.

- `PROCESS_STATE_TERMINATED`
  - Process has finished or has been killed; stack is freed.

Utility:
- `process_state_str(state)` returns a printable string for debugging.

---

## 3) Initialization

### `process_init()`
Initializes the process subsystem:

- Clears/reset all PCBs
- Initializes the ready queue
- Installs PID 0 as the “kernel” process and sets it to `RUNNING`
- Resets PID allocator to start at 1

This is called from the kernel after `memory_init()`.

---

## 4) Process Creation

### `process_create(name, entry, arg, stack_size)`
Creates a new process and returns:

- **PID** on success
- **Negative** error code on failure

What it does:

1. Finds a free slot in the process table (reuses TERMINATED slots if available).
2. Allocates a stack from the kernel heap using `kmalloc()`.
   - If `stack_size == 0`, uses `PROCESS_DEFAULT_STACK_SIZE`.
3. Initializes PCB fields:
   - `pid`, `name`, `entry`, `arg`, `stack_*`, `state = READY`
4. Enqueues the PID into the **ready queue** (FIFO) automatically.

Notes:
- Stack is freed when the process is terminated.

---

## 5) State Transitions & “Current” Process

### `process_set_state(pid, new_state)`
Sets a process state with some guardrails:

- Cannot transition back out of `TERMINATED`.
- Disallows setting `UNUSED` directly.

### `process_set_current(pid)`
Sets which process is “current”:

- Marks the previous current (if RUNNING) back to READY.
- Marks the new process as RUNNING.

This is used by the kernel test harness prior to calling a process entry function.

### `process_current()`
Returns the PCB for the current process PID.

---

## 6) Termination

### `process_terminate(pid, exit_code)`
Terminates a process:

- Kernel PID 0 cannot be terminated.
- Sets `state = TERMINATED` and stores `exit_code`.
- Frees the process stack (`kfree(stack_base)`), clears stack fields.
- If the terminated PID was current, resets current back to PID 0.

---

## 7) Lookup & Enumeration Utilities

### `process_get(pid)`
Searches the table and returns a pointer to the PCB for that PID, or `NULL`.

### `process_count()`
Counts all allocated processes (non-UNUSED) for quick diagnostics.

### `process_capacity()` / `process_at(index)`
Used for iterating over the process table from the kernel:

- `process_capacity()` returns `PROCESS_MAX`
- `process_at(i)` returns pointer to the PCB slot at index `i` (or `NULL` if out of range)

These are helpful for implementing `ps`-style debugging output.

---

## 8) Ready Queue (FIFO)

The ready queue stores runnable PIDs in FIFO order.

### `process_readyq_enqueue(pid)`
Enqueues a PID into the ready queue.

- Validates PID exists and is not UNUSED/TERMINATED
- Fails if queue is full

### `process_readyq_dequeue(&pid)`
Dequeues the next READY process PID.

- Skips stale entries (e.g., a PID that is no longer READY)
- Returns negative on empty

### `process_readyq_count()` / `process_readyq_clear()`
- Returns queue length
- Clears the queue

Important behavior:
- `process_create()` automatically enqueues new READY processes.

---

## 9) IPC (Mailbox)

IPC is implemented as a **per-process mailbox**, a fixed-size ring buffer.

### Message format: `ipc_message_t`
- `from_pid`
- `length`
- `payload[IPC_MAX_PAYLOAD]`

### `ipc_send(to_pid, data, length)`
Enqueues a message into the receiver’s mailbox.

- Validates receiver exists and is not TERMINATED/UNUSED
- `length` must be `<= IPC_MAX_PAYLOAD`
- Fails if mailbox full
- If receiver state is `WAITING_IPC`, it is set to `READY` (wake-up behavior)

### `ipc_receive(pid, &out)`
Dequeues one message from the target PID’s mailbox.

- If mailbox empty:
  - returns an error
  - sets the process state to `WAITING_IPC` (intent to block on IPC)

### `ipc_receive_current(&out)`
Convenience wrapper for the current process.

---

## 10) Kernel Test Harness Commands

The kernel includes simple commands to validate process management without context switching:

- `ps`
  - Lists processes (PID, STATE, NAME)

- `spawn N`
  - Creates N dummy processes (READY + enqueued)

- `run PID`
  - Simulates dispatch:
    - set current to PID
    - call entry function
    - terminate PID

- `runq`
  - Runs all processes currently in the ready queue in FIFO order.

- `kill PID`
  - Terminates a process and frees its stack.

The “hardcoded” processes `p1/p2/p3` are created at boot and are useful for exercising ready-queue ordering.

---

## 11) Notes / Limitations

- No true preemptive scheduling or context switching yet.
  - `run`/`runq` execute processes sequentially by directly calling their entry functions.

- Windows build note:
  - With standard MinGW, linking `kernel.elf` may fail because MinGW `ld` targets PE emulations (e.g., `i386pe`).
  - Use WSL/Linux or an `i686-elf-*` cross toolchain for ELF linking.

---

## 12) Quick Testing Checklist

1. Boot the kernel.
2. Run `ps` and confirm `p1/p2/p3` exist and are READY.
3. Run `runq` and confirm all ready processes execute in order.
4. Run `spawn 5`, then `ps`, then `runq` again.
5. For IPC validation (optional): send/receive can be tested by adding small kernel commands around `ipc_send`/`ipc_receive_current`.
