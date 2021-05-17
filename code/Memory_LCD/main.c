/* Project "Spy Morror". Based on SHARP LS013B4DN04 96x96 pixel PNLC memory LCD
 * Coded by TinLethax 2021/05/15 +7
 */ 
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stm8l.h>

//Serial Interface 
#define SPI1_DR_ADDR 0x5204
#define SCK	5
#define SDO	6
#define SCS	4
#define DSP	3

// PWM pin 
#define EXTCOM     0 //PB0 

//INT pin for waking up MCU
#define BTN	   0 //PD0

//Display buffer Setup
#define DCol	96 // 96px width 
#define DRow	96 // 96px height
#define linebuf DCol/8
#define FB_Size	linebuf*DRow

/* [W.I.P] */
//Using Flash space from UBC area, offset at page 110 (using 18 pages, 64 bytes each)
__at(0x9B7F) uint8_t DispBuf[FB_Size];// We store our data in the Flash memory (the entire screen framebuffer).
/* Page - > [Row n data][Row n+1 data][Row n+2 data][Row n+3 data] (16Bytes x4)
 *           ^      1st byte on page                     2nd byte on page                           4th byte on page
 *     [CMD][ROW][12 Bytes Display Data][Dummy x2]-[Dummy][ROW][12 Bytes Display Data][Dummy x2]...[Dummy x2]      
 */

//Serial buffer array
static uint8_t SendBuf[linebuf+4]={0};
#define HWSPI

//interrupt stuffs
bool CloakState = false; //CloakState True -> Normal Mirror, CloakState false -> Hello again, Dumbbell (you get it if you play TF2).

void SM_GPIOsetup(){// GPIO Setup
	// Serial interface pins
	PB_DDR |= (1 << SCK) | (1 << SDO) | (1 << SCS) | (1 << DSP);// All pins as output
	PB_CR1 |= (1 << SCK) | (1 << SDO) | (1 << SCS) | (1 << DSP);// with PUsh-Pull mode
	PB_CR2 |= (1 << SCK) | (1 << SDO);// But some (MOSI and SCK) need HIGH SPEEEEEEEEDDDDD.

	PB_ODR &= (0 << SCS);// for some waird reason, this DIsplay love active high chip select....

	// External COM signal pin [PWM]
	PB_DDR |= (1 << EXTCOM);// This is the PWM output pin for EXTCOM inverting signal
	PB_CR1 |= (1 << EXTCOM);// Push-Pull mode
	PB_CR2 |= (1 << EXTCOM);// fast mode

	// Wake Up Interrupt pin
	PD_DDR |= (0 << BTN);// init pin PD5 as input
	PD_CR1 |= (1 << BTN);// Input pullup
	PD_CR2 |= (1 << BTN);// interrupt
}	

void SM_periphSetup(){// Peripheral Setup. PWM, Timer and Interrupt
	// Peripheral Clock setup
	CLK_ICKCR=(1 << CLK_ICKCR_LSION);// turn LowSpeedInternal Clock
	while (!(CLK_ICKCR & (1 << CLK_ICKCR_LSIRDY)));  // enable LSI oscillator.  

	CLK_CRTCR = 0x04; //set the RTC clock to LSI clock and set the divider at 1 
	CLK_PCKENR1 |= (1 << 0) | (1 << 4);// Enable Timer2 and SPI1 clock system
	CLK_PCKENR2 |= (1 << 2) | (1 << 4);// Enable RTC and DMA clock system
	// Timer PWM setup
	
	//Timer 2 init for PWM with ARR = 2500 (50Hz)
	TIM2_ARRH = 0x09;
	TIM2_ARRL = 0xC4;

	// We got the CK_CNT or counting clock from our CPU clock at 16MHz divided by 2^0x07 (128,It's prescaler), 
	// So 16e^6/2^7 = 1.25e^5 (125KHz)
	TIM2_PSCR = (uint8_t)0x07; 

	TIM2_EGR = 0x02;

	uint8_t tmpccmr1 = TIM2_CCMR1;
	tmpccmr1 &= ~(0x70);
	tmpccmr1 |= 0x60;
	TIM2_CCMR1 = tmpccmr1;
	//I use PB0 as TIM2 channel 1 output active high 
	TIM2_CCER1 |= 0x01; // Set output polarity to active low and enable output
	//TIM2_CCMR1 = (uint8_t)(104); //Set pwm1 mode and enable preload for CCR1H/L
	TIM2_OISR |= 0x01;
	
	// and PWM at 50% duty cycle 
	TIM2_CCR1H =  0x00;
	TIM2_CCR1L =  0xFF;
	// Enable TIM2 peripheral
	TIM2_BKR |= 0x80;

	//Enable TIM2
	TIM2_CR1 = 0x01;
	//===============================================

	// Interrupt Wake-Up setup

	ITC_SPR2 |= 0x40;// Enable Port D interrupt

	EXTI_CR2 |=  0x02;// PD0 detecting Falliing-Edge only
	//===============================================
	
	// SPI setup 
	SPI1_CR1 |= (1 << 7) | (3 << 3);// LSB First, 1MHz SPI speed.
	SPI1_CR2 |= (1 << 7) | (1 << 6) | 0x03;// Transmit Only in 1 line mode, soft NSS enabled
	SPI1_CR1 |= (1 << 2);// SPI Master Mode 
	SPI1_CRCPR = 0x00;// No CRC 
	//===============================================

	// DMA setup
	DMA1_GCSR = (uint8_t)(0x3F << 2);// set DMA timeout
	
	DMA1_C2CR &= (0 << 0);// Temporaly disable DMA channel 2 
	DMA1_C2CR |= (1 << 5) | (1 << 3);// DMA Memory incremented up, DMA Memory to Peripheral mode 
	DMA1_C2SPR |= (1 << 5);// Priority High (Also using 8-bit Transfer size)
	DMA1_C2NDTR = 16;// Size of buf to be transfered, in this case is 16 bytes  

	DMA1_C2PARH = (uint8_t)(SPI1_DR_ADDR >> 8);// Higher byte of SPI data register mem location
	DMA1_C2PARL = (uint8_t)(SPI1_DR_ADDR);// Lower byte of SPI data register mem location
	DMA1_C2M0ARH = (uint8_t)((uint16_t)SendBuf >> 8);// Higher byte of Tx mem location
	DMA1_C2M0ARL = (uint8_t)((uint16_t)SendBuf);// Lower byte of Tx mem location

	SPI1_ICR |= (1 << 6);// Enable Tx DMA  

	DMA1_C2CR |= (1 << 0);// Enable DMA channel 2

	SPI1_CR1 |= (1 << 6);// Enable SPI
	//===============================================

	// RTC Setup

	// unlock the writing protection
	RTC_WPR = 0xCA;
	RTC_WPR = 0x53;

	if(RTC_ISR1 & 0x40)// Init-ed ?
		RTC_ISR1 |= (0x80);// If not, Let wake him up !
	while (RTC_ISR1 & 0x40);// He might take a while 

	RTC_CR1 &= (0 << 6);// 24 Hour format
	RTC_CR1 |= (1 << 4);// Using direct R/W instead of Shadow memory 

	// set the Prescalers regs
	RTC_SPRERH = 0x00;
	RTC_SPRERL = 0xFF;
	RTC_APRER  = 0x7F;
	// exit init mode
	RTC_ISR1 = (0 << 7);

	// lock write protection
	RTC_WPR = 0xFF; 
}

void SM_malloc(){//We will use very last pages on our Flash memory from page number 110 to 127
	// Flash set programming time
	FLASH_CR1 |= 0x10; // fast programming mode 
	
	// Flash unlock (Program region not EEPROM (data) region)
	FLASH_PUKR = FLASH_PUKR_KEY1;
	FLASH_PUKR = FLASH_PUKR_KEY2;

	while(!(FLASH_IAPSR & (1 << FLASH_IAPSR_PUL)));// wait until Flash in unlocked

	for(uint16_t i=0;i < FB_Size;i++){// Fill Framebuffer with 0
		DispBuf[i] = 0;
		while (!(FLASH_IAPSR & (1 << FLASH_IAPSR_EOP)));
	}
		
	FLASH_IAPSR &= 0xFD;// Re-lock flash (Program region) after write
}

void SM_sendByte(uint8_t *dat){// SPI bit banging (will use SPI+DMA later)
#ifdef HWSPI
	SPI1_DR = (uintptr_t)dat;
	while (!(SPI1_SR & (1 << SPI1_SR_TXE)));
#else	
	for(uint8_t j=8; j == 0;j--){// (LSB first)
	PB_ODR |= (1 << SCK); // _/
	PB_ODR = (((uintptr_t)dat << (j-1)) << SDO); // 1/0 (LSB first)
	PB_ODR &= (0 << SCK);// \_
	}
#endif	
}

void SM_lineUpdate(uint8_t line){// Single Row display update
	PB_ODR |= (1 << SCS);// Start transmission 

	SendBuf[0] = 0x01;// Display update Command
	SendBuf[1] = line;// Row number to update

	//Packet structure looks like this : command,row number, display data, dummy byte x2
	//[CMD][ROW][n bytes for 1 row][0x00][0x00] 
	for(uint8_t i=0; i < linebuf+4;i++){
	SM_sendByte(SendBuf+i);
	}

	PB_ODR &= (0 << SCS);// End tranmission
}

void SM_ScreenFill(){// Fill entire screen with black/reflective pixels
	for(uint8_t i=0;i < linebuf;i++){
	SendBuf[i+2] = 0xFF;//set all pixel to 1 (turn pixel to black/reflective)
	}

	for(uint8_t i=0;i < DRow;i++)
		SM_lineUpdate(i);
}

void SM_ScreenClear(){// Tiddy-Up the entire screen
	PB_ODR |= (1 << SCS);// Start tranmission

	SendBuf[0] = 0x04;// Display clear command
	SendBuf[1] = 0x00;// Dummy byte
	
	//Packet structure looks like this : command, dummy byte
	//[CMD][0x00] 
	for(uint8_t i=0; i < 2;i++){
	SM_sendByte(SendBuf+i);
	}

	PB_ODR &= (0 << SCS);// End tranmission	
}

void SM_ScreenFBU(){// Update entire Display with Framebuffer mem
	//Re configure DMA

	//Start DMA
	while(!(SPI1_SR & 0x02));// wait until SPI is tranmisted 
	SPI1_ICR |= (1 << 1);// DMA tranmist request 
}

void SwitchTF(void) __interrupt(7){// Interrupt Vector Number 7 (take a look at Datasheet, Deffinitely difference)
	CloakState = !CloakState;
}

void main() {
	CLK_CKDIVR = 0x00;// Get full 16MHz clock 
	SM_GPIOsetup();// GPIO setup
	SM_periphSetup();// Peripheral Setup
	SM_malloc();// Allocate flash memory for Framebuffer mem (we have very limited RAM, 1K isn't enought).
	//After power on, Fill the display with black (reflective) pixel (Active HIGH, Set 1 to turn black).
	//Disguise as a mirror.
	SM_ScreenFill();
	CloakState = true;
	__asm__("rim");// enable interrupt
	//after Cloaking done, We put CPU to Sleep
	__asm__("wfi");// wait for interrupt A.K.A CPU sleep zZzZ
	while (1) {
		if(CloakState){
		//Disguise as a Mirror
		SM_ScreenFill();
		}else{
		//Do Spy Stuff
		SM_ScreenClear();
		/* [W.I.P] 
		Possible feature here 
		- RTC clock and calendar
		- secret messages (bitmap font implementation)
		- Bitmap icons 
		- U(S)ART RX console
		*/
		while(!CloakState){


		}

		}
	__asm__("wfi");// Back to sleep 
	}
}
