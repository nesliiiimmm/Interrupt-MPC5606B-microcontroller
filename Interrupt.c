#include "MPC5606B.h"
#include "INTCInterrupts.h"
void initModesAndClks(void);
void initPeriClkGen(void);
void disableWatchdog(void);
static void PIT3_isr(void);
static void FMPLL_init(void);
static void PIT3_init(void);
//uint16_t Result; 

#define PIT_interval_1s

void initModesAndClks(void) {//mode clock ayarlarý..
	ME.MER.R = 0x0000001D; /* Enable DRUN, RUN0, SAFE, RESET modes */
	/* Initialize PLL before turning it on: */
	/* Use 1 of the next 2 lines depending on crystal frequency: */
	CGM.FMPLL_CR.R = 0x02400100; /* 8 MHz xtal: Set PLL0 to 64 MHz */
	/*CGM.FMPLL[0].CR.R = 0x12400100;*//* 40 MHz xtal: Set PLL0 to 64 MHz */
	ME.RUN[0].R = 0x001F0074; /* RUN0 cfg: 16MHzIRCON,OSC0ON,PLL0ON,syclk=PLL */
	ME.RUNPC[0].R = 0x00000010; /* Peri. Cfg. 0 settings: only run in RUN0 mode */
	/* Use the next lines as needed for MPC56xxB/S: */
	//ME.PCTL[32].R=0x0000;
	ME.PCTL[48].R = 0x0000; /* MPC56xxB LINFlex0: select ME.RUNPC[0] */
	ME.PCTL[68].R = 0x0000; /* MPC56xxB/S SIUL:  select ME.RUNPC[0] */
	ME.PCTL[32].R = 0x0000;/* MPC56xxB ADC_0 */ //neden bilmiyorum..
	/* Mode Transition to enter RUN0 mode: */
	ME.MCTL.R = 0x40005AF0; /* Enter RUN0 Mode & Key */
	ME.MCTL.R = 0x4000A50F; /* Enter RUN0 Mode & Inverted Key */
	while (ME.IS.B.I_MTC != 1) {
	} /* Wait for mode transition to complete */
	ME.IS.R = 0x00000001; /* Clear Transition flag */
}

void initPeriClkGen(void) {//peripheral clocklar...
	/* Use the following code as required for MPC56xxB or MPC56xxS:*/
	//CGM.SC_DC[0].R = 0x80;   /* MPC56xxB/S: Enable peri set 1 sysclk divided by 1 */
	CGM.SC_DC0.R = 0x80; /* MPC56xxB/S: Enable peri set 3 sysclk divided by 1 */
	CGM.SC_DC1.R = 0x80; /* MPC56xxB/S: Enable peri set 3 sysclk divided by 1 */
	CGM.SC_DC2.R = 0x80; /* MPC56xxB/S: Enable peri set 3 sysclk divided by 1 */
}

void disableWatchdog(void) {
	SWT.SR.R = 0x0000c520; /* Write keys to clear soft lock bit */
	SWT.SR.R = 0x0000d928;
	SWT.CR.R = 0x8000010A; /* Clear watchdog enable (WEN) */
}
static void PIT3_isr(void) {//bu yapýlacak olan iþlem interrupt geldiðinde
	/* toggle LED */
	SIU.GPDO[68].R ^= 1;

	PIT.CH[3].TFLG.R = 0x00000001;
}
static void FMPLL_init(void) {

#if defined(CORE_CLOCK_64MHz)

	/* Enter RUN0 with PLL as sys clk (64 MHz) with 8 MHz crystal reference. */
	ME.MER.R = 0x0000001D; /* Enable DRUN, RUN0, SAFE, RESET modes */
	CGM.FMPLL_CR.R = 0x01200100; /* 8MHz xtal: Set PLL0 to 64 MHz */
	ME.RUNPC[0].R = 0x000000FE; /* enable peripherals run in all modes */
	ME.RUN[0].R = 0x001F0074; /* RUN0 cfg: IRCON,OSC0ON,PLL0ON,syclk=PLL */

	/* Mode Transition to enter RUN0 mode: */ME.MCTL.R = 0x40005AF0; /* Enter RUN0 Mode & Key */
	ME.MCTL.R = 0x4000A50F; /* Enter RUN0 Mode & Inverted Key */
	while (ME.GS.B.S_MTRANS) {
	}; /* Wait for mode transition to complete */
	while (ME.GS.B.S_CURRENTMODE != 4) {
	}; /* Verify RUN0 is the current mode */

	printf("fsys = 64MHz\n\r");

#elif defined(CORE_CLOCK_48MHz)

	/* Enter RUN0 with PLL as sys clk (48 MHz) with 8 MHz crystal reference. */
	ME.MER.R = 0x0000001D; /* Enable DRUN, RUN0, SAFE, RESET modes */
	CGM.FMPLL_CR.R = 0x02300100; /* 8MHz xtal: Set PLL0 to 48 MHz */
	ME.RUNPC[0].R = 0x000000FE; /* enable peripherals run in all modes */
	ME.RUN[0].R = 0x001F0074; /* RUN0 cfg: IRCON,OSC0ON,PLL0ON,syclk=PLL */

	/* Mode Transition to enter RUN0 mode: */
	ME.MCTL.R = 0x40005AF0; /* Enter RUN0 Mode & Key */
	ME.MCTL.R = 0x4000A50F; /* Enter RUN0 Mode & Inverted Key */
	while (ME.GS.B.S_MTRANS) {}; /* Wait for mode transition to complete */
	while (ME.GS.B.S_CURRENTMODE != 4) {}; /* Verify RUN0 is the current mode */

	printf("fsys = 48MHz\n\r");

#endif
}
static void PIT3_init(void) {
	/* define 1 second interval based on frequency */
#if defined(CORE_CLOCK_64MHz)
#define PIT_interval_1s 64000000
#elif defined(CORE_CLOCK_48MHz)
#define PIT_interval_1s 48000000
#endif

	/* 30: MDIS = 0 to enable clock for PITs. */
	/* 31: FRZ = 1 for Timers stopped in debug mode */
	PIT.PITMCR.R = 0x00000001;
	PIT.CH[3].LDVAL.R = PIT_interval_1s - 1;

	/* clear the TIF flag */PIT.CH[3].TFLG.R = 0x00000001;

	/* 30: TIE = 1 for interrupt request enabled */
	/* 31: TEN = 1 for timer active */
	PIT.CH[3].TCTRL.R = 0x00000003;
}

int main(void) {
	volatile int i = 0;
	initModesAndClks();//
	initPeriClkGen();
	disableWatchdog();
	FMPLL_init();
	INTC_InstallINTCInterruptHandler(PIT3_isr, 127, 3);//burda en baþta girmemiz lazým çünkü interrupt geldiðinde ne yapacaðýný söylüyoruzz...
	SIU.PCR[68].B.PA = 0;
	SIU.PCR[68].B.OBE = 1;
	INTC.CPR.R = 0;
	PIT3_init();
	INTC_InitINTCInterrupts();
	for (;;) {
		//buradada birþey iþlem yaptýrmamýz gerekiyor ama çok önemli deðil bence..
		i++;
	}

}