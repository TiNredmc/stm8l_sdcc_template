/* Fl3xWatchOLED project using Futaba Flexible OLED display EPW1404AA1 (SSD1316) running wiht STM8L151F3.
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

// Define Left hand mode for who want to wear this on left hand.
// 9.9 in 10 people wears watch on left hand. I'm the 0.1 who wear on right hand.
//#define LEFT_HANDED

// Define pin to use 
// RSTB
#define EPW_RSTB		4// PC4
// 10Volt boost enable pin of TPS63060 (Dev version) or LTC3459 (Watch version).
#define EPW_VBOOST		5// PC5

// Necessary Definitions of EPW14040AA1
#define EPW_FB_SIZE 	256 	// define the memory size used for Frame Buffer (128x2 bytes = 256 bytes).

#define EPW_FB_HALFSIZE 128 // define the memory size used for the Send buffer.

#define EPW_ADDR 		0x3c // 7bit i2c address already.

#define EPW_CMD_MODE 	0x80 // First byte to send before sending any command-related byte.

#define EPW_DAT_MODE 	0x40 // First byte to send before sending any Display data. 

#define EPW_Sleep		0xAE // command to turn display off (sleep).
#define EPW_Wake		0xAF // command to turn display on (wake from sleep).

/* init commands sequence*/

#define EPW_SETCONT		0x81 // command to set contrast.
#define EPW_CONT		0x48 // contrast value that need to be set. Set to Max.

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
#define EPW_MUXR		0x0F // 15 + 1 Mux (16 com/row strating from 0 - 15).

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

static uint8_t PG;// single page vertical byte format for data sending.

//These Vars required for print function
static uint8_t YLine = 1;
static uint8_t Xcol = 1;

/* Wakeup interrupt and menu navigation stuffs */

// store PB_IDR to this variable to further detect which button is pushed/pressed. 
volatile uint8_t readPin = 0;
uint8_t prevReadPIn = 0;

// press center button to enter time update mode.
uint8_t TimeUpdate_lock = 0;

// keeping track of menu (button press cycle).
uint8_t menuTrack = 0;
#define MAX_MENU	2 // maximum menu pages

#define SCREEN_TIMEOUT	5 // screen timeout 5 secs. since we use .5s counter (1 second counter is counting up 2 times).
volatile uint8_t countdown = 0;// screen timout keeper.

/* Date and Time stuffs */

// Time keeping, read from RTC shadow registries.
uint8_t hour, minute, second;
uint8_t num2Char[8]={0,0,':',0,0,':',0,0};// store the time in char format "HH:MM:SS"

// DMY keeping, read from RTC shadow registries.
uint8_t Day, Date, Month, Year;
// convert day of week number to day name.
const static uint8_t DayNames[8]={"ERR","MON", "TUE", "WEN", "THU", "FRI", "SAT", "SUN"};
uint8_t num2DMY[10]={0,0, '/', 0,0, '/', '2','0', 0, 0};// store the date, month and year in cahr format "DD/MM/20YY"

/* Look up tables*/

// convert cursor position to array position.
const static uint8_t tPosLUT[8] = {0,1 ,0 ,3,4 ,0 ,6,7};// array position LUT for time update 
const static uint8_t dPosLUT[10] = {0,1 ,0 ,3,4 ,0 ,0 ,0 ,8,9};// array position LUT for date update

// column skipping steps, to make sure that cursor never lands on the fixed Char.
//								M,O,N, -,- ,0,1, /, 0,1, /, 2,0, 2,1
const static uint8_t DateColFWLUT[15] = {0,0,0, 2,1 ,0,0, 1, 0,0, 3, 2,1 ,0,0};// for cursor going forward
const static uint8_t DateColBWLUT[15] = {0,0,0, 1,2 ,0,0, 1, 0,0, 1, 2,3 ,0,0};// for cursor going backward

// Maximum value of Time and Date. Char data type.
//				   				    2,3/9, 5,9, 
const static uint8_t timeMaxVal[8]={'2','9',	':', '5','9', ':', '5','9'};// maximum value of each digit on Time.
// 									   3,1,     1,2, 		20,9,9
const static uint8_t dateMaxVal[10] = {'3','9', '/', '1','9', '/' ,'2','0', '9','9'};// maximum value of each digit on Date
const static uint8_t date3031[12]={'1','0','1','0','1','0','1','1','0','1','0','1'};// maximum value of ones digit of Date when It's 30 or 31.

/* Extra */
// External function, fast memcpy (Big thanks lujji for making this possible). check the fastmemcpy.s on the same folder for assembly code.
extern void fast_memcpy(uint8_t *dest, uint8_t *src, uint8_t len);

/* Low level stuffs */
// GPIO init and interrupt 
void GPIO_init(){
	
	// Init Port C for RSTB and Vboost .
	PC_DDR |= (1 << EPW_RSTB) | (1 << EPW_VBOOST);// Set PC4,5 as output
	PC_CR1 |= (1 << EPW_RSTB) | (1 << EPW_VBOOST);// with push pull mode.
	PC_CR1 |= 0x40;// configure PB6 as input pull up, This will stop noise from triggering Schimitt trigger (low power design).
	
	/* Port B to switch connection (please take look at the schematic of Fl3xWatch)
	PB0 -> LEFT
	PB1 -> CENTER
	PB2 -> Up
	PB3 -> Right
	PB4 -> Down
	*/
	// Init entire port B as input but only B0 to B4 as a input with PortB interrupt.
	PB_DDR = 0;// PB0-PB7 as input, We only use PB0-4 other pins just left unused with internal pull-up (low power design).
	PB_CR1 = 0xFF;// Entire port With pull up resistor.
	PB_CR2 = 0x1F;// and Interrupt only on PB0 to PB4.
	
	// Enable Port B interrupt.
	ITC_SPR2 |= (3 << 4);// set VECT6SPR (Entire portB interrupt) priority to high.
	
	// We need to do the EXTI setup in sequence.
	EXTI_CR3 = 0x02;// set interrupt sensitivity to detect falling edge(PortB).
	EXTI_CONF1 |= 0x03;// Enable Port B interrupt source.
	EXTI_CONF2 &= (uint8_t)~0x20;// Port B instead of Port G is used for interrupt generation.
}
// Bug not occur, so don't declair this function. save space.
// Catch bad guy that might cause CPU reset because of unintended interrupt generated by I2C
/*void catchI2C_irq(void) __interrupt(29){
	// Dummy reading the SR registries
	(void)(I2C1_SR2);
	(void)(I2C1_SR1);
	(void)(I2C1_SR3);
}*/

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
	EPW_sendCMD2(EPW_SETOSC,  EPW_OSC);// set Clock Divide Ratio and Oscillator Freq .
	EPW_sendCMD2(EPW_SETPCW,  EPW_PCW);// set Precharge Width.
	EPW_sendCMD2(EPW_SETVCDE, EPW_VCDE);// set VCOMH Deselect Level.
	
	// On display setup (for left/right handed).
#ifdef LEFT_HANDED
	EPW_sendCMD(EPW_SEGREMP_LH);// swap the column when put this watch on left hand
#else
	EPW_sendCMD(EPW_SEGREMP_RH);// Don't swap the column because we're using right handed mode.
#endif 
	EPW_sendCMD2(EPW_SETMUXR, EPW_MUXR);// set Mux Ratio to 15 (because the Display has row 0 to row 15).
	EPW_sendCMD2(EPW_SETDSOF, EPW_DSOF);// set Display column offset.
#ifndef LEFT_HANDED
	EPW_sendCMD(EPW_ROWRV);// Reverse row (segment) for right handed.
#endif 
	EPW_sendCMD2(EPW_SETPHC,  EPW_PHC);// set Pin Hardware configuration for setting segment order
	EPW_sendCMD2(EPW_SETPAM,  EPW_PAM);// set Memory addressing mode to Page address (2 pages, 8 bit width each).
	
	// Turn display on.
	EPW_sendCMD(EPW_Wake);
}

// Turn Display on/off (for standby mode). 
void EPW_DispOn(bool on){
	if (on){
		PC_ODR |= (1 << EPW_VBOOST);// Turn on TPS63060 / LTC3459 (10v boost IC).
		EPW_sendCMD(EPW_Wake);// Turn Display on.
	}else{
		EPW_sendCMD(EPW_Sleep);// Turn Display off.
		PC_ODR &= ~(1 << EPW_VBOOST);// Turn off TPS63060 / LTC3459.
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
	
	for (uint16_t FBOff = 0; FBOff < 256; FBOff ++){// 128 columns, 2 pages (for each verical byte PB[]).
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
// Clear all pixels of Frame Buffer 
void EPW_Clear(){
	Xcol = 1;
	YLine = 1;
	memset(FB0, 0, EPW_FB_SIZE);
}

// Buffer update (with X,Y Coordinate and image WxH) X,Y Coordinate start at (1,1) to (16,128)
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
		fast_memcpy(FB0 + ((Ycord + loop) * 16) + Xcord, BMP + (loop * bmpW), bmpW);
	}

}


// Same as EPW_LoadPart(), but will merge the input bitmap with current pixels data on Frame Buffer.
void EPW_LoadPartMerge(uint8_t* BMP, uint8_t Xcord, uint8_t Ycord, uint8_t bmpW, uint8_t bmpH){
	Xcord--;
	Ycord--;
	
	//Counting from Y origin point to bmpH using for loop
	for(uint8_t loop = 0; loop < bmpH; loop++){
		for(uint8_t width = 0; width < bmpW; width++)
			FB0[((Ycord + loop) * 16) + Xcord  +width]	|= *(BMP + (loop * bmpW) + width);
	}
}

//Print 8x8 Text on screen
void EPW_Print(char *txtBuf,uint8_t len){

//uint16_t chOff = 0;

while(len--){
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

//========================================================================================================================

// Entry: User Interface stuffs 
//========================================================================================================================

// Interrupt Handler for PortB smd joy stick switches.
void joystick_irq(void) __interrupt(6){
	readPin = PB_IDR;// quickly read the GPIO for further mode select.

	//reset_countdown_timer
	countdown = RTC_TR1;// recount timer.
	
	EXTI_SR2 = 0x01;// clear interrupt pending bit of PortB interrupt.
}

// Update Time and Date.
void watch_Update(){
	readPin = 0xFF;// reset pin read state
	uint8_t prevXcol = 1;// store previous cursor Xcol position and restore it after EPW_Print.
	EPW_Clear();
	
	if(menuTrack == 0)
		EPW_Print("Adjust Time:\n", 13);// Menu number 0 : Time.
	else
		EPW_Print("Adjust Date:\n", 13);// Menu number 1 : Date.

	countdown = RTC_TR1;// first lap
	
	while ((RTC_TR1 - countdown) < (SCREEN_TIMEOUT*2)){
		switch(readPin){
			case 0xF7: // right button is pressed.
				// move cursor forward.
				Xcol++;
				
				if(menuTrack == 0){
					// Cursor never lands on ':'
					if(num2Char[Xcol-1] == ':')// detect the ':' 
						Xcol++;
				}else{
					// according to the lut table 
					Xcol += DateColFWLUT[Xcol-1];
				}	
					
				if (Xcol > (menuTrack ? 15 : 8))// Cursor never goes beyond 8 (Time) or 15 (Date).
					Xcol = menuTrack ? 15 : 8;
				break;
				
			case 0xFE: // left button is pressed.
				// move cursor backward.
				if (Xcol == 1)// Cursor never goes backward beyond 1.
					break;
					
				Xcol--;
				if(menuTrack == 0){
					if(num2Char[Xcol-1] == ':')// detect the ':' 
						Xcol--;
				}else{
					Xcol -= DateColBWLUT[Xcol-1];	
				}
					
				break;
			
			case 0xFB: // Up button is pressed.
				// Increase value (of that digit).
				if(menuTrack == 0){
					// Time Update
					num2Char[tPosLUT[Xcol-1]]++;
					
					// check whether the number is out of bound. 
					if(num2Char[tPosLUT[Xcol-1]] > timeMaxVal[tPosLUT[Xcol-1]])
						num2Char[tPosLUT[Xcol-1]] = timeMaxVal[tPosLUT[Xcol-1]];
					
					// Hour maximum is 23
					if((num2Char[0] == '2') && (num2Char[1] > '3'))// Hour can't greater than 23.
							num2Char[1] = '3';
					
				}else{
					// Date update
					if(Xcol < 4){// column is in region of Day of week.
						Day++;
						if(Day > 7)
							Day = 7;
					}else{
						
						// Date Update.
						num2DMY[dPosLUT[Xcol-6]]++;
						
						uint16_t year = 2000 + ( (num2DMY[8]-0x30) *10 + num2DMY[9]-0x30);// store the number of year to calculate the leap year.
						
						// check whether the number is out of bound. 
						if(num2DMY[dPosLUT[Xcol-6]] > dateMaxVal[dPosLUT[Xcol-6]])
							num2DMY[dPosLUT[Xcol-6]] = dateMaxVal[dPosLUT[Xcol-6]];
						
						// Date maximum is 31, with leap year check.
						if(((num2DMY[3]-0x30)*10 + num2DMY[4]-0x30) == '2'){// Check if it's Feb.
								if(num2DMY[0] > '2')// date tens digit can't greater than 2
									num2DMY[0] = '2';
								
								if( ((year%4) == 0) && (((year%400) == 0) || ( (year%100) != 0 )) ){// If it's leap year.
									if(num2DMY[1] > '9') // Allow up to 29th Feb.
										num2DMY[1] = '9';
								}else{
									if(num2DMY[1] > '8')// Otherwise It's only 28th Feb.
										num2DMY[1] = '8';
								}
							
						}
						
						
						if(num2DMY[0] == '3'){// if it's day 30 or 31.
							if(date3031[((num2DMY[3]-0x30)*10 + num2DMY[4]-0x30) - 1]){// use LUT to check which month has 30 or 31 days (except Feb which is already dealed with above if-elses).
								if(num2DMY[4] > date3031[((num2DMY[3]-0x30)*10 + num2DMY[4]-0x30) - 1])// check for number that out of bound.
									num2DMY[4] = date3031[((num2DMY[3]-0x30)*10 + num2DMY[4]-0x30) - 1];
							}
						}

						// Month maximum is 12
						if(num2DMY[3] == '1'){// if it's month 10 11 or 12
							if(num2DMY[4] > '2')// Month can't greater than 12.
								num2DMY[4] = '2';
						}
						
					}
				}
				break;
			
			case 0xEF: // Down button is pressed.
				// Decrease value (of that digit).
				if(menuTrack == 0){
					if(num2Char[tPosLUT[Xcol-1]] == '0')// never goes below 0 and get rick roll over.
						break;
					num2Char[tPosLUT[Xcol-1]]--;
				}else{
					if(Xcol < 4){// column is in region of Day of week.
						if(Day == 1)
							break;
						Day--;
					}else{
						if(num2DMY[dPosLUT[Xcol-6]] == '1')// never goes below 0 and get rick roll over.
							break;
						num2DMY[dPosLUT[Xcol-6]]--;
					}
				}
				break;
					
			case 0xFD: // center button is pressed
					// save Time/Date setting.
				TimeUpdate_lock +=1;
				if(TimeUpdate_lock == 2){// If pressed 2 times.
					TimeUpdate_lock = 0;
					__asm__("sim");// spinlock
					if(menuTrack == 0){
						liteRTC_SetHMSBCD(((num2Char[0] - 0x30) << 4) | (num2Char[1] - 0x30), ((num2Char[3] - 0x30) << 4) | (num2Char[4] - 0x30), ((num2Char[6] - 0x30) << 4) | (num2Char[7] - 0x30));
					}else{
						liteRTC_SetDMYBCD(Day, ((num2DMY[0]-0x30) << 4) | (num2DMY[1]-0x30), ((num2DMY[3]-0x30) << 4) | (num2DMY[4]-0x30), ((num2DMY[8]-0x30) << 4) | (num2DMY[9]-0x30));
					}
					__asm__("rim");// de-spinlock
					
					// reset the Cursor and line position, watch_showMenu() will redraw the text on the FB0.
					YLine = 1;
					Xcol = 1;
					return;// exit the timeUpdate function
				}
				
			default:
				break;
		}
		
	readPin = 0xFF;// reset pin read state
	YLine = 9;// Force the Cursor to stay on 2nd line
	prevXcol = Xcol;// Backup
	Xcol = 1;// reset column
	if(menuTrack == 0){
		EPW_Print(num2Char, 8);
	}else{
		EPW_Print(&DayNames[Day], 3);
		EPW_Print("--", 2);
		EPW_Print(num2DMY, 10);
	}
	EPW_LoadPartMerge(font8x8_basic + 504, prevXcol, 9, 1, 8);// underscore the currently selected digit.

	Xcol = prevXcol;// Restore.
	EPW_Update();// Update display
	}
}


// Show each menu on screen.
void watch_showMenu(uint8_t menum){
	EPW_Clear();
	
	switch(menum){
		case 0:// show current time
			// grep time from shadow registries
			// liteRTC_grepTime(&hour, &minute, &second);
			// I don't implement the grepTime that return(?) the BCD number, so just read directly from shadow registries
			second = RTC_TR1;
			minute = RTC_TR2;
			hour   = RTC_TR3 & 0x3F;
			
			// convert BCD  to char 
			num2Char[6] = (second >> 4) + 0x30;// convert the tens digit to char 
			num2Char[7] = (second & 0x0F) + 0x30;// convert the ones digit to char 
			
			num2Char[3] = (minute >> 4) + 0x30;// convert the tens digit to char 
			num2Char[4] = (minute & 0x0F) + 0x30;// convert the ones digit to char 
			
			num2Char[0] = (hour >> 4) + 0x30;// convert the tens digit to char 
			num2Char[1] = (hour & 0x0F) + 0x30;// convert the ones digit to char 
			
			// Print to Display and update
			EPW_Print("Current Time:\n", 14);
			EPW_Print(num2Char, 8);
			break;
		case 1:// show Day of week, Date Month and year.
			
			//liteRTC_grepDate(&Day, &Date, &Month, &Year);
			// I don't implement the grepTime that return(?) the BCD number, so just read directly from shadow registries

			Date = RTC_DR1;
			Month = RTC_DR2;
			Day = (uint8_t)(Month >> 5);// grep day of week from Month registry.
			Year = RTC_DR3;
			
			Month &= 0x1F;// clean out the Day of week bits.
			
			num2DMY[0] = (Date >> 4) + 0x30;
			num2DMY[1] = (Date & 0x0F) + 0x30;
			
			num2DMY[3] = (Month >> 4) + 0x30;
			num2DMY[4] = (Month & 0x0F) + 0x30;
			
			num2DMY[8] = (Year >> 4) + 0x30;
			num2DMY[9] = (Year & 0x0F) + 0x30;

			
			EPW_Print("Current Date:\n", 14);
			EPW_Print(&DayNames[Day], 3);
			EPW_Print("--", 2);
			EPW_Print(num2DMY, 10);
			break;
		default:
			break;
	}
			EPW_Update();
}

// Watch thingy stuffs handler.
void watch_handler(){

	EPW_DispOn(true);// turn display on 
	watch_showMenu(menuTrack);
	// start counter for screen timeout.
	countdown = RTC_TR1;// snapshot of start time.
	
	while ((RTC_TR1 - countdown) < SCREEN_TIMEOUT){// find delta for how long does this loop currenty take.
		switch(readPin){
			
		case 0xFD:// Center button is pressed. PB1
			TimeUpdate_lock +=1;
				if(TimeUpdate_lock == 2){// If pressed 2 times.
					TimeUpdate_lock = 0;// release Time update lock.
					watch_Update();// enter time/date update mode.
				}
			
			break;
			
		case 0xF7:// Right button is pressed. PB3
			TimeUpdate_lock = 0;// release Tim update lock, make sure that accidentally pressing center won't trigger Time/Date Update.
			menuTrack++;// navigate to next menu.
			if(menuTrack > (MAX_MENU-1))
				menuTrack = 0;
			
			break;
			
		case 0xFE: // Left button is pressed. PB0
			TimeUpdate_lock = 0;// release Tim update lock, make sure that accidentally pressing center wont trigger Time/Date Update
			if(menuTrack == 0){// if already on the first menu 
				menuTrack = MAX_MENU - 1;// roll over to the last menu page
				break;
			}
			menuTrack--;// navigate to previous menu.

			break;
			
		default:
			break;
		}
		readPin = 0xFF;// reset pin read state
		watch_showMenu(menuTrack);// put this outside swithc case to make it possible to update display every .5 second
	}
	
	// reset counter 
	countdown = 0;
	
	// enter sleep mode.
	EPW_DispOn(false);// turn display of via command then turn of the Boost IC.
	__asm__("halt");// halt CPU, Now in active halt mode. Only RTC and ITC are running.
}

//========================================================================================================================


// Entry: Main 
//========================================================================================================================

void main(){
	GPIO_init();// GPIO Init
	i2c_init(0x00, I2C_400K);// I2C at 400KHz, SDA on PC0 SCL on PC1
	liteRTC_Init();// Init RTC peripheral.
	PWR_CSR2 |= (1 << 1);// disable ADC ref voltage regulator when sleeping. (No effect sine we don't use ADC).
	
	__asm__("rim");// enable interrupt controller.
	// Clean Frame Buffer for Fresh init.
	EPW_Clear();
	
	// Start the OLED display
	EPW_start();
	while(1){
		watch_handler();// watch handler, including menu select, Enter update mode, and put to sleep.
	}// While loop
  
}// main 
