/* kernel.c - Main kernel with null process and memory tests */
#include "types.h"
#include "serial.h"
#include "string.h"
#include "src/memory.h"

#define MAX_INPUT 128

extern uint32_t __kernel_end;

void kmain(void) {
    char input[MAX_INPUT];
    int pos = 0;

    /* Initialize hardware */
    serial_init();

    /* Initialize memory manager (heap + stack) */
    memory_init((uint32_t)&__kernel_end);
    serial_puts("Memory manager initialized\n\n");

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
            serial_puts("You typed: ");
            serial_puts(input);
            serial_puts("\n");
        }
    }

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
