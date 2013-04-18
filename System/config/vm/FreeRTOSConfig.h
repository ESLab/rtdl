/*
    FreeRTOS V7.0.1 - Copyright (C) 2011 Real Time Engineers Ltd.


	FreeRTOS supports many tools and architectures. V7.0.0 is sponsored by:
	Atollic AB - Atollic provides professional embedded systems development
	tools for C/C++ development, code analysis and test automation.
	See http://www.atollic.com


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

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION			1
#define configUSE_IDLE_HOOK				0
#define configUSE_TICK_HOOK				0
#define configTICK_RATE_HZ				( ( portTickType ) 1000 )			// 1 ms tick
#define configMAX_PRIORITIES			( ( unsigned portBASE_TYPE ) 10 )
#define configMINIMAL_STACK_SIZE		( ( unsigned portSHORT ) 256 * 4 )
#define configTOTAL_HEAP_SIZE			( ( size_t ) 8 * 1024 * 1024 )
#define configMAX_TASK_NAME_LEN			( 16 )
#define configUSE_TRACE_FACILITY		1
#define configUSE_16_BIT_TICKS			0
#define configIDLE_SHOULD_YIELD			0
#define configUSE_MUTEXES				1
#define configUSE_RECURSIVE_MUTEXES		1
#define configUSE_COUNTING_SEMAPHORES	1
#define configUSE_MALLOC_FAILED_HOOK	1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 			0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet				1
#define INCLUDE_uxTaskPriorityGet				1
#define INCLUDE_vTaskDelete					1
#define INCLUDE_vTaskCleanUpResources			        1
#define INCLUDE_vTaskSuspend					1
#define INCLUDE_vTaskDelayUntil					1
#define INCLUDE_vTaskDelay					1
#define INCLUDE_xTaskGetCurrentTaskHandle 		        1

/* Interrupt Priority Levels. */
#define configLOWEST_INTERRUPT_PRIORITY 		( 0xFF )
#define configKERNEL_INTERRUPT_PRIORITY 		( 0xC0 )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( 0xA0 ) /* equivalent to 0xa0, or priority 5. */

#define configVM_PRE_SETUP					1
#define configMIS_SECTION_ADDRESS				((void *)0x60200000)

/* Select the Platform 0=Realview-PBX-A9, 1=nVidia-Tegra2, 2=Versatile-Express */
#define configPLATFORM							( 2 )

#if configPLATFORM == 0
	#define configCPU_CLOCK_HZ				( ( unsigned portLONG ) 1000000 )
	#define configCPU_PERIPH_HZ				( configCPU_CLOCK_HZ / 2 )
	#define configUART_PORT					( 0 )
	/* Use the IDLE Hook to implement the Echo. */
	#undef configUSE_IDLE_HOOK
	#define configUSE_IDLE_HOOK				1
#elif configPLATFORM == 1
	#define configCPU_CLOCK_HZ				( ( unsigned portLONG ) 2500000 )
	#define configCPU_PERIPH_HZ				( configCPU_CLOCK_HZ / 2 )
	#define configUART_PORT					( 1 )
#elif configPLATFORM == 2
	#define configCPU_CLOCK_HZ				( ( unsigned portLONG ) 2000000 )
	#define configCPU_PERIPH_HZ				( configCPU_CLOCK_HZ / 2 )
	#define configUART_PORT					( portCORE_ID() )
	#undef configUSE_IDLE_HOOK
	#define configUSE_IDLE_HOOK				1
#endif /* configPLATFORM */

#endif /* FREERTOS_CONFIG_H */

