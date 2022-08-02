/* T6A04A interface converter into SPI for driving LS013B4DN04 96x96 (only use 96x64) PLCC memory display.
 * working as a TI-82 calculator display replacement.
 * Coded by TinLethax 2021/09/21 +7
 */ 
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stm8l.h>
#include <usart.h>
#include <delay.h>

/* Connection Guide */
// MCU -> Display
// PB5 -> SCLK 	(1)
// PB6 -> SI	(2)
// PB4 -> SCS	(3)
// PB0 -> EXTCOM(4)
// PB3 -> DISP	(5)

// NOTE : The ExtComIn mode is External signal (Tie EXTMODE pin to VCC)

//Serial Interface 
#define SCK	5 // PB5 
#define SDO	6 // PB6
#define SCS	4 // PB4
#define DSP	3 // PB3

// PWM pin 
#define EXTCOM     0 // PB0 

//Display buffer Setup
#define DCol	96 // 400px width 
#define DRow	64 // 48px height
#define linebuf DCol/8

uint8_t DispBuf[linebuf];// 1 line framebuffer.
/* 1 byte contain 8 pixels data, using */
/* the way that the packet is sent and how to grab the right FB data is the same as my STM32 project (on my GitHub). */

//Serial buffer array
static uint8_t SendBuf[linebuf+4]={0};


void LCD_GPIOsetup(){// GPIO Setup
	// Serial interface pins
	PB_DDR |= (1 << SCK) | (1 << SDO) | (1 << SCS) | (1 << DSP);// All pins as output
	PB_CR1 |= (1 << SCK) | (1 << SDO) | (1 << SCS) | (1 << DSP);// with PUsh-Pull mode
	PB_CR2 |= (1 << SCK) | (1 << SDO);// But some (MOSI and SCK) need HIGH SPEEEEEEEEDDDDD.

	PB_ODR &= (0 << SCS);// for some weird reason, this Display love active high chip select....


	// External COM signal pin [PWM]
	PB_DDR |= (1 << EXTCOM);// This is the PWM output pin for EXTCOM inverting signal
	PB_CR1 |= (1 << EXTCOM);// Push-Pull mode
	PB_CR2 |= (1 << EXTCOM);// fast mode
}	

void LCD_periphSetup(){// Peripheral Setup. PWM and SPI

	CLK_PCKENR1 |= (1 << 0);// Enable Timer2 clock system
	CLK_PCKENR1 |= (1 << 4);// Enable SPI1 clock system
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

	// SPI setup 
	SPI1_CR1 |= 0x80 | (3 << 3);// LSB first, 1MHz SPI speed.
	SPI1_CR2 |= 0x03;// soft NSS enabled and enable master mode of slave select.
	SPI1_CR1 |= (1 << 2);// SPI Master Mode 
	SPI1_CRCPR = 0x00;// No CRC 

	SPI1_CR1 |= (1 << 6);// Enable SPI

}

void LCD_sendByte(uint8_t *dat, size_t len){// SPI bit banging (will use SPI+DMA later)
	while(len--){
		SPI1_DR = *dat++;
		while (!(SPI1_SR & (1 << SPI1_SR_TXE)));
	}
	while (SPI1_SR & (1 << 7));// wait until we are not busy 
}

void LCD_lineUpdate(uint8_t line){// Single Row display update
	PB_ODR |= (1 << SCS);// Start transmission 

	SendBuf[0] = 0x01;// Display update Command
	SendBuf[1] = line;// Row number to update

	//Packet structure looks like this : command,row number, display data, dummy byte x2
	//[CMD][ROW][n bytes for 1 row][0x00][0x00] 
	LCD_sendByte(SendBuf, sizeof(SendBuf));

	PB_ODR &= (0 << SCS);// End transmission
}

void LCD_rangeUpdate(uint8_t Start, uint8_t End){// Multiple Row update from Start to End 
	uint16_t offset = 0;
	uint8_t *fb;
	fb = (uint16_t *)BUFOFF;
	SendBuf[0] = 0x01;// Display update Command
	SendBuf[1] = Start;

	PB_ODR |= (1 << SCS);// Start transmission

	for(;End >= Start; End--){
		offset = (Start-1) * linebuf;
		LCD_sendByte(SendBuf,2);// Sending only two first bytes (command and row number)
		LCD_sendByte(fb + offset,50);// then send out the 12 bytes (96 px)
		SendBuf[1] = Start++;
	}
		LCD_sendByte(SendBuf + linebuf + 2,2);// Send two dummy bytes to mark as end of transmission
	PB_ODR &= (0 << SCS);// End transmission
}

void LCD_ScreenClear(){// Tiddy-Up the entire screen
	// Reset Cursor
	YLine = 1;
	Xcol = 1;

	SendBuf[0] = 0x04;// Display clear command
	SendBuf[1] = 0x00;// Dummy byte
	
	PB_ODR |= (1 << SCS);// Start tranmission
	//Packet structure looks like this : command, dummy byte
	//[CMD][0x00] 
	LCD_sendByte(SendBuf, 2);
	
	PB_ODR &= (0 << SCS);// End tranmission	
}




void main() {
	CLK_CKDIVR = 0x00;// Get full 16MHz clock 
	LCD_GPIOsetup();// GPIO setup
	LCD_periphSetup();// Peripheral Setup
	LCD_ScreenClear();// Clear garbage in Display buffer mem.

	while (1) {
	
	}
}
