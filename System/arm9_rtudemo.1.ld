ENTRY(Vector_Init);

MEMORY
{
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 0x10000000
	flash (rx) : ORIGIN = 0x10000000, LENGTH = 0x10000000
}

PROVIDE(__stack = 0x400000);

SECTIONS
{
    .dynsym : { *(.dynsym) }
    .rel.dyn : { *(.rel.dyn) }
    .plt : { *(.plt) }
    .text :
    {
        _text = .;
        __isr_vector_start = .;
        *(.isr_vector)
        __isr_vector_end = .;
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > flash

    .data : AT(ADDR(.text) + SIZEOF(.text))
    {
        _data = .;
        *(vtable)
        *(.data*)
        _edata = .;
    } > ram
    .applications : {
         INCLUDE "build/applications-arm9_rtudemo.ld"
    } > ram
    .system_elf : {
        _system_elf_start = .;
	INCLUDE "build/arm9_rtudemo.0.ld";
	_system_elf_end = .;
    }
    .bss :
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > ram
}

