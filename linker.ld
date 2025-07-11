OUTPUT_FORMAT(elf32-i386)
ENTRY(kmain)

SECTIONS {
    . = 1M;                  /* place le kernel à 1 MiB */

    .multiboot : { KEEP(*(.multiboot)) }
    .text       : { *(.text)   }
    .rodata     : { *(.rodata) }
    .data       : { *(.data)   }
    .bss        : { *(.bss COMMON) }
}
