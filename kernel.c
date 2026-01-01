/* kernel.c - Main kernel with null process and memory tests */
#include "types.h"
#include "serial.h"
#include "string.h"
#include "src/memory.h"
#include "src/process.h"

#define MAX_INPUT 128

extern uint32_t __kernel_end;

static void serial_put_u32(uint32_t value) {
    char buf[11];
    uint32_t i = 0;

    if (value == 0) {
        serial_putc('0');
        return;
    }

    while (value > 0 && i < sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0) {
        serial_putc(buf[--i]);
    }
}

static uint32_t parse_u32(const char* s, uint32_t* out_ok) {
    uint32_t value = 0;
    uint32_t ok = 0;

    if (!s || !*s) {
        if (out_ok) *out_ok = 0;
        return 0;
    }

    while (*s == ' ') s++;
    while (*s >= '0' && *s <= '9') {
        ok = 1;
        value = (value * 10) + (uint32_t)(*s - '0');
        s++;
    }

    if (out_ok) *out_ok = ok;
    return value;
}

static int starts_with(const char* s, const char* prefix) {
    if (!s || !prefix) return 0;
    while (*prefix) {
        if (*s++ != *prefix++) return 0;
    }
    return 1;
}

static void dummy_process(void* arg) {
    const char* tag = (const char*)arg;
    serial_puts("[dummy] running: ");
    serial_puts(tag ? tag : "(null)");
    serial_puts("\n");
}

static void cmd_ps(void) {
    serial_puts("PID\tSTATE\t\tNAME\n");
    for (uint32_t i = 0; i < process_capacity(); i++) {
        process_t* p = process_at(i);
        if (!p || !p->used || p->state == PROCESS_STATE_UNUSED) {
            continue;
        }
        serial_put_u32(p->pid);
        serial_puts("\t");
        serial_puts(process_state_str(p->state));
        serial_puts("\t");
        if (p->state == PROCESS_STATE_WAITING_IPC) {
            serial_puts("\t");
        }
        serial_puts(p->name);
        serial_puts("\n");
    }
}

static void cmd_spawn(uint32_t n) {
    if (n == 0) {
        serial_puts("spawn: provide N > 0\n");
        return;
    }

    uint32_t created = 0;
    for (uint32_t i = 0; i < n; i++) {
        int32_t pid = process_create("dummy", dummy_process, "dummy", 0);
        if (pid < 0) {
            serial_puts("spawn: failed at i=");
            serial_put_u32(i);
            serial_puts(" err=");
            serial_put_u32((uint32_t)(-pid));
            serial_puts("\n");
            break;
        }
        created++;
        serial_puts("spawned pid=");
        serial_put_u32((uint32_t)pid);
        serial_puts("\n");
    }

    serial_puts("spawn: created ");
    serial_put_u32(created);
    serial_puts(" process(es)\n");
}

static void cmd_kill(uint32_t pid) {
    int32_t rc = process_terminate(pid, 0);
    if (rc < 0) {
        serial_puts("kill: failed\n");
        return;
    }
    serial_puts("killed pid=");
    serial_put_u32(pid);
    serial_puts("\n");
}

static void cmd_run(uint32_t pid) {
    process_t* p = process_get(pid);
    if (!p) {
        serial_puts("run: no such pid\n");
        return;
    }
    if (!p->entry) {
        serial_puts("run: no entry function\n");
        return;
    }

    int32_t rc = process_set_current(pid);
    if (rc < 0) {
        serial_puts("run: cannot set current\n");
        return;
    }

    serial_puts("run: executing pid=");
    serial_put_u32(pid);
    serial_puts("\n");

    p->entry(p->arg);

    process_terminate(pid, 0);
    serial_puts("run: finished pid=");
    serial_put_u32(pid);
    serial_puts("\n");
}

void kmain(void) {
    char input[MAX_INPUT];
    int pos = 0;

    /* Initialize hardware */
    serial_init();

    /* Initialize memory manager (heap + stack) */
    memory_init((uint32_t)&__kernel_end);
    serial_puts("Memory manager initialized\n\n");

    /* Initialize process manager */
    process_init();
    serial_puts("Process manager initialized\n\n");

    /* ================= HEAP TESTS ================= */
    serial_puts("=== HEAP TESTS ===\n");

    char* h1 = (char*)kmalloc(16);
    strcpy(h1, "Heap1");
    serial_puts("Allocated h1: "); serial_puts(h1); serial_puts("\n");

    char* h2 = (char*)kmalloc(32);
    strcpy(h2, "Heap2");
    serial_puts("Allocated h2: "); serial_puts(h2); serial_puts("\n");

    kfree(h1); // Free first block
    serial_puts("Freed h1\n");

    char* h3 = (char*)kmalloc(8); // Should reuse h1 space if merged/split
    strcpy(h3, "H3");
    serial_puts("Allocated h3 after free: "); serial_puts(h3); serial_puts("\n");

    kfree(h2);
    kfree(h3);
    serial_puts("Freed h2 and h3\n");

    /* ================= STACK TESTS ================= */
    serial_puts("\n=== STACK TESTS ===\n");

    char* s1 = (char*)kalloc_stack(1024);
    strcpy(s1, "Stack1");
    serial_puts("Allocated s1: "); serial_puts(s1); serial_puts("\n");

    char* s2 = (char*)kalloc_stack(512);
    strcpy(s2, "Stack2");
    serial_puts("Allocated s2: "); serial_puts(s2); serial_puts("\n");

    kfree_stack(s2); // Free last stack first
    serial_puts("Freed s2\n");

    kfree_stack(s1); // Free first stack
    serial_puts("Freed s1\n");

    serial_puts("\nAll memory tests completed successfully!\n\n");

    /* ================= WELCOME MESSAGE ================= */
    serial_puts("========================================\n");
    serial_puts("    kacchiOS - Minimal Baremetal OS\n");
    serial_puts("========================================\n");
    serial_puts("Hello from kacchiOS!\n");
    serial_puts("Running null process...\n\n");
    serial_puts("Commands: ps | spawn N | run PID | kill PID\n\n");

    /* Main loop - the "null process" */
    while (1) {
        serial_puts("kacchiOS> ");
        pos = 0;

        /* Read input line */
        while (1) {
            char c = serial_getc();

            if (c == '\r' || c == '\n') {
                input[pos] = '\0';
                serial_puts("\n");
                break;
            } else if ((c == '\b' || c == 0x7F) && pos > 0) {
                pos--;
                serial_puts("\b \b");  /* Erase character */
            } else if (c >= 32 && c < 127 && pos < MAX_INPUT - 1) {
                input[pos++] = c;
                serial_putc(c);
            }
        }

        if (pos > 0) {
            if (strcmp(input, "ps") == 0) {
                cmd_ps();
            } else if (starts_with(input, "spawn ")) {
                uint32_t ok = 0;
                uint32_t n = parse_u32(input + 6, &ok);
                if (!ok) {
                    serial_puts("usage: spawn N\n");
                } else {
                    cmd_spawn(n);
                }
            } else if (starts_with(input, "run ")) {
                uint32_t ok = 0;
                uint32_t pid = parse_u32(input + 4, &ok);
                if (!ok) {
                    serial_puts("usage: run PID\n");
                } else {
                    cmd_run(pid);
                }
            } else if (starts_with(input, "kill ")) {
                uint32_t ok = 0;
                uint32_t pid = parse_u32(input + 5, &ok);
                if (!ok) {
                    serial_puts("usage: kill PID\n");
                } else {
                    cmd_kill(pid);
                }
            } else {
                serial_puts("Unknown command. Try: ps | spawn N | run PID | kill PID\n");
            }
        }
    }

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
