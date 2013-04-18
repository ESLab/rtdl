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

/* UART Driver for the PL011 Peripheral. */

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
/*----------------------------------------------------------------------------*/

#define UART_USE_INTERRUPT			1

#define UART0_BASE			( 0x70006000UL )			/* nVidia Tegra 2 - UART A */
#define UART1_BASE			( 0x70006040UL )			/* nVidia Tegra 2 - UART B */
#define UART2_BASE			( 0x70006200UL )			/* nVidia Tegra 2 - UART C */
#define UART3_BASE			( 0x70006300UL )			/* nVidia Tegra 2 - UART D */
#define UART4_BASE			( 0x70006400UL )			/* nVidia Tegra 2 - UART E */

#define UART_THR_DLAB(x)	( (unsigned long *)	( (x) + 0x0000UL ) )	/* Register */
#define UART_IER_DLAB(x)	( (unsigned long *)	( (x) + 0x0004UL ) )	/* Register */
#define UART_IIR_FCR(x)		( (unsigned long *)( (x) + 0x0008UL ) )		/* Register */
#define UART_LCR(x)			( (unsigned long *)	( (x) + 0x000CUL ) )	/* Register */
#define UART_MCR(x)			( (unsigned long *)( (x) + 0x0010UL ) )		/* Register */
#define UART_LSR(x)			( (unsigned long *)	( (x) + 0x0014UL ) )	/* Register */
#define UART_MSR(x)			( (unsigned long *)	( (x) + 0x0018UL ) )	/* Register */
#define UART_SPR(x)			( (unsigned long *)	( (x) + 0x001CUL ) )	/* Register */
#define UART_IRDA_CSR(x)	( (unsigned long *)( (x) + 0x0020UL ) )		/* Register */
#define UART_ASR(x)			( (unsigned long *)	( (x) + 0x003CUL ) )	/* Register */

#define UART_IER_DMA_ENABLE 			( 1 << 7 )
#define UART_IER_UNIT_ENABLE 			( 1 << 6 )
#define UART_IER_NRZ_ENABLE 			( 1 << 5 )
#define UART_IER_RX_TIMEOUT_INT_ENABLE	( 1 << 4 )
#define UART_IER_MODEM_INT_ENABLE 		( 1 << 3 )
#define UART_IER_RXLS_INT_ENABLE 		( 1 << 2 )
#define UART_IER_TXDR_INT_ENABLE 		( 1 << 1 )
#define UART_IER_RXDA_INT_ENABLE 		( 1 << 0 )

#define UART_IIR_FIFO_ENABLE_STATUS_MASK	( 3 << 6 )
#define UART_IIR_TIMEOUT_STATUS_MASK		( 1 << 3 )
#define UART_IIR_INTERRUPT_STATUS_MASK		( 3 << 1 )
#define UART_IIR_INTERRUPT_STATUS_RX_ERROR	( 3 << 1 )
#define UART_IIR_INTERRUPT_STATUS_RX_DR		( 1 << 2 )
#define UART_IIR_INTERRUPT_STATUS_TX		( 1 << 1 )
#define UART_IIR_INTERRUPT_PENDING_MASK		( 1 << 0 )

#define UART_FCR_INTERRUPT_TRIGGER_LEVEL_MASK	( 3 << 6 )
#define UART_FCR_RESET_TX_EMPTY_INT_FLAG		( 1 << 2 )
#define UART_FCR_RESET_TX_FLAG					( 1 << 2 )
#define UART_FCR_RESET_RX_FLAG					( 1 << 1 )
#define UART_FCR_TX_RX_ENABLE_FLAG				( 1 << 0 )

#define UART_LCR_DIVISOR_LATCH_ACCESS_FLAG	( 1 << 7 )
#define UART_LCR_SET_BREAK_FLAG				( 1 << 6 )
#define UART_LCR_STICKY_PARITY_FLAG			( 1 << 5 )
#define UART_LCR_EVEN_PARITY_FLAG			( 1 << 4 )
#define UART_LCR_PARITY_ENABLE_FLAG			( 1 << 3 )
#define UART_LCR_STOP_BIT_FLAG				( 1 << 2 )
#define UART_LCR_WORD_LENGTH_SELECT_MASK	( 3 << 0 )

#define UART_LSR_FIFO_ERROR_FLAG		( 1 << 7 )
#define UART_LSR_TXE_FLAG				( 1 << 6 )
#define UART_LSR_TXDR_FLAG				( 1 << 5 )
#define UART_LSR_BI_FLAG				( 1 << 4 )
#define UART_LSR_FRAME_ERROR_FLAG		( 1 << 3 )
#define UART_LSR_PARITY_ERROR_FLAG		( 1 << 2 )
#define UART_LSR_OVERRUN_ERROR_FLAG		( 1 << 1 )
#define UART_LSR_RXDR_FLAG				( 1 << 0 )

#define UART_MCR_LOOP_FLAG	( 1 << 4 )
#define UART_MCR_OUT2_FLAG	( 1 << 3 )
#define UART_MCR_OUT1_FLAG	( 1 << 2 )
#define UART_MCR_RTS_FLAG	( 1 << 1 )
#define UART_MCR_DTR_FLAG	( 1 << 0 )

#define UART_MSR_DCD_FLAG	( 1 << 7 )
#define UART_MSR_RI_FLAG	( 1 << 6 )
#define UART_MSR_DSR_FLAG	( 1 << 5 )
#define UART_MSR_CTS_FLAG	( 1 << 4 )
#define UART_MSR_DDCD_FLAG	( 1 << 3 )
#define UART_MSR_TERI_FLAG	( 1 << 2 )
#define UART_MSR_DDSR_FLAG	( 1 << 1 )
#define UART_MSR_DCTS_FLAG	( 1 << 0 )


#define UART_CLK_HZ				( 216000000UL )
#define UART0_VECTOR_ID			( 32 )

#define UART_FIFO_SIZE_BYTES	( 32UL )
#define UART1_VECTOR_ID			( 33 )
#define UART2_VECTOR_ID			( 34 )
#define UART3_VECTOR_ID			( 35 )
#define UART4_VECTOR_ID			( 36 )

#define TX_QUEUE				( 0 )
#define RX_QUEUE				( 1 )
/*----------------------------------------------------------------------------*/

#if configPLATFORM == 1

static xQueueHandle xUartQueues[5][2] = { { NULL, NULL } };
/*----------------------------------------------------------------------------*/

#if UART_USE_INTERRUPT

void vUARTInterruptHandler( void *pvBaseAddress )
{
unsigned long ulBase = (unsigned long)pvBaseAddress;
unsigned short usStatus = 0;
unsigned short usSource = 0;
unsigned char ucReceiveChar = 0;
portBASE_TYPE xTaskWoken = pdFALSE;
xQueueHandle xTxQueue = NULL;
xQueueHandle xRxQueue = NULL;
unsigned long ulUART = 0;

	/* Select the UART Queues. */
	switch ( ulBase )
	{
	case UART0_BASE:
		ulUART = 0;
		break;
	case UART1_BASE:
		ulUART = 1;
		break;
	case UART2_BASE:
		ulUART = 2;
		break;
	case UART3_BASE:
		ulUART = 3;
		break;
	case UART4_BASE:
		ulUART = 4;
		break;
	}

	xTxQueue = xUartQueues[ulUART][TX_QUEUE];
	xRxQueue = xUartQueues[ulUART][RX_QUEUE];

	/* Find the cause of the interrupt */
	usSource = *UART_IIR_FCR(ulBase);

	/* Has an error occurred? */
	if ( UART_IIR_INTERRUPT_STATUS_RX_ERROR == ( usSource & UART_IIR_INTERRUPT_STATUS_MASK ) )
	{
		/* Read the Character to discard it. */
		do
		{
			ucReceiveChar = *UART_THR_DLAB(UART0_BASE);
		} while ( *UART_LSR(ulBase) & UART_LSR_FIFO_ERROR_FLAG );
	}

	/* Have we received something? */
	if ( ( UART_IIR_INTERRUPT_STATUS_RX_DR  == ( usSource & UART_IIR_INTERRUPT_STATUS_MASK ) )
			|| ( usSource & UART_IIR_TIMEOUT_STATUS_MASK ) )
	{
		while ( *UART_LSR(ulBase) & UART_LSR_RXDR_FLAG )
		{
			/* Read the Character. */
			ucReceiveChar = *UART_THR_DLAB(ulBase);
			/* And pass to the application. */
			if ( pdTRUE != xQueueSendFromISR( xRxQueue, &ucReceiveChar, &xTaskWoken ) )
			{
				/* Receive Queue is Full. */
				break;
				/* Not good as we are going to lose this character. */
			}
		}
	}

	/* Or was this an interrupt on completed transmit? */
	if ( UART_IIR_INTERRUPT_STATUS_TX == ( usSource & UART_IIR_INTERRUPT_STATUS_MASK ) )
	{
		/* Need to give as many times as spots that have been freed in the FIFO. */
		while ( pdTRUE == xSemaphoreGiveFromISR( xTxQueue, &xTaskWoken ) )
		{
			/* Is there still something in the buffer? */
			if ( 0 == ( *UART_LSR(ulBase) & UART_LSR_TXE_FLAG ) )
			{
				/* Yes there is so only allow one character back. */
				break;
			}
		}
		if ( 0 != ( *UART_LSR(ulBase) & UART_LSR_TXE_FLAG ) )
		{
			*UART_IER_DLAB(ulBase) &= ~UART_IER_TXDR_INT_ENABLE;
		}
	}

	/* Should also handle errors. */


	/* Finally, acknowledge the interrupt. */
	*UART_LSR(ulBase) = usStatus;

	/* Finally, switch task if necessary. */
	portEND_SWITCHING_ISR(xTaskWoken);
}
/*----------------------------------------------------------------------------*/

#endif /* UART_USE_INTERRUPT */

void vUARTInitialise(unsigned long ulUARTPeripheral, unsigned long ulBaud, unsigned long ulQueueSize )
{
extern void vPortInstallInterruptHandler( void (*vHandler)(void *), void *pvParameter, unsigned long ulVector, unsigned char ucEdgeTriggered, unsigned char ucPriority, unsigned char ucProcessorTargets );
unsigned long ulBase = 0;
unsigned long ulVectorID = 0;

	switch ( ulUARTPeripheral )
	{
	case 0:
		ulBase = UART0_BASE;
		ulVectorID = UART0_VECTOR_ID;
		/* Create the Queues to Handle the bytes. */
		xUartQueues[0][TX_QUEUE] = xSemaphoreCreateCounting( ulQueueSize, ulQueueSize );
		xUartQueues[0][RX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		break;
	case 1:
		ulBase = UART1_BASE;
		ulVectorID = UART1_VECTOR_ID;
		/* Create the Queues to Handle the bytes. */
		xUartQueues[1][TX_QUEUE] = xSemaphoreCreateCounting( ulQueueSize, ulQueueSize );
		xUartQueues[1][RX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		break;
	case 2:
		ulBase = UART2_BASE;
		ulVectorID = UART2_VECTOR_ID;
		/* Create the Queues to Handle the bytes. */
		xUartQueues[2][TX_QUEUE] = xSemaphoreCreateCounting( ulQueueSize, ulQueueSize );
		xUartQueues[2][RX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		break;
	case 3:
		ulBase = UART3_BASE;
		ulVectorID = UART3_VECTOR_ID;
		/* Create the Queues to Handle the bytes. */
		xUartQueues[3][TX_QUEUE] = xSemaphoreCreateCounting( ulQueueSize, ulQueueSize );
		xUartQueues[3][RX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		break;
	case 4:
		ulBase = UART4_BASE;
		ulVectorID = UART4_VECTOR_ID;
		/* Create the Queues to Handle the bytes. */
		xUartQueues[4][TX_QUEUE] = xSemaphoreCreateCounting( ulQueueSize, ulQueueSize );
		xUartQueues[4][RX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		break;
	}

	/* Disable and program some of the baudrate */
	*UART_IIR_FCR(ulBase) = 0;
	*UART_IER_DLAB(ulBase) = 0;
	*UART_LCR(ulBase) = UART_LCR_DIVISOR_LATCH_ACCESS_FLAG;
	*UART_THR_DLAB(ulBase) = ( ( UART_CLK_HZ / 16 ) / 115200 );
	*UART_LCR(ulBase) = 0;
	*UART_IER_DLAB(ulBase) = 0;

	/* Program the Mode */
	*UART_IIR_FCR(ulBase) = UART_FCR_RESET_TX_FLAG | UART_FCR_RESET_RX_FLAG;
	*UART_LCR(ulBase) = ( 3 & UART_LCR_WORD_LENGTH_SELECT_MASK ) | UART_FCR_RESET_TX_EMPTY_INT_FLAG;	/* 8 bits, No parity, 1 stop bit. */
	if ( ulUARTPeripheral != 0 )
	{
		*UART_MCR(ulBase) = UART_MCR_RTS_FLAG | UART_MCR_OUT2_FLAG | UART_MCR_LOOP_FLAG;
	}
	else
	{
		*UART_MCR(ulBase) = UART_MCR_RTS_FLAG | UART_MCR_OUT2_FLAG;
	}
	*UART_MSR(ulBase) = 0;
	*UART_SPR(ulBase) = 0;
	*UART_IRDA_CSR(ulBase) = 0;
	*UART_ASR(ulBase) = 0;

#if UART_USE_INTERRUPT
	/* Finally enable the interrupts. */
	vPortInstallInterruptHandler( vUARTInterruptHandler, (void *)ulBase, ulVectorID, pdFALSE, configMAX_SYSCALL_INTERRUPT_PRIORITY, 1 );
	*UART_IER_DLAB(ulBase) = UART_IER_UNIT_ENABLE | UART_IER_RX_TIMEOUT_INT_ENABLE | UART_IER_RXLS_INT_ENABLE | UART_IER_TXDR_INT_ENABLE | UART_IER_RXDA_INT_ENABLE;
#endif /* UART_USE_INTERRUPT */

	/* Enable the Device. */
	*UART_IIR_FCR(ulBase) = UART_FCR_TX_RX_ENABLE_FLAG;
}
/*----------------------------------------------------------------------------*/

portBASE_TYPE xUARTSendCharacter( unsigned long ulUARTPeripheral, unsigned char ucChar, portTickType xDelay )
{
xQueueHandle xTxQueue = NULL;
unsigned long ulBase = 0;
portBASE_TYPE xReturn = pdFALSE;

	switch ( ulUARTPeripheral )
	{
	case 0:
		ulBase = UART0_BASE;
		xTxQueue = xUartQueues[0][TX_QUEUE];
		break;
	case 1:
		ulBase = UART1_BASE;
		xTxQueue = xUartQueues[1][TX_QUEUE];
		break;
	case 2:
		ulBase = UART2_BASE;
		xTxQueue = xUartQueues[2][TX_QUEUE];
		break;
	case 3:
		ulBase = UART3_BASE;
		xTxQueue = xUartQueues[3][TX_QUEUE];
		break;
	case 4:
		ulBase = UART4_BASE;
		xTxQueue = xUartQueues[4][TX_QUEUE];
		break;
	}

	if ( 0 != ulBase )
	{
#if UART_USE_INTERRUPT
		/* Add to the queue regardless. */
		xReturn = xSemaphoreTake( xTxQueue, xDelay );
		/* Did we time out? */
		if ( pdTRUE == xReturn )
		{
			/* Free to Transmit. */
			*UART_THR_DLAB(ulBase) = ucChar;
			*UART_IER_DLAB(ulBase) |= UART_IER_TXDR_INT_ENABLE;
		}
#else
		while ( ( *UART_LSR(ulBase) & UART_LSR_TXE_FLAG ) == 0 );
		*UART_THR_DLAB(ulBase) = ucChar;
		xReturn = pdTRUE;
#endif	/* UART_USE_INTERRUPT */
	}
	return xReturn;
}
/*----------------------------------------------------------------------------*/

portBASE_TYPE xUARTReceiveCharacter( unsigned long ulUARTPeripheral, unsigned char *pucChar, portTickType xDelay )
{
xQueueHandle xRxQueue = NULL;
unsigned long ulBase = 0;
portBASE_TYPE xReturn = pdFALSE;

	switch ( ulUARTPeripheral )
	{
	case 0:
		ulBase = UART0_BASE;
		xRxQueue = xUartQueues[0][RX_QUEUE];
		break;
	case 1:
		ulBase = UART1_BASE;
		xRxQueue = xUartQueues[1][RX_QUEUE];
		break;
	case 2:
		ulBase = UART2_BASE;
		xRxQueue = xUartQueues[2][RX_QUEUE];
		break;
	case 3:
		ulBase = UART3_BASE;
		xRxQueue = xUartQueues[3][RX_QUEUE];
		break;
	case 4:
		ulBase = UART4_BASE;
		xRxQueue = xUartQueues[4][RX_QUEUE];
		break;
	}

	if ( 0 != ulBase )
	{
#if UART_USE_INTERRUPT
		xReturn = xQueueReceive( xRxQueue, pucChar, xDelay );
#else
		if ( ( *UART_LSR(ulBase) & UART_LSR_RXDR_FLAG ) != 0 )
		{
			*pucChar = *UART_THR_DLAB(ulBase);
			xReturn = pdTRUE;
		}
#endif /* UART_USE_INTERRUPT */
	}

	return xReturn;
}
/*----------------------------------------------------------------------------*/

#endif /* configPLATFORM == 1 */
