/**************************************************
 *
 * This file declares the ARM intrinsic inline functions.
 *
 * Copyright 1999-2006 IAR Systems. All rights reserved.
 *
 * $Revision: 22838 $
 *
 **************************************************/

#ifndef __INTRINSICS_INCLUDED
#define __INTRINSICS_INCLUDED

#include "ycheck.h"

#ifndef _SYSTEM_BUILD
  #pragma system_include
#endif

/*
 * Check that the correct C compiler is used.
 */

#if ((__TID__ >> 8) & 0x007F) != 79
#error "File intrinsics.h can only be used together with iccarm."
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma language=save
#pragma language=extended

__intrinsic void    __no_operation(void);

__intrinsic void    __disable_interrupt(void);
__intrinsic void    __enable_interrupt(void);

typedef unsigned long __istate_t;

__intrinsic __istate_t __get_interrupt_state(void);
__intrinsic void __set_interrupt_state(__istate_t);


#define __segment_size( SEG ) ((char *)__sfe( SEG ) - (char*)__sfb( SEG ))
#define __sfs( SEG )          ((char *)__sfe( SEG ) - (char*)__sfb( SEG ))


/* System control access for Cortex-M cores */

__intrinsic unsigned long __get_PRIMASK( void );
__intrinsic void          __set_PRIMASK( unsigned long );
__intrinsic unsigned long __get_CONTROL( void );
__intrinsic void          __set_CONTROL( unsigned long );

/*
unsigned long __get_PRIMASK( void );
void          __set_PRIMASK( unsigned long );
unsigned long __get_CONTROL( void );
void          __set_CONTROL( unsigned long );
*/
/* These are only available for v7M */
/*
__intrinsic unsigned long __get_FAULTMASK( void );
__intrinsic void          __set_FAULTMASK(unsigned long);
__intrinsic unsigned long __get_BASEPRI( void );
__intrinsic void          __set_BASEPRI( unsigned long );

/* "Old" style intrerrupt control routines */
/*
__intrinsic void __disable_irq(void);
__intrinsic void __enable_irq(void);
__intrinsic void __disable_fiq(void);
__intrinsic void __enable_fiq(void);
*/
void __disable_irq(void);
void __enable_irq(void);
void __disable_fiq(void);
void __enable_fiq(void);



/* ARM-mode intrinsics */

__intrinsic unsigned long __SWP( unsigned long, unsigned long * );
__intrinsic unsigned char __SWPB( unsigned char, unsigned char * );

typedef unsigned long __ul;

/*  Co-processor access */
__intrinsic void            __MCR( __ul coproc, __ul opcode_1, __ul src,
                                   __ul CRn, __ul CRm, __ul opcode_2 );
__intrinsic unsigned long   __MRC( __ul coproc, __ul opcode_1, __ul CRn,
                                   __ul CRm, __ul opcode_2 );

/* Status register access */
__intrinsic unsigned long __get_CPSR( void );
__intrinsic void          __set_CPSR( unsigned long );

/* Architecture v5T, CLZ is also available in Thumb mode for Thumb2 cores */
__intrinsic unsigned char __CLZ( unsigned long );

/* Architecture v5TE */
__intrinsic signed long __QADD( signed long, signed long );
__intrinsic signed long __QDADD( signed long, signed long );
__intrinsic signed long __QSUB( signed long, signed long );
__intrinsic signed long __QDSUB( signed long, signed long );

__intrinsic signed long __QDOUBLE( signed long );

__intrinsic int         __QFlag( void );

__intrinsic signed long __SMUL( signed short, signed short );

/* Architecture v6, REV and REVSH are also available in thumb mode */
__intrinsic unsigned long __REV( unsigned long );
__intrinsic signed long __REVSH( short );

__intrinsic unsigned long __LDREX( unsigned long * );
__intrinsic unsigned long __STREX( unsigned long, unsigned long * );

/* Architecture v6 Media instructions.... */
__intrinsic unsigned long __SEL( unsigned long op1, unsigned long op2 );

__intrinsic unsigned long __SADD8    (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __SADD16   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __SSUB8    (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __SSUB16   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __SADDSUBX (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __SSUBADDX (unsigned long pair1, unsigned long pair2);

__intrinsic unsigned long __SHADD8   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __SHADD16  (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __SHSUB8   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __SHSUB16  (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __SHADDSUBX(unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __SHSUBADDX(unsigned long pair1, unsigned long pair2);

__intrinsic unsigned long __QADD8    (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __QADD16   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __QSUB8    (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __QSUB16   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __QADDSUBX (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __QSUBADDX (unsigned long pair1, unsigned long pair2);

__intrinsic unsigned long __UADD8    (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UADD16   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __USUB8    (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __USUB16   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UADDSUBX (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __USUBADDX (unsigned long pair1, unsigned long pair2);

__intrinsic unsigned long __UHADD8   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UHADD16  (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UHSUB8   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UHSUB16  (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UHADDSUBX(unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UHSUBADDX(unsigned long pair1, unsigned long pair2);

__intrinsic unsigned long __UQADD8   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UQADD16  (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UQSUB8   (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UQSUB16  (unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UQADDSUBX(unsigned long pair1, unsigned long pair2);
__intrinsic unsigned long __UQSUBADDX(unsigned long pair1, unsigned long pair2);

/* Architecture v7 instructions.... */
__intrinsic void __DMB(void);
__intrinsic void __DSB(void);
__intrinsic void __ISB(void);

/*
 * The following intrinsic function aliases are supplied due to instruction
 * set changes made by ARM. All previous mnemonics of the form *ADDSUBX or
 * *SUBADDX have been changed into *ASX and *SAX, respectively.
 * For increased readability and consistency, the suggested intrinsic
 * functions to use are the new short form versions defined below.
 */

#define __SASX  __SADDSUBX
#define __SSAX  __SSUBADDX
#define __SHASX __SHADDSUBX
#define __SHSAX __SHSUBADDX
#define __QASX  __QADDSUBX
#define __QSAX  __QSUBADDX

#define __UASX  __UADDSUBX
#define __USAX  __USUBADDX
#define __UHASX __UHADDSUBX
#define __UHSAX __UHSUBADDX
#define __UQASX __UQADDSUBX
#define __UQSAX __UQSUBADDX

#pragma language=restore

#ifdef __cplusplus
}
#endif

#endif  /* __INTRINSICS_INCLUDED */
