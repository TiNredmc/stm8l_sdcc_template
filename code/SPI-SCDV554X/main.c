/* SCDV554X 4 digits vertical 5x5 LED matrix display example code running with RTC and Active halt mode.
 * Coded by TinLethax 2021/09/17 +7
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
#include <spi.h>
#include <liteRTC.h>
#include <font5X5.h>

#define SPI_CS	4// PC4 will be used as CS pin

#define SPI_SEL		PC_ODR &= ~(1 << SPI_CS)
#define SPI_UNSEL   PC_ODR |= (1 << SPI_CS)

uint8_t Hrs, Mins, Secs;

// Init GPIOs for Joy stock button and CS pin
void GPIO_init(){
	// Init PC4 as push-pull output and set at High level
	PC_DDR |= (1 << SPI_CS);
	PC_CR1 |= (1 << SPI_CS);
	PC_ODR |= (1 << SPI_CS);
	
	PB_CR1 |= 0x1F;// PB0-4 as input already, we just need to config it with pull up.
	
}
// clear the display
void scdv_clean(){
	uint8_t cleanCMD = 0xC0;
	SPI_SEL;
	SPI_Write(&cleanCMD, 1);
	SPI_UNSEL;
}

// Set brightness : 0 to 6
void scdv_setBrightness(uint8_t blvl){
	blvl += 0xF0;// add brightness setting command to that val.
	SPI_SEL;
	SPI_Write(&blvl, 1);
	SPI_UNSEL;
}

// put the screen to sleep, or wake it up 
void scdv_sleep(bool sleep){
	uint8_t sleepCMD = 0xFF;
	SPI_SEL;
	if(sleep)
		SPI_Write(&sleepCMD, 1);
	else
		 //just set brightness to wake it up again 
		scdv_setBrightness(1);
	
	SPI_UNSEL;
}

// Lamp test, test the display dots 
void scdv_test(bool test){
	uint8_t testCMD = 0xF8;// enter test mode with this command
	SPI_SEL;
	if(test)
		SPI_Write(&testCMD, 1);
	else
		scdv_clean();
		
	SPI_UNSEL;
}

// Write single character to display , Digit number start at 0 through 3
void scdv_writeChar(uint8_t digit, char n){
	if (digit > 3)// only 4 digits (0-3)
		return;
	// Select Digit to write
	SPI_SEL;
	digit += 0xA0;// add digit select command to the digit number.
	SPI_Write(&digit, 1);
	SPI_UNSEL;
	
	// Write row data to the display RAM
	SPI_SEL;
	SPI_Write(Font5x5 + ((n - 0x20) * 5), 5);
	SPI_UNSEL;
}

void main() {
	CLK_CKDIVR = 0x00;// Full 16MHz
	GPIO_init();
	SPI_Init(FSYS_DIV_2);// 16MHz/256 = 62.5kHz SPI clock
	liteRTC_Init();// Turn LSI on and Initialize RTC
	liteRTC_SetHMS(11, 0, 0);


    while (1) {
	//liteRTC_grepTime(&Hrs, &Mins, &Secs);
	/*Secs = RTC_TR1;
	Mins = RTC_TR2;
	Hrs = RTC_TR3 & 0x3F;
	scdv_writeChar(0, (Hrs << 4) + 0x30);
	scdv_writeChar(1, (Hrs & 0x0F) + 0x30);
	scdv_writeChar(2, (Mins << 4) + 0x30);
	scdv_writeChar(3, (Mins & 0x0F) + 0x30);
	delay_ms(1000);*/
	scdv_test(true);
	delay_ms(500);
	scdv_test(false);
	delay_ms(500);
    }
}
