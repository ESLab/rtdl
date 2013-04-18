ENTRY(Vector_Init);

MEMORY
{
	ram (rwx) : ORIGIN = 0x10000, LENGTH = ( 64M - 0x10000 )
	//flash (rx) : ORIGIN = 0x10000, LENGTH = ( 64M - 0x10000 )
	//ram (rwx) : ORIGIN = 0x4001000, LENGTH = ( 128M - 0x1000 )
	//sdram (rwx) : ORIGIN = 0x20000000, LENGTH = 512M
}

PROVIDE(__stack = 0x10000);

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
    } > ram

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
	INCLUDE "build/arm9_rtudemo.1.ld";
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

