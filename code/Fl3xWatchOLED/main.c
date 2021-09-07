/* Fl3xWatchOLED project using Futaba Flexible OLED display EPW14040AA1 (SSD1316) running wiht STM8L151F3.
 * Coded by TinLethax 2021/09/07 +7
 */
#include <stm8l.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <delay.h>
#include <i2c.h>

#include <liteRTC.h>

#include <font8x8_basic.h>

// Define pin to use 
// RSTB
#define EPW_RSTB		4// PC4
// 10Volt boost enable pin of TPS63060 (Dev version) or LTC3459 (Watch version).
#define EPW_VBOOST		5// PC5

// Definitions of EPW14040AA1
#define EPW_FB_SIZE 	256 	// define the memory size used for Frame Buffer (128x2 bytes = 256 bytes).

#define EPW_FB_HALFSIZE 128 // define the memory size used for the Send buffer.

#define EPW_ADDR 		0x78 // 7bit address already.

#define EPW_CMD_MODE 	0x80 // First byte to send before sending any command-related byte.

#define EPW_DAT_MODE 	0x40 // First byte to send before sending any Display data. 

#define EPW_Sleep		0xAE // command to turn display off (sleep).
#define EPW_Wake		0xAF // command to turn display on (wake from sleep).

/* init sequence commands */
// DON'T CHANGE THE PARAMETER, THESE PARAMS ARE FIXED BY MANUFACTURER !

#define EPW_SETCONT		0x81 // command to set contrast.
#define EPW_CONT		0x48 // contras value that need to be set.

#define EPW_SETVCOM		0xAD // command to set VCOMH.
#define EPW_VCOM		0x10 // enable Iref during power on and use internal VCOM (according to SSD 1316 datasheet).

#define EPW_SETOSC		0xD5 // command to set Divide ratio and Oscillator Freq.
#define EPW_OSC			0xC2 // Divide ratio = 2+1, Oscillator is default value.

#define EPW_SETPCW		0xD9 // command to set Pre Charge Width.
#define EPW_PCW			0xF1 // Phase 1 is 1, Phase 2 is 15.

#define EPW_SETVCDE		0xDB // command to set VCOMH Deselect Level.
#define EPW_VCDE		0x30 // ~0.83 * Vcc.

#define EPW_SETMUXR		0xA8 // command to set Multiplex Ratio.
#define EPW_MUXR		0x0F // 23 + 1 Mux (?).

#define EPW_SETDSOF		0xD3 // command to Display offset.
#define EPW_DSOF		0x1F // vertical shift by COM from 38.

#define EPW_SETPHC		0xDA // command to se t Pin Hardware Configuration.
#define EPW_PHC			0x12 // Alternative SEG pin configuration and Disable SEG left/right remap.

#define EPW_SETPAM		0x20 // command to set memory addressing mode.
#define EPW_PAM			0x02 // Page Addressing Mode (default)
/* end init sequence commands */

#define EPW_SEGREMP		0xA0 // command to set segment remap (default). 

#define EPW_PGSEL_0		0xB0 // select Page 0 (pixel row 0 to 7).

#define EPW_PGSEL_1		0xB1 // select Page 1 (pixel row 8 to 15).


#define SCREEN_TIMEOUT	5*2 // screen timeout 5 secs. since we use .5s counter (1 second counter is counting up 2 times).

// allocate mem for screen buffer 

// 16 column bytes and 16 rows 
uint8_t FB0[EPW_FB_SIZE]={0};// standard horizontal byte format Frame buffer

// 16 column bytes and 8 rows (For updating each Memory Page (0:1)).
static uint8_t PG[EPW_FB_HALFSIZE];// single page vertical byte format for data sending.

//These Vars required for print function
static uint8_t YLine = 1;
static uint8_t Xcol = 1;

// store PB_IDR to this variable to further detect which button is pushed/pressed. 
volatile uint8_t readPin = 0;
// press center button to enter time update mode.
volatile uint8_t TimeUpdate_lock = 0;
// keeping track of menu (button press cycle).
uint8_t menuTrack = 0;
#define MAX_MENU	2 // maximum menu pages

uint8_t countdown = 0;// screen timout keeper.

// Time keeping, read from RTC shadow registries.
uint8_t hour, minute, second;
uint8_t num2Char[8]={0,0,':',0,0,':',0,0};// store the time in char format "HH:MM:SS"

// DMY keeping, read from RTC shadow registries.
uint8_t Day, Date, Month, Year;
// convert day of week number to day name.
const static uint8_t DayNames[7]={"SUN", "MON", "TUE", "WEN", "THU", "FRI", "SAT"};
uint8_t num2DMY[10]={0,0, 0x2f, 0,0, 0x2f, '2','0', 0, 0};// store the date, month and year in cahr format "DD/MM/20YY"

/* Low level stuffs */
// GPIO init and interrupt 
void GPIO_init(){
	
	// Init Port C for RSTB and Vboost .
	PC_DDR |= (1 << EPW_RSTB) | (1 << EPW_VBOOST);// Set PC4,5 as output
	PC_CR1 |= (1 << EPW_RSTB) | (1 << EPW_VBOOST);// with push pull mode.
	
	// Init port B0 to B4 as a input with PortB interrupt.
	PB_DDR &= ~0x1F;// PB0-PB4 as input
	PB_CR1 = 0x1F;// With pull up resistor.
	PB_CR2 = 0x1F;// and Interrupt
	
	// Enable Port B interrupt.
	ITC_SPR2 |= (3 << 4);// set VECT6SPR (Entire portB interrupt) priority to high.
	//EXTI_CR1 = 0x00;// set interurpt to detect falling and low (Port0-3)
	//EXTI_CR2 &= ~0x03;// set interrupt to detect falling and low (Port4).
	EXTI_CR3 &= ~0x03;// set interruot to detect falling and low (PortB).


	EXTI_CONF1 |= 0x03;// Map interupt to PortB instead of individual pin.
}

// Entry: OLED_driver
//========================================================================================================================

// I2C send single CMD byte 
void EPW_sendCMD(uint8_t packet){
	i2c_start();// generate start condition.
	
	i2c_write_addr(EPW_ADDR);// call slave device for write.
		i2c_write(EPW_CMD_MODE);// Indicator that next byte is command
		i2c_write(packet);// write data to EPW14040AA1.
	
	i2c_stop();// generate stop condition.
	
}

// I2C send double CMD byte
void EPW_sendCMD2(uint8_t pac1, uint8_t pac2){
	EPW_sendCMD(pac1);// write CMD to EPW14040AA1.
	EPW_sendCMD(pac2);// write Param of above CMD to EPW14040AA1.
}

// Initialize the Display
void EPW_start(){
	PC_ODR |= (1 << EPW_RSTB);// set RSTB high.
	delay_us(10);// wait a little bit.
	PC_ODR |= (1 << EPW_VBOOST);// turn the TPS63060 on (EN pin default is pull high by resistor, remove that resistor).
	
	delay_ms(1);
	EPW_sendCMD2(EPW_SETCONT, EPW_CONT);// set contrast. 
	EPW_sendCMD2(EPW_SETVCOM, EPW_VCOM);// select VCOMH/IREF.
	EPW_sendCMD2(EPW_SETOSC,	EPW_OSC);// set Clock Divide Raton and Oscillator Freq .
	EPW_sendCMD2(EPW_SETPCW, EPW_PCW);// set Precharge Width.
	EPW_sendCMD2(EPW_SETVCDE, EPW_VCDE);// set VCOMH Deselect Level.
	EPW_sendCMD2(EPW_SETMUXR, EPW_MUXR);// set Mux Ratio.
	EPW_sendCMD2(EPW_SETDSOF, EPW_DSOF);// set Display offset.
	EPW_sendCMD2(EPW_SETPAM,	EPW_PAM);// set Memory addressing mode to Page address (2 pages, 8 bit width each).
}

// Turn Display on/off (for standby mode). 
void EPW_DispOn(bool on){
	if (on){
		EPW_sendCMD(EPW_Wake);// Turn Display on.
		PC_ODR |= (1 << EPW_VBOOST);// Turn on TPS63060 / LTC3459 (10v boost IC).
	}else{
		EPW_sendCMD(EPW_Sleep);// Turn Display off.
		PC_ODR &= ~(1 << EPW_VBOOST);// Turn off TPS63060 / LTC3459.
	}
}

// Update Display  
void EPW_Update(){
	
for (uint8_t p=0; p < 2; p++){// Do this twice for 2 mem pages.
	// Byte Flip 90 degree
	for (uint8_t FBOff = 0; FBOff < 16; FBOff ++){// 16 bytes column of FB0.
		for (uint8_t i=0; i < 8; i++){// read each bit of every byte from FB0.
			for (uint8_t j=0; j < 8; j++){// Single bit of FB0 every 16n byte is rotated to vertical byte 
				PG[FBOff+1] |= ( ( FB0[(16*j) /*Row select*/ +(FBOff/8) /*Horizontal offset, move to next FB0 byte every 8 column*/ + (p ? EPW_FB_HALFSIZE : 0) /* Page changing */ ] && (1 << (i%8)) ) << j);
			}
		}
	}
				// select Memory page before updating.
	EPW_sendCMD(p ? EPW_PGSEL_1 : EPW_PGSEL_0);// Select whether to send to Page number 0 or 1.
	
	i2c_start();// generate start condition.
	
	i2c_write_addr(EPW_ADDR);// call slave device for write.
		i2c_write(EPW_DAT_MODE);// send 0x40 to indicate that this is Display data
		i2c_writePtrAuto(PG);// sequential write to Display, 128 byte data of that page
		
	i2c_stop();// generate stop condition.
}
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
	memset(FB0, 1, EPW_FB_SIZE);
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
		//XYoff = (Ycord+loop) * 128;
		//XYoff += Xcord;// offset start at the left most, The count from left to right for Xcord times

		// turn W and H into absolute offset number for Bitmap image
		//WHoff = loop * bmpW;
		memcpy(FB0 + ((Ycord + loop) * 128), BMP + (loop * bmpW), bmpW);
	}

}


//Print 8x8 Text on screen
void EPW_Print(char *txtBuf){

uint16_t chOff = 0;

while(*txtBuf){
	// In case of reached 50 chars or newline detected , Do the newline
	if ((Xcol > 16) || *txtBuf == 0x0A){
		Xcol = 1;// Move cursor to most left
		YLine += 8;// enter new line
		txtBuf++;// move to next char
	}

	// Avoid printing Newline
	if (*txtBuf != 0x0A){
	chOff = (*txtBuf - 0x20) * 8;// calculate char offset (fist 8 pixel of character)
	EPW_LoadPart(font8x8_basic + chOff, Xcol, YLine, 1, 8);// Align the char with the 8n pixels

	txtBuf++;// move to next char
	Xcol++;// move cursor to next column
	}
  }
}

//========================================================================================================================

// Timer setup (for sleep time out counter)
//========================================================================================================================
void TIM4_counter_init(){// We are using TIM4 as a screen timeout counter
	CLK_PCKENR1 |= 0x04;// enable the TIM4 clock
}

//========================================================================================================================

void joystick_irq(void) __interrupt(6){
	readPin = PB_IDR;// quickly read the GPIO for further mode select.
	
	//reset_countdown_timer
	TIM4_SR &= ~0x01;// reset sleep cont down timer when pressing button.
	countdown = TIM4_CNTR;// recount timer.
	
	EXTI_SR2 = 0x01;// clear interrupt pending bit of PortB interrupt.
}

void timeUpdate(){
	
	countdown = TIM4_CNTR;
	
	while ((TIM4_CNTR - countdown) < SCREEN_TIMEOUT){
		
	}
}

// CPU enter active halt mode.
void watch_halt(){
	EPW_DispOn(false);// turn display of via command then turn of the Boost IC.
	__asm__("halt");// halt CPU, Now in active halt mode. Only RTC and ITC are running.
}

void watch_showMenu(uint8_t menum){

	switch(menum){
		case 0:// show current time
			// grep time from shadow registries
			//liteRTC_grepTime(&hour, &minute, &second);
			// I don't implement the grepTime that return(?) the BCD number, so just read directly from shadow registries

			second = RTC_TR1;
			minute = RTC_TR2;
			hour   = RTC_TR3 & 0x3F;
			
			// convert BCD  to char 
			num2Char[0] = (second >> 4) + 0x30;// convert the tens digit to char 
			num2Char[1] = (second & 0x0F) + 0x30;// convert the ones digit to char 
			
			num2Char[3] = (minute >> 4) + 0x30;// convert the tens digit to char 
			num2Char[4] = (minute & 0x0F) + 0x30;// convert the ones digit to char 
			
			num2Char[6] = (hour >> 4) + 0x30;// convert the tens digit to char 
			num2Char[7] = (hour & 0x0F) + 0x30;// convert the ones digit to char 
			
			// Print to Display and update
			EPW_Print("Current Time:\n");
			EPW_Print(num2Char);
			EPW_Update();
			break;
		case 1:// show Day of week, Date Month and year.
		
			//liteRTC_grepDate(&Day, &Date, &Month, &Year);
			// I don't implement the grepTime that return(?) the BCD number, so just read directly from shadow registries

			Date = RTC_DR1;
			Month = RTC_DR2;
			Day = (uint8_t)(Month >> 5);// grep day of week from Month registry.
			Year = RTC_DR3;
			
			Month &= 0x1F;// clean out the Day of week bits.
			
			EPW_Print("Current Date:\n");
			EPW_Print(&DayNames[Day]);
			EPW_Print("--");
			
			num2DMY[0] = (Date >> 4) + 0x30;
			num2DMY[1] = (Date & 0xF0) + 0x30;
			
			num2DMY[3] = (Month >> 4) + 0x30;
			num2DMY[4] = (Month & 0x0F) + 0x30;
			
			num2DMY[8] = (Year >> 4) + 0x30;
			num2DMY[9] = (Year & 0xF0) + 0x30;
			
			EPW_Print(num2DMY);
			break;
		default:
			break;
	}
			EPW_Update();
}

void watch_handler(){
	if(readPin != 0xFF){// if any pin is triggered interrupt
			EPW_DispOn(true);// turn display on 
	}
	
	// start counter for screen timeout.
	TIM4_ARR = 242; //set prescaler period for 0.5s 
	TIM4_PSCR = 0x0F;// set prescaler, divide sysclock to 487Hz for making 0.5s counter 
	TIM4_EGR = 0x01;// event source update 
	// start timer 
	TIM4_CR1 |= 0x01;// the time at the starting point
	
	countdown = TIM4_CNTR;// snapshot of start time.
	
	while ((TIM4_CNTR - countdown) < SCREEN_TIMEOUT){// find delta for how long does this loop currenty take.
		switch(readPin){
			
		case 0xFC:// Center button is pressed.
			TimeUpdate_lock +=1;
				if(TimeUpdate_lock == 2){
					//timeupdate();
				}
			break;
		case 0xF7:// Right button is pressed.
			menuTrack++;
			if(menuTrack > (MAX_MENU-1))
				menuTrack = 0;

			break;
			
		case 0xFE: // Left button is pressed.
			menuTrack--;
			if(menuTrack > (MAX_MENU-1))
				menuTrack = 0;
			
			break;
			
		default:
			break;
		}
		
		watch_showMenu(menuTrack);// put in while loop make it possible to update display every .5 second
	}
	
	// reset counter 
	TIM4_SR &= ~0x01;
	// Turn TIM4 off
	TIM4_CR1 &= ~0x01;    
	
	watch_halt();// enter sleep mode.
}

void main(){
	GPIO_init();// GPIO Init
	i2c_init(0x00, I2C_400K);// I2C at 400KHz
	TIM4_counter_init();// Enable TIM4 clock for Screen timeout timer.
	
	// Clean Frame Buffer for Fresh init.
	EPW_Clear();
	
	//Start the OLED display
	EPW_start();
	delay_ms(500);
	
	// Splash text
	EPW_Print("Fl3xWatch OLED\n");
	EPW_Print("By TinLethax");
	EPW_Update();
	delay_ms(1000);
	watch_showMenu(0);// Show time on screen
	
	while(1){
		watch_handler();
	}// While loop
  
}// main 
