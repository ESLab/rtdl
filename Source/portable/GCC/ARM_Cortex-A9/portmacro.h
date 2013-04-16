/*
    FreeRTOS V7.0.1 - Copyright (C) 2011 Real Time Engineers Ltd.
	

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/


#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------
 * Port specific definitions.  
 *
 * The settings in this file configure FreeRTOS correctly for the
 * given hardware and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Type definitions. */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	unsigned portLONG
#define portBASE_TYPE	long

typedef struct xMIT {
	void		*phys_start;
	portLONG	 phys_data_size;
	void		*phys_end;
	void		*phys_entry_point;
	void		*phys_heap_begin;
	portLONG	 phys_heap_size;
	void		*phys_iccs_begin;
	portLONG	 phys_iccs_size;
} xMemoryInformationType;

#if( configUSE_16_BIT_TICKS == 1 )
	typedef unsigned portSHORT portTickType;
	#define portMAX_DELAY ( portTickType ) 0xffff
#else
	typedef unsigned portLONG portTickType;
	#define portMAX_DELAY ( portTickType ) 0xffffffff
#endif
/*-----------------------------------------------------------*/	

/* Architecture specifics. */
#define portSTACK_GROWTH			( -1 )
#define portTICK_RATE_MS			( ( portTickType ) 1000 / configTICK_RATE_HZ )		
#define portBYTE_ALIGNMENT			8
#define portHEAP_END				( ( (char * )&_data + (unsigned long)configTOTAL_HEAP_SIZE ) )
#define portMAX_CORE_ID			(3)
/*-----------------------------------------------------------*/	

/* Critical section management. */
extern void vTaskEnterCritical();
extern void vTaskExitCritical();
#define portCRITICAL_NESTING_IN_TCB			1
#define portENTER_CRITICAL()				vTaskEnterCritical()
#define portEXIT_CRITICAL()					vTaskExitCritical()

extern portBASE_TYPE xPortSetInterruptMask(void);
extern void vPortClearInterruptMask(portBASE_TYPE);
#define portSET_INTERRUPT_MASK()				xPortSetInterruptMask()
#define portCLEAR_INTERRUPT_MASK(x)				vPortClearInterruptMask(x)
#define portSET_INTERRUPT_MASK_FROM_ISR()		portSET_INTERRUPT_MASK()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	portCLEAR_INTERRUPT_MASK(x)
#define portDISABLE_INTERRUPTS()				((void)portSET_INTERRUPT_MASK())
/* #define portENABLE_INTERRUPTS()					portCLEAR_INTERRUPT_MASK(configLOWEST_INTERRUPT_PRIORITY) */
static inline void portENABLE_INTERRUPTS(void)
{
	portCLEAR_INTERRUPT_MASK(configLOWEST_INTERRUPT_PRIORITY);
	__asm__ __volatile__ ( "nop" ); /* Allow the yield SGI time to propagate. */
}

/*-----------------------------------------------------------*/

/* Task function macros as described on the FreeRTOS.org WEB site. */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )

#define portNOP()

static inline unsigned long portCORE_ID(void)
{
	unsigned long val;
	__asm(" mrc p15,0,%[val],c0,c0,5\n":[val] "=r" (val)::);
	return val&3;
}

#if configPLATFORM == 0
	/* The System Control Registers. */
	#define portSYSCTRL_ZERO_REG				( ( volatile unsigned long * ) 0x10001000 )		/* realview-pbx-a9 */
	#define portSYSCTRL_ONE_REG					( ( volatile unsigned long * ) 0x1001A000 )		/* realview-pbx-a9 */
	#define portSYSCTRL_ZERO_TIMER0_ENABLE		( 0x00018000 )

	/* Peripheral Base. */
	#define portPERIPHBASE							( 0x1F000000UL )	/* realview-pbx-a9 */
	#define portGIC_PRIVATE_BASE					( portPERIPHBASE + 0x100UL )
	#define portGIC_DISTRIBUTOR_BASE				( portPERIPHBASE + 0x1000UL )
	#define portGIC2_BASE							( 0x1DFFFF00UL )	/* realview-pbx-a9 */
	#define portGIC2_DISTRIBUTOR_BASE				( 0x1E000000UL )	/* realview-pbx-a9 */
	#define portEXCEPTION_VECTORS_BASE				( 0x0UL )
	#define portMAX_VECTORS							( 64UL )
#elif configPLATFORM == 1
	/* Peripheral Base. */
	#define portPERIPHBASE							( 0x50040000UL )	/* nVidia Tegra 2 */
	#define portGIC_PRIVATE_BASE					( portPERIPHBASE + 0x100UL )
	#define portGIC_PRIMARY_BASE					( 0x60004000UL )	/* nVidia Tegra 2 */
	#define portGIC_SECONDARY_BASE					( 0x60004100UL )	/* nVidia Tegra 2 */
	#define portGIC_TERTIARY_BASE					( 0x60004200UL )	/* nVidia Tegra 2 */
	#define portGIC_QUATERNARY_BASE					( 0x60004300UL )	/* nVidia Tegra 2 */
	#define portGIC_DISTRIBUTOR_BASE				( portPERIPHBASE + 0x1000UL )
	#define portEXCEPTION_VECTORS_BASE				( 0x00000000UL )	/* nVidia Tegra 2 */
	#define portMAX_VECTORS							( 160UL )
#elif configPLATFORM == 2
	/* Peripheral Base. */
	#define portPERIPHBASE							( 0x1E000000 )		/* vexpress-a9 */
	#define portGIC_PRIVATE_BASE					( portPERIPHBASE + 0x100UL )
	#define portGIC_DISTRIBUTOR_BASE				( portPERIPHBASE + 0x1000UL )
//	#define portEXCEPTION_VECTORS_BASE				( 0x0UL )
	#define portEXCEPTION_VECTORS_BASE				( portCORE_ID()*0x1000000 )
	#define portMAX_VECTORS							( 32UL )
#endif

/* Snoop Control Unit Processor Registers. */
#define portSCU_BASE						( portPERIPHBASE + 0x00UL )
#define portSCU_CONTROL						( ( volatile unsigned long * ) ( portSCU_BASE + 0x00UL ) )
#define portSCU_CONFIGURATION				( ( volatile unsigned long * ) ( portSCU_BASE + 0x04UL ) )

/* GIC Processor Registers. */
#define portGIC_ICCICR(x)					( ((unsigned long)(x)) + 0x00UL )
#define portGIC_ICCPMR(x)					( ((unsigned long)(x)) + 0x04UL )
#define portGIC_ICCBPR(x)					( ((unsigned long)(x)) + 0x08UL )
#define portGIC_ICCIAR(x)					( ((unsigned long)(x)) + 0x0CUL )
#define portGIC_ICCEOIR(x)					( ((unsigned long)(x)) + 0x10UL )
#define portGIC_ICCRPR(x)					( ((unsigned long)(x)) + 0x14UL )
#define portGIC_ICCHPIR(x)					( ((unsigned long)(x)) + 0x18UL )

/* GIC Distributor Registers. */
#define portGIC_ICDDCR(x)					( ((unsigned long)(x)) + 0x00UL )
#define portGIC_ICDICTR(x)					( ((unsigned long)(x)) + 0x04UL )
#define portGIC_ICDISER_BASE(x)				( ((unsigned long)(x)) + 0x100UL )
#define portGIC_ICDICER_BASE(x)				( ((unsigned long)(x)) + 0x180UL )
#define portGIC_ICDISPR_BASE(x)				( ((unsigned long)(x)) + 0x200UL )
#define portGIC_ICDICPR_BASE(x)				( ((unsigned long)(x)) + 0x280UL )
#define portGIC_ICDABR_BASE(x)				( ((unsigned long)(x)) + 0x300UL )
#define portGIC_ICDIPR_BASE(x)				( ((unsigned long)(x)) + 0x400UL )
#define portGIC_ICDIPTR_BASE(x)				( ((unsigned long)(x)) + 0x800UL )
#define portGIC_ICDICR_BASE(x)				( ((unsigned long)(x)) + 0xC00UL )
#define portGIC_ICDPPIS(x)					( ((unsigned long)(x)) + 0xD00UL )
#define portGIC_ICDSPIS_BASE(x)				( ((unsigned long)(x)) + 0xD04UL )
#define portGIC_ICDSGIR(x)					( ((unsigned long)(x)) + 0xF00UL )
#define portGIC_ICDCPENDGIR(x)				( ((unsigned long)(x)) + 0xF10UL )
#define portGIC_ICDSPENDGIR(x)				( ((unsigned long)(x)) + 0xF20UL )

/* Macros to access the GIC. */
#define portGIC_WRITE(address,value)		( *( ( unsigned long * volatile )( address ) ) = ( value ) )
#define portGIC_SET(address,value)			( *( ( unsigned long * volatile )( address ) ) |= ( value ) )
#define portGIC_READ(address)				( *( ( unsigned long * volatile )( address ) ) )
#define portGIC_CLEAR(address,value)		( *( ( unsigned long * volatile )( address ) ) &= ~( value ) )

/* GIC Values. */
#define portGIC_CONTROL_ENABLE_INTERRUPTS	( 0x01UL )
#define portGIC_CONTROL_DISABLE_INTERRUPTS	( 0x00UL )
#define portGIC_PRIORITY_MASK_MASK			( 0xF0UL )
#define portGIC_SPURIOUS_VECTOR				( 1023 )
#define portGIC_CPU_INTERRUPT_SOURCE_MASK	( 0x1C00UL )
#define portGIC_VECTOR_MASK					( 0x3FFUL )

/* Private Timers. */
#define portSYSTICK_BASE				( portPERIPHBASE + 0x600 )
#define portSYSTICK_LOAD				( ( volatile unsigned long * ) ( portSYSTICK_BASE + 0x00 ) )
#define portSYSTICK_CONTROL				( ( volatile unsigned long * ) ( portSYSTICK_BASE + 0x08 ) )
#define portSYSTICK_INTERRUPT_STATUS	( ( volatile unsigned long * ) ( portSYSTICK_BASE + 0x0C ) )
#define portSYSTICK_CTRL_ENABLE_PERIODIC_INTERRUPTS			  ( 0x00000007 )
#define portSYSTICK_PRESCALE			( 99 )		/* realview-pbx-a9 */
#define portSYSTICK_VECTOR_ID			( 29 )

/* SGI for Yielding Task. */
#define portSGI_YIELD_VECTOR_ID			( 1 )
#define portSGI_YIELD( xCPUID )			( ( 0 << 24 ) | ( ( 1 << 16 ) << ( xCPUID ) ) | portSGI_YIELD_VECTOR_ID )
/* #define portYIELD()		( ( portGIC_READ( portGIC_ICDISPR_BASE( portGIC_DISTRIBUTOR_BASE ) ) & portSGI_YIELD_VECTOR_ID ) == 0UL ? portGIC_WRITE( portGIC_ICDSGIR( portGIC_DISTRIBUTOR_BASE ), portSGI_YIELD( portCORE_ID() ) ) : (void)portGIC_DISTRIBUTOR_BASE ) */

static inline void portYIELD(void)
{
	if( ( portGIC_READ( portGIC_ICDISPR_BASE( portGIC_DISTRIBUTOR_BASE ) ) & portSGI_YIELD_VECTOR_ID ) == 0UL)
	{
		portGIC_WRITE( portGIC_ICDSGIR( portGIC_DISTRIBUTOR_BASE ), portSGI_YIELD( portCORE_ID() ) );
		__asm__ __volatile__ ( "nop" ); /* Allow the yield SGI time to propagate. */
	}
}
#define portSGI_CLEAR_YIELD( pxDistributorBase, xCPUID )	portGIC_WRITE( portGIC_ICDICPR_BASE( pxDistributorBase ), portSGI_YIELD_VECTOR_ID )
#define portEND_SWITCHING_ISR( xSwitchRequired ) ( (xSwitchRequired) ? portYIELD() : (void)xSwitchRequired )

/* Processor Mode Definitions (CPSR) */
#define portPROCESSOR_MODE_MASK	( ~(0x1FUL) )
#define portUSER_MODE			( 0x10UL )
#define portFIQ_MODE			( 0x11UL )
#define portIRQ_MODE			( 0x12UL )
#define portSVC_MODE			( 0x13UL )
#define portMON_MODE			( 0x16UL )
#define portABT_MODE			( 0x17UL )
#define portUND_MODE			( 0x1BUL )
#define portSYS_MODE			( 0x1FUL )

/* Stack Size definitions for each of the modes. */
#define portFIQ_STACK_SIZE		( 256 )
#define portIRQ_STACK_SIZE		( 256 )
#define portABORT_STACK_SIZE	( 256 )
#define portSVC_STACK_SIZE		( 256 )

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */

