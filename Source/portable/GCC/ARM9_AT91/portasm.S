     .balign 4
     .code 32

	.extern vTaskSwitchContext

	.global vPortYieldProcessor
	.global vPortStartFirstTask

#include "ISR_Support.h"

vPortStartFirstTask:
/*restore context*/
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


vPortYieldProcessor:
	ADD		LR, LR, #4																		
								
	/*save context*/
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

	LDR R0, =vTaskSwitchContext	
	mov     lr, pc
	BX R0
	/*restore context*/
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

	.end

