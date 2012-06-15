// Copyright ARM Ltd 2001. All rights reserved.
// Modified by VL (07/24/2003)

	.global Vector_Init
	.global Kernel_Start

	.balign 4
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
	.extern setup_vm

// *****************
// Exception Vectors
// *****************

// Note: LDR PC instructions are used here, though branch (B) instructions
// could also be used, unless the ROM is at an address >32MB.

	.section .isr_vector, "x"

Vector_Init:
	ldr	pc, Reset_Addr
	ldr	pc, Undefined_Addr
	ldr	pc, SWI_Addr
	ldr	pc, Prefetch_Addr
	ldr	pc, Abort_Addr
	nop
	ldr	pc, IRQ_Addr
	ldr	pc, FIQ_Addr

Reset_Addr:
		.word	Reset_Handler
Undefined_Addr:
		.word	Undefined_Handler
SWI_Addr:
		.word	SWI_Handler
Prefetch_Addr:	.word	Prefetch_Handler
Abort_Addr:	.word	Abort_Handler
IRQ_Addr:	.word	IRQ_Handler
FIQ_Addr:	.word	FIQ_Handler

Kernel_Start:
	bl setup_vm
	ldr pc, Init_Addr

Init_Addr:
	.word	_init

// ************************
// Exception Handlers
// ************************

Reset_Handler:
	ldr	sp, Stack_Addr
	b	_init
Stack_Addr:	.word	0

Undefined_Handler:
	// Put system/user mode R14-R0, exception address (LR - (4(ARM) or 2(THUMB))) and SPSR on stack
	// used for logging
	stmdb	sp, {r0-lr}^
	nop
	sub	sp, sp, #60
	mrs	r0, spsr
	sub	r1, lr, #2
	// if ARM mode subtract 2 more
	tst	r0, #T_bit
	subeq	r1, r1, #2
	stmdb	sp!, {r1}
	stmdb	sp!, {r0}
	nop
	mov	r0, sp
	bl	Undefined_Handler_Panic
Prefetch_Handler:
	// Put system/user mode R14-R0, exception address (LR - 4) and SPSR on stack
	// used for logging
	stmdb	sp, {r0-lr}^
	nop
	sub	sp, sp, #60
	sub	r0, lr, #4
	stmdb	sp!, {r0}
	mrs	r0, spsr
	stmdb	sp!, {r0}
	nop
	mov	r0, sp
	bl	Prefetch_Handler_Panic
Abort_Handler:
	// Put system/user mode R14-R0, exception address (LR - 8) and SPSR on stack
	// used for logging
	stmdb	sp, {r0-lr}^
	nop
	sub	sp, sp, #60
	sub	r0, lr, #8
	stmdb	sp!, {r0}
	mrs	r0, spsr
	stmdb	sp!, {r0}
	nop
	mov	r0, sp
	bl	Abort_Handler_Panic

// never refered to, this is only for backward compatibility purpose.
// FIQ_Handler
// 		B       FIQ_Handler


// The following SWI handler support just the taskswitch for OpenRTOS
SWI_Handler:
	B	vPortSVCHandler

FIQ_Handler:
IRQ_Handler:
	B	vPortInterruptContext

	.end
