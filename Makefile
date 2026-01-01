# Makefile for kacchiOS
CROSS ?=
CC = $(CROSS)gcc
LD = $(CROSS)ld
AS = $(CROSS)as

CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -nostdinc \
         -fno-builtin -fno-stack-protector -I.
ASFLAGS = --32
LDFLAGS = -m elf_i386

OBJS = boot.o kernel.o serial.o string.o src/memory.o src/process.o

all: kernel.elf

objs: $(OBJS)

kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -T link.ld -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

run: kernel.elf
	qemu-system-i386 -kernel kernel.elf -m 64M -serial stdio -display none

run-vga: kernel.elf
	qemu-system-i386 -kernel kernel.elf -m 64M -serial mon:stdio

debug: kernel.elf
	qemu-system-i386 -kernel kernel.elf -m 64M -serial stdio -display none -s -S &
	@echo "Waiting for GDB connection on port 1234..."
	@echo "In another terminal run: gdb -ex 'target remote localhost:1234' -ex 'symbol-file kernel.elf'"

clean:
ifeq ($(OS),Windows_NT)
	-del /Q *.o kernel.elf 2>NUL
	-del /Q src\*.o 2>NUL
else
	rm -f *.o kernel.elf src/*.o
endif

.PHONY: all run run-vga debug clean
