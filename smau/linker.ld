/* Minimal linker script for QEMU ARM64 virt bare-metal */
SECTIONS
{
    . = 0x40000000;

    .text : {
        *(.text._start)
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }

    . = ALIGN(16);
    . += 0x100000;      /* 1 MB stack */
    __stack_top = .;
}
