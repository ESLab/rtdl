
#define SET(register, flags)        ((register) = (register) | (flags))
#define CLEAR(register, flags)      ((register) &= ~(flags))
#define ISSET(register, flags)      (((register) & (flags)) == (flags))
#define ISCLEARED(register, flags)  (((register) & (flags)) == 0)

#define CFRNW_pin   (0x1<<14)
#define NWAIT_pin   (0x1<< 5)
#define CFCS0_pin   (0x1<< 6)
#define CFCS1_pin   (0x1<< 7)
#define CFCE1_pin   (0x1<< 8)
#define CFCE2_pin   (0x1<< 9)
#define all_pins   CFRNW_pin | NWAIT_pin | CFCS0_pin | CFCS1_pin | CFCE1_pin | CFCE2_pin   

#define P2_pin (0x1<<2)
#define P3_pin (0x1<<3)
#define P16_pin (0x1<<16)
#define P17_pin (0x1<<17)
#define P18_pin (0x1<<18)
#define P19_pin (0x1<<19)
#define P20_pin (0x1<<20)
#define P21_pin (0x1<<21)
#define P22_pin (0x1<<22)
#define P23_pin (0x1<<23)
#define P24_pin (0x1<<24)
#define P25_pin (0x1<<25)
#define P26_pin (0x1<<26)
#define P27_pin (0x1<<27)
#define P28_pin (0x1<<28)
#define P29_pin (0x1<<29)
#define P30_pin (0x1<<30)
#define P31_pin (0x1<<31)
#define some_pins P2_pin |P3_pin |P16_pin |P17_pin |P18_pin |P19_pin |P20_pin |P21_pin |P22_pin |P23_pin |P24_pin |P25_pin |P26_pin |P27_pin |P28_pin |P29_pin |P30_pin |P31_pin 

#define CFRESET_pin      (0x1<<19)
#define CFIRQ_pin      (0x1<<20)
#define CARD_DETECT_pin   (0x1<<22)
#define PMC_PIOD      (0x1<< 4)
#define PMC_PIOB      (0x1<< 3)

//SMC_CTRL - MODE REGISTER
#define READ_MODE      (0x0<< 0)
#define WRITE_MODE      (0x0<< 1)
#define EXNW_MODE      (0x1<< 5) | (0x0<< 4)
#define BAT            (0x0<< 8)
#define DBW            (0x0<<13) | (0x0<<12)
#define DTF_CYCLES      (0x1<<19) | (0x1<<18) | (0x1<<17) | (0x1<<16)
#define DTF_MODE      (0x0<<20)
#define PMEN         (0x0<<24)
#define   PS            (0x0<<29) | (0x0<<28)

//SMC_SETUP
   //X setup length = (128* X_SETUP[5] + X_SETUP[4:0]) clock cycles
   //according to compact flash specyfication p.51
#define NCS_RD_SETUP   (0x32<<24) //(0x32<<24)
#define NRD_SETUP      (0x32<<16) //(0x32<<16)
#define NCS_WR_SETUP   (0x3C<< 8) //(0x3C<< 8)
#define NWE_SETUP      (0x3C<< 0) //(0x3C<< 0)

//SMC_PULSE
   //X pulse length = (256* X_PULSE[6] + X_PULSE[5:0]) clock cycles
   //according to compact flash specyfication p.51
#define NCS_RD_PULSE   (0x05<<24) //(0x05<<24)
#define NRD_PULSE      (0x05<<16) //(0x05<<16)
#define NCS_WR_PULSE   (0xFE<< 8) //(0xFE<< 8)
#define NWE_PULSE      (0xFE<< 0) //(0xFE<< 0)

//SMC_CYCLE
#define NRD_CYCLE      0x00AC0000 //0x003C0000
   //NRD_CYCLE[8:0]=SMC_CYCLE[24:16]
   //Write cycle length = (NWE_CYCLE[8:7]*256 + NWE_CYCLE[6:0]) clock cycles
   //according to compact flash specyfication p.51 write cycle = 600ns
#define NWE_CYCLE      0x000000AC //0x0000003C
   //NWE_CYCLE[8:0]=SMC_CYCLE[ 8: 0]
   //Read  cycle length = (NRD_CYCLE[8:7]*256 + NRD_CYCLE[6:0]) clock cycles
   //according to compact flash specyfication p.51 read  cycle = 600ns


void cfconfigure(void){
   //swith on compact flash on ebi0 on both slots
   /*SET(AT91C_BASE_CCFG->CCFG_EBI0CSA, (AT91C_EBI_CS4A_CF | AT91C_EBI_CS5A_CF));*/
   /*SET(AT91C_BASE_CCFG->CCFG_EBI1CSA, (AT91C_EBI_CS1A_SDRAMC));*/
   SET(CCFG_EBI1CSA, (AT91C_EBI_CS1A_SDRAMC));
   
   //address line A21 s to select IDE mode (0) or Alternate IDE (1)

   //set in pio controler lines CFRNW, CFS0, CFCS1, CFCE1, CFCE2 to be enabled on the pad
      /* Enable the periph clock for the PIO controller */
      /* This is mandatory when PIO are configured as input */
      SET(AT91C_BASE_PMC->PMC_PCER, PMC_PIOD);
      while(!ISSET(AT91C_BASE_PMC->PMC_PCSR, PMC_PIOD));
      SET(AT91C_BASE_PMC->PMC_PCER, PMC_PIOB);
      while(!ISSET(AT91C_BASE_PMC->PMC_PCSR, PMC_PIOB));
      /*enable pull-up resistors*/
      SET(AT91C_BASE_PIOD->PIO_PPUDR, some_pins);
      /*disable multi-driver*/
      SET(AT91C_BASE_PIOD->PIO_MDDR,  some_pins);
      /*enable pin as output*/
      SET(AT91C_BASE_PIOD->PIO_OER,   some_pins);
      /*disable PIO to control pad*/
      SET(AT91C_BASE_PIOD->PIO_PDR,   some_pins);
      /*set priph A to control pad*/
      SET(AT91C_BASE_PIOD->PIO_ASR,   some_pins);
      

   //set pio lines to enable IRQ, reset and card detection
      /*enable pull-up resistors*/
      SET(AT91C_BASE_PIOB->PIO_PPUDR, (CFIRQ_pin | CARD_DETECT_pin));
      /*disable multi-driver*/
      SET(AT91C_BASE_PIOB->PIO_MDDR , (CFIRQ_pin | CARD_DETECT_pin));
      /*enable PIO control*/
      SET(AT91C_BASE_PIOB->PIO_PER  , (CFIRQ_pin | CARD_DETECT_pin));
      /*disable pin as output*/
      SET(AT91C_BASE_PIOB->PIO_ODR  , (CFIRQ_pin | CARD_DETECT_pin));
      /*turn on input glitch filter*/
      SET(AT91C_BASE_PIOB->PIO_IFER , (CFIRQ_pin | CARD_DETECT_pin));
      /*enable pull-up resistors*/
      SET(AT91C_BASE_PIOB->PIO_PPUDR, (CFRESET_pin));
      /*disable multi-driver*/
      SET(AT91C_BASE_PIOB->PIO_MDDR , (CFRESET_pin));
      /*enable PIO control*/
      SET(AT91C_BASE_PIOB->PIO_PER  , (CFRESET_pin));
      /*enable pin as output*/
      SET(AT91C_BASE_PIOB->PIO_OER  , (CFRESET_pin));
      
   //set mode registers in Static Memory Controler
      //set slowest posible timings
      AT91C_BASE_SMC0->SMC_SETUP4= NCS_RD_SETUP | NRD_SETUP | NCS_WR_SETUP | NWE_SETUP;
      AT91C_BASE_SMC0->SMC_SETUP5= NCS_RD_SETUP | NRD_SETUP | NCS_WR_SETUP | NWE_SETUP;
      AT91C_BASE_SMC0->SMC_PULSE4= NCS_RD_PULSE | NRD_PULSE | NCS_WR_PULSE | NWE_PULSE;
      AT91C_BASE_SMC0->SMC_PULSE5= NCS_RD_PULSE | NRD_PULSE | NCS_WR_PULSE | NWE_PULSE;
      AT91C_BASE_SMC0->SMC_CYCLE4= NRD_CYCLE | NWE_CYCLE;
      AT91C_BASE_SMC0->SMC_CYCLE5= NRD_CYCLE | NWE_CYCLE;
      //MODE reister is called CTRL register
      AT91C_BASE_SMC0->SMC_CTRL4= READ_MODE  |
                           WRITE_MODE |
                           EXNW_MODE |
                           BAT |
                           DBW |
                           DTF_CYCLES |
                           DTF_MODE |
                           PMEN |
                           PS;
      AT91C_BASE_SMC0->SMC_CTRL5= READ_MODE  |
                           WRITE_MODE |
                           EXNW_MODE |
                           BAT |
                           DBW |
                           DTF_CYCLES |
                           DTF_MODE |
                           PMEN |
                           PS;
}