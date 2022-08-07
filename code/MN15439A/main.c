/* MN15439A simple controller code for STM8L, No PWM (Connect the SIN1 SIN2 SIN3 of the display together)
 * Coded by TinLethax 2020/05/03 +7
 */
#include <stdint.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
//#include "BMP.h"

/* Define your pinout (MUST BE FAST MODE COMPATIBLE !!) 
 * If Changed, You must change the bset and bres inline assembly pin 
 * For example __asm__("bset 0x5005, #3"); >> __asm__("BitSet(to1) Px_ODR address, #PinNumer"); Logic 1
 * __asm__("bres 0x5005, #3"); >> __asm__("BitReset(to0) Px_ODR address, #PinNumer"); Logic 0
 */

#define BLK 3	// Display blanking on PB3
#define LAT 4	// Latching on PB4
#define CLK 5	// Clock line on PB5
#define Mo  6	// Master out on PB6

//Dummy bytes ; Format (MSB)[a,b,c,d,e,f,0,0](LSB) 
const static char img1[39]={
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54, 0xA8,
0x54, 0xA8, 0x54      };


const static uint8_t reOrder[2][6]={
{0,2,0,3,0,4}, /* the bit shift to convert def to the afbecd format */
{7,0,6,0,5,0} /* the bit shift to convert abc to the afbecd format */
}; 

void LBB(uint8_t Grid,char *rowdata);
static uint8_t EvOd = 0;
static uint8_t logic = 0;
static uint8_t GridNum = 1;

// GPIOs initialization, Super Fast 10MHz instead 2MHz
void initGPIOs(){
	PB_DDR |= (1 << BLK) | (1 << LAT) | (1 << CLK) | (1 << Mo); // set all pin as output
	PB_CR1 |= (1 << BLK) | (1 << LAT) | (1 << CLK) | (1 << Mo); // by using Push-pull mode
	PB_CR2 |= (1 << BLK) | (1 << LAT) | (1 << CLK) | (1 << Mo); // That Super fast at 10MHz max speed
	__asm__("bset 0x5005, #5"); // _/ clock need to be high if no data is being sent 
}

// TIM4 for display refreshing
void TIM4fps_init(){
	CLK_PCKENR1 |= 0x04;// enable the TIM4 clock
	/* TIM4_TimeBaseInit(pscrVal, arrVal)*/
	TIM4_ARR = 40-1; //set prescaler period
	TIM4_PSCR = 0x06;// set prescaler 
	TIM4_EGR = 0x01;// event source update 
	/*TIM4_cmd(ENABLE)*/
	TIM4_CR1 |= 0x01 ;
	/*TIM4_ClearFlag()*/
	TIM4_SR = (uint8_t)~0x01;
	/*TIM4_ITConfig(TIM4_IT_Update, ENABLE)*/	
	TIM4_IER |= 0x01;// make sure that I enable interrupt for the TIM4
	/*TIM4_cmd(ENABLE)*/
	TIM4_CR1 |= 0x01 ;
}

/* Interrupt handler for TIM4 */
void TIM4isr(void) __interrupt(25){// the interrupt vector is 25 (from the STM8L151F36U datasheet page 49)
	LBB(GridNum++, img1);
	if(GridNum == 53)
		GridNum = 1;
	/*TIM4_ITClearPendingBit()*/
	TIM4_SR = (uint8_t)~0x01;
}

// Very Long 288 bits bitbanging into 288 bits Shift register.

/* first 234 bits is divided into 39 part, each part contain 6 bit of dot data 
 * [1:1a,1f,1b,1e,1c,1d] [2:2a,2f,2b,2e,2c,2d] ..... [39:39a,39f,39b,39e,39c,39d] ; [Row Number:Column a, f, b, e, c, d]
 * The rest 54 bits (238 to 286) is the grid control bits 
 * [G1:235] [G2:236] ..... [G54:288] ; [Grid number : bit position] It seems like only grid 1 to 52 is usable 
 */

/* uint8_t Grid is the current Grid number for displaying certain region of the display, The active grid will be in the form N and N+1 by N is between 1 and 53
 * char *rowdata is the pointer that pointed to the array containing the Display bitmap.
 */    

void LBB(uint8_t Grid,char *rowdata){
// Logically thinking : Determine the Grid is Event or Odd number (Important, For the simple algoithm to convert abcdef to afbecd format).
	if(Grid%2){
	EvOd = 1;// event number (event grid), Only manipulate the d, e, f Dots
	}else{	
	EvOd = 0;// odd number (odd grid), Only manipulate the a, b, c Dots
	}
// Initial phase : Start communication
	__asm__("bset 0x5005, #3"); // Display blank, Set PB3 to 1
	__asm__("bset 0x5005, #4"); // Hold Latch for little while, Set PB4 to 1
	__asm__("nop");
	__asm__("nop");
	__asm__("bres 0x5005, #4"); // release Latch, Set PB4 to 0
	__asm__("bres 0x5005, #3"); // blank is done, Set PB3 to 0

// Bitmap phase : send the 234 bit of bitmap data
	for (uint8_t i=0;i< 39;i++){

	for (uint8_t a=0;a< 6;a++){
	__asm__("bres 0x5005, #5"); // \_

	memcpy(&logic, rowdata + i, 1);
	//logic = img1[i];// copy and move to the next data in the array
	logic = logic & (uint8_t)(1 << reOrder[EvOd][a]);
	
	if(logic) //sending the data
	__asm__("bset 0x5005, #6");// set PB5 to 1	
	else
	__asm__("bres 0x5005, #6");// Set PB5 to 0	


	__asm__("bset 0x5005, #5"); // _/
	}// for (int a=0;a< 6;a++)

	}// for (int i=0;i< 39;i++)


// Grid Activation phase 1 : Disable unused grid before Grid N
	for (uint8_t i=1;i<Grid;i++){
	__asm__("bres 0x5005, #5"); // \_
	__asm__("bres 0x5005, #6"); // The Unused Grid will be turn off
	__asm__("bset 0x5005, #5"); // _/

	}

// Grid Activation phase 2 : Only turn the Grid N and N+1 on by send 1 two times
	for (uint8_t i=0; i < 2; i++){
	__asm__("bres 0x5005, #5"); // \_
	__asm__("bset 0x5005, #6"); // two pair will be activated	
	__asm__("bset 0x5005, #5"); // _/

	}
	__asm__("bres 0x5005, #6"); // The Unused Grid will be turn off
// Grid Activation phase 3 : Disable the rest unused Grid after Grid N+1
	for (int i=Grid;i<53;i++){
	__asm__("bres 0x5005, #5"); // \_

	__asm__("bset 0x5005, #5"); // _/

	}
}

// TODO : implement PWM to provide GPC pin with W3iRd frequency shifting.

void main() {
	CLK_CKDIVR = 0x00; // Keep the clock at 16MHz with no clock divider
	initGPIOs(); // Init all pin that we want at Super fast 10MHz
	TIM4fps_init();
	__asm__("rim");// trun system interrupt controller on
 	while (1) {
	// Still working on it dude.
			
    	}
}
