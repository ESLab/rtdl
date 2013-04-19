/***********************************************************************************/
/* Copyright (c) 2013, Wictor Lund. All rights reserved.			   */
/* Copyright (c) 2013, Åbo Akademi University. All rights reserved.		   */
/* 										   */
/* Redistribution and use in source and binary forms, with or without		   */
/* modification, are permitted provided that the following conditions are met:	   */
/*      * Redistributions of source code must retain the above copyright	   */
/*        notice, this list of conditions and the following disclaimer.		   */
/*      * Redistributions in binary form must reproduce the above copyright	   */
/*        notice, this list of conditions and the following disclaimer in the	   */
/*        documentation and/or other materials provided with the distribution.	   */
/*      * Neither the name of the Åbo Akademi University nor the		   */
/*        names of its contributors may be used to endorse or promote products	   */
/*        derived from this software without specific prior written permission.	   */
/* 										   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND */
/* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   */
/* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE	   */
/* DISCLAIMED. IN NO EVENT SHALL ÅBO AKADEMI UNIVERSITY BE LIABLE FOR ANY	   */
/* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES	   */
/* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;	   */
/* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND	   */
/* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT	   */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS   */
/* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 		   */
/***********************************************************************************/

ENTRY(Vector_Init);

MEMORY
{
	flash (rwx) : ORIGIN = 0x10000000, LENGTH = 0x10000000
	ram (rwx) : ORIGIN = 0x00300000, LENGTH = 0x00100000
}

PROVIDE(__stack = 0x00314000);

SECTIONS
{
    .bss :
    {
	_bss = .;
	*(.bss*)
	*(COMMON)
	_ebss = .;
    } > ram

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
	*(.data*)
	_edata = .;
    } > flash

    .kernel : {
	    _kernel_elf_start = .;
	    INCLUDE "build/kernel-CONFIG`_'nodbg.ld"
	    _kernel_elf_end = .;
    } > flash

    .applications : {
	    INCLUDE "build/applications-CONFIG.ld"
    } > flash
	
}
