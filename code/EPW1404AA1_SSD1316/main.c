/* EPW1404AA1 (based on SSD1316) driver code on STM8L
 * Coded by TinLethax 2021/08/25 +7
 */
#include <stm8l.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <delay.h>
#include <i2c.h>

#include <font8x8_basic.h>

// Define Left hand mode for who want to wear this on left hand.
// 9.9 in 10 people wears watch on left hand. I'm the 0.1 who wear on right hand.
//#define LEFT_HANDED


// Define pin to use 
// RSTB
#define EPW_RSTB		4// PC4
// 10Volt boost enable pin of TPS63060 (Dev version) or LTC3459 (Watch version).
#define EPW_VBOOST		5// PC5

// Necessary Definitions of EPW1404AA1
#define EPW_FB_SIZE 	256 	// define the memory size used for Frame Buffer (128x2 bytes = 256 bytes).

#define EPW_FB_HALFSIZE 128 // define the memory size used for the Send buffer.

#define EPW_ADDR 		0x3C // 7bit i2c address already.

#define EPW_CMD_MODE 	0x80 // First byte to send before sending any command-related byte.

#define EPW_DAT_MODE 	0x40 // First byte to send before sending any Display data. 

#define EPW_Sleep		0xAE // command to turn display off (sleep).
#define EPW_Wake		0xAF // command to turn display on (wake from sleep).

/* init commands sequence*/

#define EPW_SETCONT		0x81 // command to set contrast.
#define EPW_CONT		0xFF // contras value that need to be set. Might tinkering with this later.

// DON'T CHANGE THE PARAMETER, THESE PARAMS ARE FIXED BY MANUFACTURER !
#define EPW_SETVCOM		0xAD // command to set VCOMH.
#define EPW_VCOM		0x10 // enable Iref during power on and use internal VCOM (according to SSD 1316 datasheet).

#define EPW_SETOSC		0xD5 // command to set Divide ratio and Oscillator Freq.
#define EPW_OSC			0xC2 // Divide ratio = 2+1, Oscillator is default value.

#define EPW_SETPCW		0xD9 // command to set Pre Charge Width.
#define EPW_PCW			0xF1 // Phase 1 is 1, Phase 2 is 15.

#define EPW_SETVCDE		0xDB // command to set VCOMH Deselect Level.
#define EPW_VCDE		0x30 // ~0.83 * Vcc.
// End manufacture stuffs 
 
#define EPW_SEGREMP_RH	0xA0 // command to set segment remap (Don't swap column 0 with 127 (and so on), We're using right hand mode).
#define EPW_SEGREMP_LH 	0xA1 // Do the reverse of 0xA0 (This will be used when define LEFT_HANDED.

#define EPW_SETMUXR		0xA8 // command to set Multiplex Ratio.
#define EPW_MUXR		0x0F // 16 - 1 Mux (16 com/row strating from 0 - 15).

#define EPW_SETDSOF		0xD3 // command to Display offset.
#define EPW_DSOF		0x1F // vertical shift by COM from 38.

#define EPW_ROWRV		0xC8 // command to reverse row, Using this with right handed.

#define EPW_SETPHC		0xDA // command to set Pin Hardware Configuration.
#define EPW_PHC			0x12 // Alternative SEG pin configuration and Disable SEG left/right remap.

#define EPW_SETPAM		0x20 // command to set memory addressing mode.
#define EPW_PAM			0x00 // Horizontal addressing mode (easier to send data).

/* end init sequence commands */

/* Graphic related stuffs */

// allocate mem for screen buffer
// 16 column bytes and 16 rows 
uint8_t FB0[EPW_FB_SIZE]={0};// standard horizontal byte format Frame buffer

// 16 column bytes and 8 rows (For updating each Memory Page (0:1)).
static uint8_t PG;// byte holding the vertical byte format for data sending.

//These Vars required for print function
static uint8_t YLine = 1;
static uint8_t Xcol = 1;

/* Low level stuffs */
// GPIO init and interrupt 
void GPIO_init(){
	
	// Init Port C for RSTB and Vboost .
	PC_DDR |= (1 << EPW_RSTB) | (1 << EPW_VBOOST);// Set PC4,5 as output
	PC_CR1 |= (1 << EPW_RSTB) | (1 << EPW_VBOOST);// with push pull mode.
}

// Entry: OLED_driver
//========================================================================================================================

// I2C send single CMD byte 
void EPW_sendCMD(uint8_t packet){
	i2c_start();// generate start condition.
		
	i2c_write_addr(EPW_ADDR);// call slave device for write.
		i2c_write(EPW_CMD_MODE);// Indicator that next byte is command
		i2c_write(packet);// write data to EPW1404AA1.
	
	i2c_stop();// generate stop condition.
	
}

// I2C send double CMD byte
void EPW_sendCMD2(uint8_t pac1, uint8_t pac2){
	EPW_sendCMD(pac1);// write CMD to EPW1404AA1.
	EPW_sendCMD(pac2);// write Param of above CMD to EPW1404AA1.
}

// Initialize the Display
void EPW_start(){
	PC_ODR &= ~(1 << EPW_RSTB);// reset the display
	delay_ms(1);
	PC_ODR |= (1 << EPW_RSTB);// set RSTB high.
	delay_us(10);// wait a little bit.
	PC_ODR |= (1 << EPW_VBOOST);// turn the TPS63060 on (EN pin default is pull high by resistor, remove that resistor).

	EPW_sendCMD(EPW_Sleep);// Turn Display off.
	
	EPW_sendCMD2(EPW_SETCONT, EPW_CONT);// set contrast. 
	EPW_sendCMD2(EPW_SETVCOM, EPW_VCOM);// select VCOMH/IREF.
	EPW_sendCMD2(EPW_SETOSC,	EPW_OSC);// set Clock Divide Raton and Oscillator Freq .
	EPW_sendCMD2(EPW_SETPCW, EPW_PCW);// set Precharge Width.
	EPW_sendCMD2(EPW_SETVCDE, EPW_VCDE);// set VCOMH Deselect Level.
	

	EPW_sendCMD2(EPW_SETMUXR, EPW_MUXR);// set Mux Ratio.
	EPW_sendCMD2(EPW_SETDSOF, EPW_DSOF);// set Display offset.
#ifndef LEFT_HANDED
	EPW_sendCMD(EPW_ROWRV);// Reverse row (segment) for right handed.
#endif 
	EPW_sendCMD2(EPW_SETPHC,  EPW_PHC);// set Pin Hardware configuration for setting segment order
	EPW_sendCMD2(EPW_SETPAM,	EPW_PAM);// set Memory addressing mode to Page address (2 pages, 8 bit width each).
#ifdef LEFT_HANDED
	EPW_sendCMD(EPW_SEGREMP_LH);// swap the column when put this watch on left hand
#else
	EPW_sendCMD(EPW_SEGREMP_RH);// Don't swap the column because we're using right handed mode.
#endif 

	
	// Turn display on.
	EPW_sendCMD(EPW_Wake);
}

// Turn Display on/off (for standby mode). 
void EPW_DispOn(bool on){
	if (on){
		PC_ODR |= (1 << EPW_VBOOST);// Turn on TPS63060 (10v boost IC).
		EPW_sendCMD(EPW_Wake);// Turn Display on.
	}else{
		EPW_sendCMD(EPW_Sleep);// Turn Display off.
		PC_ODR &= ~(1 << EPW_VBOOST);// Turn off TPS63060.
	}
}

// Update Display  
void EPW_Update(){
	
	EPW_sendCMD(0x21);// set column offset and end 
	EPW_sendCMD(0);// start at column 0
	EPW_sendCMD(127);// to column 127
	
	EPW_sendCMD(0x22);// set start and stop page
	EPW_sendCMD(0);// start at page 0
	EPW_sendCMD(1);// and end at page 1
	
	
	i2c_start();// generate start condition.
	
	i2c_write_addr(EPW_ADDR);// call slave device for write.
		i2c_write(EPW_DAT_MODE);// send 0x40 to indicate that this is Display data
		
	for (uint16_t FBOff = 0; FBOff < 256; FBOff ++){// 128 columns (for each verical byte PB[]).
			for (uint8_t i=0; i < 8; i++){// Single bit of FB0 every 16n byte is rotated to vertical byte 
				if(FB0[(16*i) + ( (FBOff%128) / 8) + ((FBOff / EPW_FB_HALFSIZE)*EPW_FB_HALFSIZE)] & (1 << (FBOff%8)))
					PG |= (1 << i);
				else
					PG &= ~(1 << i);
			}

			i2c_write(PG);// Write 1 vertical byte to display for 128 times.
	}

	i2c_stop();// generate stop condition.

}

/* Graphic stuffs */

// Load Graphic with same size as Frame Buffer into FB0
void EPW_LoadFull(uint8_t *BMP){
	memcpy(FB0, BMP, EPW_FB_SIZE);
}

// Clear all pixels of Frame Buffer 
void EPW_Clear(){
	Xcol = 1;
	YLine = 1;
	memset(FB0, 0, EPW_FB_SIZE);
}

// Fill entire Frame Buffer with 1
void EPW_Fill(){
	memset(FB0, 0xFF, EPW_FB_SIZE);
}

// Buffer update (with X,Y Coordinate and image WxH) X,Y Coordinate start at (1,1) to (50,240)
//
//NOTE THAT THE X COOR and WIDTH ARE BYTE NUMBER NOT PIXEL NUMBER (8 pixel = 1 byte). A.K.A IT'S BYTE ALIGNED
//
void EPW_LoadPart(uint8_t* BMP, uint8_t Xcord, uint8_t Ycord, uint8_t bmpW, uint8_t bmpH){

	Xcord--;
	Ycord--;
	//uint16_t XYoff,WHoff = 0;

	//Counting from Y origin point to bmpH using for loop
	for(uint8_t loop = 0; loop < bmpH; loop++){
		// turn X an Y into absolute offset number for Buffer
		//XYoff = (Ycord+loop) * 16;
		//XYoff += Xcord;// offset start at the left most, The count from left to right for Xcord times

		// turn W and H into absolute offset number for Bitmap image
		//WHoff = loop * bmpW;
		memcpy(FB0 + ((Ycord + loop) * 16) + Xcord, BMP + (loop * bmpW), bmpW);
	}

}


//Print 8x8 Text on screen
void EPW_Print(char *txtBuf){

//uint16_t chOff = 0;

while(*txtBuf){
	// In case of reached 16 chars or newline detected , Do the newline
	if ((Xcol > 16) || *txtBuf == 0x0A){
		Xcol = 1;// Move cursor to most left
		YLine += 8;// enter new line
		txtBuf++;// move to next char
	}

	// Avoid printing Newline
	if (*txtBuf != 0x0A){
	//chOff = (*txtBuf - 0x20) * 8;// calculate char offset (fist 8 pixel of character)
	EPW_LoadPart(font8x8_basic + ((*txtBuf - 0x20) * 8), Xcol, YLine, 1, 8);// Align the char with the 8n pixels

	txtBuf++;// move to next char
	Xcol++;// move cursor to next column
	}
	
  }
}

void main(){
	GPIO_init();// GPIO Init
	i2c_init(0x00, I2C_400K);// I2C at 400KHz
	
	// Clean Frame Buffer for Fresh init.
	EPW_Clear();
	
	//Start the OLED display
	EPW_start();


	while(1){
	EPW_Clear();
	EPW_Print("EPW1404AA1 OLED");
	EPW_Print("\nBy TinLethax");
	EPW_Update();
	delay_ms(1000);
	EPW_Clear();
	EPW_Fill();
	EPW_Update();
	delay_ms(1000);
	}// While loop
  
}// main 
