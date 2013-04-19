
#include "ISR_Support.h"

/*
     IAR startup file for AT91SAM9XE microcontrollers.
 */
        .global Vector_Init		
        .balign 4
        .code 32

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#define __ASSEMBLY__
#include "board.h"

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------

#define ARM_MODE_ABT     0x17
#define ARM_MODE_FIQ     0x11
#define ARM_MODE_IRQ     0x12
#define ARM_MODE_SVC     0x13
#define ARM_MODE_SYS     0x1F

#define I_BIT            0x80
#define F_BIT            0x40

//------------------------------------------------------------------------------
//         Startup routine
//------------------------------------------------------------------------------

/*
   Exception vectors
 */
	.section .isr_vector, "x"
	
Vector_Init:
        /*SECTION .vectors:CODE:NOROOT(2)*/

        .global  resetVector
        .global  irqHandler
	.global  Kernel_Start

        .extern  Undefined_Handler
        .extern  vPortYieldProcessor
        .extern  Prefetch_Handler
        .extern  Abort_Handler
        .extern  FIQ_Handler
        .arm


Kernel_Start:	
resetVector:
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

//	.space	4096
	.space	128
IRQStackEnd:
//	.space	4096
	.space	128
FIQStackEnd:
//	.space	8192
	.space	128
SWIStackEnd:
	
/*
   Handles incoming interrupt requests by branching to the corresponding
   handler, as defined in the AIC. Supports interrupt nesting.
 */
IRQEntryPoint:
/*saveaux*/
		STMDB	SP!, {R0}		
		STMDB	SP, {SP}^ 		
		NOP					
		SUB	SP, SP, #4		
		LDMIA	SP!, {R0}	
		STMDB	R0!, {LR}		
		MOV	LR, R0			
		LDMIA	SP!, {R0}	
		STMDB	LR, {R0-LR}^	
		NOP					
		SUB	LR, LR, #60		
		MRS	R0, SPSR		
		STMDB	LR!, {R0}		
		LDR	R0, =ulCriticalNesting	
		LDR	R0, [R0]		
		STMDB	LR!, {R0}		
		LDR	R1, =pxCurrentTCB		
		LDR	R0, [R1]		
		STR	LR, [R0]     
   
        LDR     lr, =AT91C_BASE_AIC
        LDR     r0, [r14, #AIC_IVR]
        STR     lr, [r14, #AIC_IVR]      
        MOV     lr, pc
        BX      r0
        LDR     lr, =AT91C_BASE_AIC
        STR     lr, [r14, #AIC_EOICR]
/*restoreaux*/
		LDR	R1, =pxCurrentTCB	
		LDR	R0, [R1]			
		LDR	LR, [R0]			
		LDR	R0, =ulCriticalNesting	
		LDMFD	LR!, {R1}			
		STR	R1, [R0]			
		LDMFD	LR!, {R0}			
		MSR	SPSR_cxsf, R0		
		LDMFD	LR, {R0-R14}^		
		NOP															
		LDR	LR, [LR, #+60]		
		SUBS	PC, LR, #4			

        .global  resetHandler
        .extern  LowLevelInit
        .extern  ?main
        .arm

ResetEntryPoint:
	// Set IRQ stack.
	mov		r0,#0xD2
	msr		cpsr_c,r0
	ldr		sp,=IRQStackEnd
	bl		LowLevelInit

	// Set FIQ stack.
	mov		r0,#0xD1
	msr		cpsr_c,r0
	ldr		sp,=FIQStackEnd

	// Set SWI stack, and stay in SWI mode.
	mov		r0,#0xD3
	msr		cpsr_c,r0
	ldr		sp,=SWIStackEnd
	ldr		pc,MainEntryPointAddress
//	bl		main	
End:
	b		End
MainEntryPointAddress:		.word main
     
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
/*
	.weak  IRQEntryPoint
IRQEntryPoint:
	push	{r0-r12,lr}
	bl		IRQHandler
	pop		{r0-r12,lr}
	subs	pc,lr,#4
*/
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

