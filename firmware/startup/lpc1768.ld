ENTRY(Reset_Handler)
MEMORY {
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 512K
    RAM (rwx)  : ORIGIN = 0x10000000, LENGTH = 32K
}
_estack = ORIGIN(RAM) + LENGTH(RAM);
SECTIONS {
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
    .text : { *(.text*) *(.rodata*) . = ALIGN(4); } > FLASH
    .ARM.extab : { *(.ARM.extab*) } > FLASH
    .ARM.exidx : { *(.ARM.exidx*) } > FLASH
    _sidata = LOADADDR(.data);
    .data : { . = ALIGN(4); _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > RAM AT > FLASH
    .bss (NOLOAD) : { . = ALIGN(4); _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } > RAM
    ASSERT(_ebss <= _estack - 4096, "RAM full: retain a 4 KiB stack")
    /DISCARD/ : { *(.comment*) *(.note*) }
}
