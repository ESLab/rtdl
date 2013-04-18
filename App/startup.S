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
        LDR     PC, Undefined_Addr
        LDR     PC, SWI_Addr
        LDR     PC, Prefetch_Addr
        LDR     PC, Abort_Addr
        NOP
        LDR     PC, IRQ_Addr
        LDR     PC, FIQ_Addr

Reset_Addr:      .word     Reset_Handler
Undefined_Addr:  .word     Undefined_Handler
SWI_Addr:        .word     SWI_Handler
Prefetch_Addr:   .word     Prefetch_Handler
Abort_Addr:      .word     Abort_Handler
IRQ_Addr:        .word     IRQ_Handler
FIQ_Addr:        .word     FIQ_Handler

// ************************
// Exception Handlers
// ************************

Reset_Handler:
	LDR			SP, Stack_Addr
	B			_init
Stack_Addr:		.word	  __stack

Undefined_Handler:
    // Put system/user mode R14-R0, exception address (LR - (4(ARM) or 2(THUMB))) and SPSR on stack
    // used for logging
    STMDB      SP, {R0-LR}^
    NOP
    SUB        SP, SP, #60
    MRS        R0, SPSR
    SUB        R1, LR, #2
    // if ARM mode subtract 2 more
    TST        R0, #T_bit
    SUBEQ      R1, R1, #2
    STMDB      SP!, {R1}
    STMDB      SP!, {R0}
    NOP
    MOV        R0, SP
    BL         Undefined_Handler_Panic
Prefetch_Handler:
    // Put system/user mode R14-R0, exception address (LR - 4) and SPSR on stack
    // used for logging
    STMDB      SP, {R0-LR}^
    NOP
    SUB        SP, SP, #60
    SUB        R0, LR, #4
    STMDB      SP!, {R0}
    MRS        R0, SPSR
    STMDB      SP!, {R0}
    NOP
    MOV        R0, SP
    BL         Prefetch_Handler_Panic
Abort_Handler:
    // Put system/user mode R14-R0, exception address (LR - 8) and SPSR on stack
    // used for logging
    STMDB      SP, {R0-LR}^
    NOP
    SUB        SP, SP, #60
    SUB        R0, LR, #8
    STMDB      SP!, {R0}
    MRS        R0, SPSR
    STMDB      SP!, {R0}
    NOP
    MOV        R0, SP
    BL         Abort_Handler_Panic

// never refered to, this is only for backward compatibility purpose.
// FIQ_Handler
// 		B       FIQ_Handler


// The following SWI handler support just the taskswitch for OpenRTOS
SWI_Handler:
		B				vPortSVCHandler

FIQ_Handler:
IRQ_Handler:
	B					vPortInterruptContext

    .end

