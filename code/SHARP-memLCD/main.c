/* USART RX terminal on LSLS027B7DH01 (400 x 240 px)
 * But due to insufficient of memory on my STM8L151F3, The heigth pixels is reduced to 48 pixels high, that cost 2.4 Kbyte of flash as a buffer (really bad idea)
 * starting from address 0x969F and will be filled up to address 0x9FFF (I want to stay away from the code area as far as possible). 
 * Coded by TinLethax 2021/05/23 +7
 */ 
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stm8l.h>
#include <usart.h>
#include <delay.h>
#include "font8x8_basic.h"
/* Connection Guide */
// MCU -> Display
// PB5 -> SCLK 	(1)
// PB6 -> SI	(2)
// PB4 -> SCS	(3)
// PB0 -> EXTCOM(4)
// PB3 -> DISP	(5)

// NOTE : The ExtComIn mode is External signal (Tie EXTMODE pin to VCC)

//Serial Interface 
#define SPI1_DR_ADDR 0x5204
#define SCK	5 // PB5 
#define SDO	6 // PB6
#define SCS	4 // PB4
#define DSP	3 // PB3

// PWM pin 
#define EXTCOM     0 // PB0 


//Display buffer Setup
#define DCol	400 // 400px width 
#define DRow	48 // 48px height
#define linebuf DCol/8
#define FB_Size	linebuf*DRow

#define BUFOFF 0x969F

/* [W.I.P] */
//Using Flash space from main program area, offset at 0x969F (using 37.5 pages, 64 bytes each, total 2400 bytes)
//__at(BUFOFF) uint8_t DispBuf[FB_Size];// We store our data in the Flash memory (the entire screen framebuffer).
/* 1 byte contain 8 pixels data, using */
/* the way that the packet is sent and how to grab the right FB data is the same as my STM32 project (on my GitHub). */

//This buffer holds 1 Character bitmap image (8x8)
uint8_t chBuf[8];

//These Vars required for print function
uint8_t YLine = 1;
uint8_t Xcol = 1;

//Serial buffer array
uint8_t SendBuf[linebuf+4];

#define HWSPI

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

	SendBuf[linebuf+2]= 0;	
	SendBuf[linebuf+3]= 0;

	CLK_PCKENR1 |= (1 << 0);// Enable Timer2 clock system
#ifdef HWSPI
	CLK_PCKENR1 |= (1 << 4);// Enable SPI1 clock system
#endif
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

#ifdef HWSPI	
	// SPI setup 
	SPI1_CR1 |= (1 << 7) | (3 << 3);// LSB First, 1MHz SPI speed.
	SPI1_CR2 |= (1 << 7) | (1 << 6) | 0x03;// Transmit Only in 1 line mode, soft NSS enabled
	SPI1_CR1 |= (1 << 2);// SPI Master Mode 
	SPI1_CRCPR = 0x00;// No CRC 

	SPI1_CR1 |= (1 << 6);// Enable SPI
#endif
}

void LCD_unlock(){// We will leave the Flash unlocked, so we don't need two times reset to re-write the framebuffer to flash again
	uint8_t *fb;
	fb = (uint8_t *)BUFOFF;
	// Flash unlock (Program region not EEPROM (data) region)
	FLASH_PUKR = FLASH_PUKR_KEY1;// 0x56
	FLASH_PUKR = FLASH_PUKR_KEY2;// 0xAE
	while(!(FLASH_IAPSR & (1 << FLASH_IAPSR_PUL)));// wait until Flash in unlocked
	
	for(uint16_t i=0; i < FB_Size; i++){
		*fb++ = 0xFF;// Fill FB with white Pixels 
		while (!(FLASH_IAPSR & (1 << FLASH_IAPSR_EOP)));// Wait until data is written to Flash	
	}
}

void LCD_flashWrite(uint8_t *Src, uint16_t offset, size_t len){// It's memcpy-likes but Flash friendly
	uint8_t *fb;
	fb = (uint8_t *)BUFOFF + offset;

	while(len--){
		*fb++ = *Src++;
		delay_us(2);
	}
}

void LCD_sendByte(uint8_t *dat, size_t len){// SPI bit banging (will use SPI+DMA later)
#ifdef HWSPI
	while(len--){
		SPI1_DR = *dat++;
		while (!(SPI1_SR & (1 << SPI1_SR_TXE)));
	}
	while (SPI1_SR & (1 << 7));// wait until we are not busy 
#else	
	while(len--){
		for(uint8_t j=8; j == 0;j--){// (LSB first)
		PB_ODR |= (1 << SCK); // _/
		PB_ODR = (((uint8_t *)dat << (j-1)) << SDO); // 1/0 (LSB first)
		PB_ODR &= (0 << SCK);// \_
		}
	dat++;
	}
#endif	
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
	fb = (uint8_t *)BUFOFF;
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

void LCD_ScreenFill(){// Fill entire screen with black/reflective pixels
	for(uint8_t i=0;i < linebuf;i++){
	SendBuf[i+2] = 0x00;//set all pixel to 0 (turn pixel to black/reflective)
	}

	for(uint8_t i=1;i < DRow+1;i++)
		LCD_lineUpdate(i);
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

void LCD_loadPart(uint8_t* BMP, uint8_t Xcord, uint8_t Ycord, uint8_t bmpW, uint8_t bmpH){

	Xcord = Xcord - 1;
	Ycord = Ycord - 1;
	uint16_t XYoff,WHoff = 0;

	//Counting from Y origin point to bmpH using for loop
	for(uint8_t loop = 0; loop < bmpH; loop++){
		// turn X an Y into absolute offset number for Buffer
		XYoff = (Ycord+loop) * linebuf;
		XYoff += Xcord;// offset start at the left most, The count from left to right for Xcord times

		// turn W and H into absolute offset number for Bitmap image
		WHoff = loop * bmpW;

		LCD_flashWrite(BMP + WHoff, XYoff, bmpW);
	}

}

//Print 8x8 Text on screen
void LCD_Print(unsigned char *txtBuf){
uint16_t chOff = 0;
while (*txtBuf){
	// In case of reached 50 chars or newline detected , Do the newline. Two characters are separated by 1 pixel 
	if ((Xcol > 50) || *txtBuf == 0x0A){ 
		txtBuf++;// move to next char
		Xcol = 1;// Move cursor to most left
		YLine += 8;// enter new line
	}

	// Avoid printing Newline or screen clear
	if ((*txtBuf != 0x0A)){

	chOff = (*txtBuf - 0x20) * 8;// calculate char offset on font array(fist 8 pixel of character)

	for(uint8_t i=0;i < 8;i++){// Copy the inverted color px to buffer
	chBuf[i] = ~font8x8_basic[i + chOff];
	}

	LCD_loadPart(chBuf, Xcol, YLine, 1, 8);// Align the char with the 8n pixels

	txtBuf++;// move to next char
	Xcol++;// move cursor to next column
	}

 }
	LCD_rangeUpdate(1, 48);// Update only 8n Lines (Heigth of pixels plus enter newline)

}


void main() {
	CLK_CKDIVR = 0x00;// Get full 16MHz clock 
	LCD_GPIOsetup();// GPIO setup
	LCD_periphSetup();// Peripheral Setup
	LCD_ScreenClear();// Clear garbage in Display buffer mem.
	LCD_unlock();// Unlock Flash Once  

	LCD_ScreenFill();
	delay_ms(500);	
	LCD_ScreenClear();
	LCD_Print("TinLethax!\n");
	delay_ms(250);
	LCD_Print("1234567890\n");
	delay_ms(250);
	LCD_Print("NotSafeForCode\n");
	while (1) {

	}
}
