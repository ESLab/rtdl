	.section .isr_vector, "x"
	.extern Start
	.extern _data
	.extern _edata
	.extern _idata
	.extern _bss
	.extern _ebss

	.globl VectorTable
VectorTable:
	ldr		pc,ResetEntryPointAddress
	ldr		pc,UndefinedEntryPointAddress
	ldr		pc,SWIEntryPointAddress
	ldr		pc,PrefetchEntryPointAddress
	ldr		pc,DataEntryPointAddress
	nop
	ldr		pc,IRQEntryPointAddress
	ldr		pc,FIQEntryPointAddress

ResetEntryPointAddress:		.word ResetEntryPoint
UndefinedEntryPointAddress:	.word UndefinedEntryPoint
SWIEntryPointAddress:		.word SWIEntryPoint
PrefetchEntryPointAddress:	.word PrefetchEntryPoint
DataEntryPointAddress:		.word DataEntryPoint
IRQEntryPointAddress:		.word IRQEntryPoint
FIQEntryPointAddress:		.word FIQEntryPoint

	.space	4096
IRQStackEnd:
	.space	4096
FIQStackEnd:
	.space	8192
SWIStackEnd:

ResetEntryPoint:
	// Set vector base.
	ldr		r0,=VectorTable
	mcr		p15,#0,r0,c12,c0,#0

	// Set IRQ stack.
	mov		r0,#0xD2
	msr		cpsr_c,r0
	ldr		sp,=IRQStackEnd

	// Set FIQ stack.
	mov		r0,#0xD1
	msr		cpsr_c,r0
	ldr		sp,=FIQStackEnd

	// Set SWI stack, and stay in SWI mode.
	mov		r0,#0xD3
	msr		cpsr_c,r0
	ldr		sp,=SWIStackEnd

	// Clear BSS segment.
	ldr		r0,=_bss
	ldr		r1,=_ebss
	movs	r3,#0
ClearBSS:
	str		r3,[r0],#4
	cmp		r0,r1
	blt		ClearBSS

	// Copy data segment.
/*	ldr		r0,=_data
	ldr		r1,=_edata
	ldr		r2,=_idata
CopyData:
	ldr		r3,[r2],#4
	str		r3,[r0],#4
	cmp		r0,r1
	blt		CopyData*/

	bl		Start
End:
	b		End

	.weak  UndefinedEntryPoint
UndefinedEntryPoint:
	push	{r0-r12,lr}
	bl		UndefinedHandler
	pop		{r0-r12,lr}
	movs	pc,lr

	.weak  SWIEntryPoint
SWIEntryPoint:
	push	{r0-r12,lr}
	bl		SWIHandler
	pop		{r0-r12,lr}
	movs	pc,lr

	.weak  PrefetchEntryPoint
PrefetchEntryPoint:
	push	{r0-r12,lr}
	bl		PrefetchHandler
	pop		{r0-r12,lr}
	subs	pc,lr,#4

	.weak  DataEntryPoint
DataEntryPoint:
	push	{r0-r12,lr}
	bl		DataHandler
	pop		{r0-r12,lr}
	subs	pc,lr,#8

	.weak  IRQEntryPoint
IRQEntryPoint:
	push	{r0-r12,lr}
	bl		IRQHandler
	pop		{r0-r12,lr}
	subs	pc,lr,#4

	.weak  FIQEntryPoint
FIQEntryPoint:
	push	{r0-r7,lr}
	bl		FIQHandler
	pop		{r0-r7,lr}
	subs	pc,lr,#4


	.weak  UndefinedHandler
UndefinedHandler:
	b		UndefinedHandler

	.weak  SWIHandler
SWIHandler:
	b		SWIHandler

	.weak  PrefetchHandler
PrefetchHandler:
	b		PrefetchHandler

	.weak  DataHandler
DataHandler:
	b		DataHandler

	.weak  IRQHandler
IRQHandler:
	b		IRQHandler

	.weak  FIQHandler
FIQHandler:
	b		FIQHandler



	.globl DisableInterrupts
DisableInterrupts:
	mrs		r0,cpsr
	orr		r0,r0,#0xc0
	msr		cpsr_c,r0
	bx		lr

	.globl EnableInterrupts
EnableInterrupts:
	mrs		r0,cpsr
	bic		r0,r0,#0xc0
	msr		cpsr_c,r0
	bx		lr

	.globl DisableIRQs
DisableIRQs:
	mrs		r0,cpsr
	orr		r0,r0,#0x80
	msr		cpsr_c,r0
	bx		lr

	.globl EnableIRQs
EnableIRQs:
	mrs		r0,cpsr
	bic		r0,r0,#0x80
	msr		cpsr_c,r0
	bx		lr

	.globl DisableFIQs
DisableFIQs:
	mrs		r0,cpsr
	orr		r0,r0,#0x40
	msr		cpsr_c,r0
	bx		lr

	.globl EnableFIQs
EnableFIQs:
	mrs		r0,cpsr
	bic		r0,r0,#0x40
	msr		cpsr_c,r0
	bx		lr

