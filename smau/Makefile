# Makefile — ARM64 NEON Matrix Multiply-Add Test
#
# Linux native:
#   make linux     → builds and runs on Linux AArch64
#
# Bare-metal (need aarch64-none-elf-gcc + QEMU):
#   make baremetal → builds mat_test.elf
#   make qemu      → runs under QEMU (semihosting output)
#   make clean     → remove build artifacts

CROSS ?= aarch64-none-elf-
CC    ?= gcc
CFLAGS  = -O2 -mcpu=cortex-a72
QEMU   ?= qemu-system-aarch64

all: linux

linux: mat_test.c
	$(CC) $(CFLAGS) -o mat_test mat_test.c
	./mat_test

baremetal: mat_test.c linker.ld
	$(CROSS)gcc $(CFLAGS) -ffreestanding -nostdlib -DBAREMETAL \
		-Tlinker.ld -o mat_test.elf mat_test.c

qemu: baremetal
	$(QEMU) -M virt -cpu max -nographic -semihosting -kernel mat_test.elf

clean:
	rm -f mat_test mat_test.elf
