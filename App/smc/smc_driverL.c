/***********************************************************************************/
/* Copyright (c) 2013, Simon Holmbacka. All rights reserved.			   */
/* Copyright (c) 2013, Åbo Akademi University. All rights reserved.		   */
/* 										   */
/* Redistribution and use in source and binary forms, with or without		   */
/* modification, are permitted provided that the following conditions are met:	   */
/*      * Redistributions of source code must retain the above copyright	   */
/*        notice, this list of conditions and the following disclaimer.		   */
/*      * Redistributions in binary form must reproduce the above copyright	   */
/*        notice, this list of conditions and the following disclaimer in the	   */
/*        documentation and/or other materials provided with the distribution.	   */
/*      * Neither the name of the Åbo Akademi University nor the		   */
/*        names of its contributors may be used to endorse or promote products	   */
/*        derived from this software without specific prior written permission.	   */
/* 										   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND */
/* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   */
/* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE	   */
/* DISCLAIMED. IN NO EVENT SHALL ÅBO AKADEMI UNIVERSITY BE LIABLE FOR ANY	   */
/* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES	   */
/* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;	   */
/* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND	   */
/* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT	   */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS   */
/* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 		   */
/***********************************************************************************/

#include "smc_driverL.h"
#include "gpio.h"
#include "at91sam9_smc.h"
#include "at91_pio.h"
#include "at91sam9g45.h" 
#include <stdint.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>

#define writel(value, address) \
	(*(volatile unsigned int *)(address)) = (value)
#define readl(address) \
	(*(volatile unsigned int *)(address))

//#define NULL 0

uint32_t gpio_reg_base[] = {
  0xFFFFF200,
  0xFFFFF400,
  0xFFFFF600,
  0xFFFFF800,
  0xFFFFFA00
};

#define AT91_IO_PHYS_BASE	AT91_BASE_SYS
#define AT91_IO_VIRT_BASE	AT91_IO_PHYS_BASE
#define AT91_IO_P2V(x)		((x) - AT91_IO_PHYS_BASE + AT91_IO_VIRT_BASE)
#define AT91_VA_BASE_SYS	AT91_IO_P2V(AT91_BASE_SYS)

static inline void at91_sys_write(unsigned int reg_offset, unsigned long value)
{
  void *addr = (void *)AT91_VA_BASE_SYS;
  writel(value, addr + reg_offset);
}


static inline void *pin_to_controller(unsigned pin)
{
	pin -= PIN_BASE;
	pin /= 32;
	return (void*)gpio_reg_base[pin];
}

static inline unsigned pin_to_mask(unsigned pin)
{
	pin -= PIN_BASE;
	return 1 << (pin % 32);
}

int at91_set_A_periph(unsigned pin, int use_pullup)
{
	//void __iomem	*pio = pin_to_controller(pin);
	void *pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		//return -EINVAL;
	  return 1;

	writel(mask, pio + PIO_IDR);
	writel(mask, pio + (use_pullup ? PIO_PUER : PIO_PUDR));
	writel(mask, pio + PIO_ASR);
	writel(mask, pio + PIO_PDR);
	return 0;
}


#define EINVAL 1
#define __raw_writel writel
#define __iomem
#define __init_or_module


int __init_or_module at91_set_gpio_output(unsigned pin, int value)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;

	__raw_writel(mask, pio + PIO_IDR);
	__raw_writel(mask, pio + PIO_PUDR);
	__raw_writel(mask, pio + (value ? PIO_SODR : PIO_CODR));
	__raw_writel(mask, pio + PIO_OER);
	__raw_writel(mask, pio + PIO_PER);
	return 0;
}


int at91_set_gpio_value(unsigned pin, int value)
{
	void __iomem	*pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -EINVAL;
	__raw_writel(mask, pio + (value ? PIO_SODR : PIO_CODR));
	return 0;
}


void init_smc_ddr()
{
#if 0
	printf("leds should change now!\n");
  	at91_set_gpio_output(AT91_PIN_PA0, 1);
	at91_set_gpio_output(AT91_PIN_PA1, 0);

	vTaskDelay(2000);
	printf("leds should change now!\n");
	at91_set_gpio_value(AT91_PIN_PA0, 0);
	at91_set_gpio_value(AT91_PIN_PA1, 1);
#endif
  
/* Configure the EBI1 pins for the wr switch */
	int i;
	
	/* PC16..31: periphA as EBI1_D16..31 */
	for (i = AT91_PIN_PC16; i <= AT91_PIN_PC31; i++){
		at91_set_A_periph(i, 0);
	}
	/* PC2 and PC3 too: EBI1_A19 EBI1_A20 */
	at91_set_A_periph(AT91_PIN_PC2, 0);
	at91_set_A_periph(AT91_PIN_PC3, 0);
	
	
	/* FIXME: We should pull rst high for when it is programmed */

	/* Then, write the EBI1 configuration (NCS0 == 0x1000.0000) */
	at91_sys_write(AT91_SMC_SETUP(0),
				AT91_SMC_NWESETUP_(4) |
				AT91_SMC_NCS_WRSETUP_(2) |
				AT91_SMC_NRDSETUP_(4) |
				AT91_SMC_NCS_RDSETUP_(2));
	at91_sys_write(AT91_SMC_PULSE(0),
				AT91_SMC_NWEPULSE_(30) |
				AT91_SMC_NCS_WRPULSE_(34) |
				AT91_SMC_NRDPULSE_(30) |
				AT91_SMC_NCS_RDPULSE_(34));
	at91_sys_write(AT91_SMC_CYCLE(0),
				AT91_SMC_NWECYCLE_(40) |
				AT91_SMC_NRDCYCLE_(40));
	at91_sys_write(AT91_SMC_MODE(0),
				AT91_SMC_DBW_32 |
				AT91_SMC_TDF_(0) |
				AT91_SMC_READMODE |
				AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_FROZEN);
	

	
	
}
