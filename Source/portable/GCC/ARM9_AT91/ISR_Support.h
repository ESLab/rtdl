
  //extern pxCurrentTCB
  //extern ulCriticalNesting
/*
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Context save and restore macro definitions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
*/
#define portSAVE_CONTEXT()													\
{																			\
  asm volatile (															\
	/*; Push R0 as we are going to use the register. 	*/					\
	"STMDB	SP!, {R0}		\n"												\
																			\
	/*; Set R0 to point to the task stack pointer. 	*/						\
	"STMDB	SP, {SP}^ 		\n"												\
	"NOP					\n"												\
	"SUB	SP, SP, #4		\n"												\
	"LDMIA	SP!, {R0}		\n"												\
																			\
	/*; Push the return address onto the stack. */							\
	"STMDB	R0!, {LR}		\n"												\
																			\
	/*; Now we have saved LR we can use it instead of R0.*/ 				\
	"MOV	LR, R0			\n"												\
																			\
	/*; Pop R0 so we can save it onto the system mode stack. */				\
	"LDMIA	SP!, {R0}		\n"												\
																			\
	/* Push all the system mode registers onto the task stack. 		*/		\
	"STMDB	LR, {R0-LR}^	\n"												\
	"NOP					\n"												\
	"SUB	LR, LR, #60		\n"												\
	/*; Push the SPSR onto the task stack. 	*/								\
	"MRS	R0, SPSR		\n"												\
	"STMDB	LR!, {R0}		\n"												\
	"LDR	R0, =ulCriticalNesting	\n "									\
	"LDR	R0, [R0]		\n"												\
	"STMDB	LR!, {R0}		\n"												\
	/*; Store the new top of stack for the task. 	*/						\
	"LDR	R1, =pxCurrentTCB		\n"										\
	"LDR	R0, [R1]		\n"												\
	"STR	LR, [R0]		\n"												\
	);																		\
}																			\

#define portRESTORE_CONTEXT()												\
{																			\
  asm volatile(																\
	/*; Set the LR to the task stack. */									\
	"LDR	R1, =pxCurrentTCB	\n"											\
	"LDR	R0, [R1]			\n"											\
	"LDR	LR, [R0]			\n"											\
	/*; The critical nesting depth is the first item on the stack. 			\
	; Load it into the ulCriticalNesting variable. */						\
	"LDR	R0, =ulCriticalNesting	\n"										\
	"LDMFD	LR!, {R1}			\n"											\
	"STR	R1, [R0]			\n"											\
	/*; Get the SPSR from the stack. */										\
	"LDMFD	LR!, {R0}			\n"											\
	"MSR	SPSR_cxsf, R0		\n"											\
	/*; Restore all system mode registers for the task. */					\
	"LDMFD	LR, {R0-R14}^		\n"											\
	"NOP							\n"										\
	/*; Restore the return address. */										\
	"LDR	LR, [LR, #+60]		\n"											\
	/*; And return - correcting the offset in the LR to obtain the 			\
	; correct address. 		*/												\
	"SUBS	PC, LR, #4			\n"											\
	);																		\
}																			\


