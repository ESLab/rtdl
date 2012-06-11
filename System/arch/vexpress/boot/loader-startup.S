// Copyright ARM Ltd 2001. All rights reserved.
// Modified by VL (07/24/2003)

        .global Vector_Init

        .balign 4
//        .section .isr_vector, "x"
        .code 32

// These are example exception vectors and exception handlers
//
// Where there is ROM fixed at 0x0 (build_b), these are hard-coded at 0x0.
// Where ROM/RAM remapping occurs (build_c), these are copied from ROM to RAM.
// The copying is done automatically by the C library code inside __main.
//
// This version puts a branch to an Interrupt Handler (IRQ_Handler in int_handler.c)
// in its exception vector table.

	.equ T_bit, 0x20

	.extern vPortSVCHandler
	.extern vPortResetHandler
	.extern Undefined_Handler_Panic
	.extern Prefetch_Handler_Panic
	.extern Abort_Handler_Panic
	.extern __stack
	.extern _init

// *****************
// Exception Vectors
// *****************

// Note: LDR PC instructions are used here, though branch (B) instructions
// could also be used, unless the ROM is at an address >32MB.

        .section .isr_vector, "x"

Vector_Init:
        LDR     PC, Reset_Addr
        LDR     PC, Reset_Addr
        LDR     PC, Reset_Addr
        LDR     PC, Reset_Addr
        LDR     PC, Reset_Addr
        NOP
        LDR     PC, Reset_Addr
        LDR     PC, Reset_Addr

Reset_Addr:      .word     Reset_Handler

// ************************
// Exception Handlers
// ************************

Reset_Handler:
	ADR	SP, Vector_Init
	LDR	R0, Stack_Offset
	ADD	SP, R0, SP
	B	_init
Stack_Offset:	.word	__stack_offset
    .end

