/* Project "Spy Morror". Based on SHARP LS013B4DN04 96x96 pixel PNLC memory LCD
 * Coded by TinLethax 2021/05/15 +7
 */ 
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stm8l.h>

//Serial Interface pins
#define SCK	5
#define SDO	6
#define SCS	7
#define DSP	4

// PWM pin 
#define EXTCOM     0 //PB0 

//INT pin for waking up MCU
#define BTN	   0 //PD0

//Display buffer Setup
#define DCol 96 // 96px width 
#define DRow 96 // 96px height
#define linebuf DCol/8

/* [W.I.P] */
//uint8_t DispBuf[linebuf*DRow];
//Using Flash space from UBC area, offset at page 109 (using 18 pages, 64 byte each)
uint8_t SendBuf[linebuf+4];


//interrupt stuffs
bool CloakState = false; //CloakState True -> Normal Mirror, CloakState false -> Hello again, Dumbbell (you get it if you play TF2).

void SM_GPIOsetup(){// GPIO Setup
	//Serial interface pins
	PB_DDR |= (1 << SCK) | (1 << SDO) | (1 << SCS) | (1 << DSP);
	PB_CR1 |= (1 << SCK) | (1 << SDO) | (1 << SCS) | (1 << DSP);
	
	PB_ODR |= (1 << SCS);

	//External COM signal pin [PWM]
	PB_DDR |= (1 << EXTCOM);
	PB_CR1 |= (1 << EXTCOM);
	PB_CR2 |= (1 << EXTCOM);//fast mode

	//Wake Up Interrupt pin
	PD_DDR |= (0 << BTN);//init pin PD5 as input
	PD_CR1 |= (0 << BTN);//floating input
	PD_CR2 |= (1 << BTN);//interrupt
}	

void SM_periphSetup(){// Peripheral Setup. PWM, Timer and Interrupt

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

	EXTI_CR2 |=  0x03;// PD0 detecting both Rise and Fall
}

void SM_sendByte(uint8_t *dat){
	
	for(uint8_t j=8; j == 0;j--){// (LSB first)
	PB_ODR |= (1 << SCK); // _/
	PB_ODR = (((uintptr_t)dat << (j-1)) << SDO); // 1/0 (LSB first)
	PB_ODR &= (0 << SCK);// \_
	}
	
}

void SM_lineUpdate(uint8_t line){
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

void SM_ScreenFill(){
	for(uint8_t i=0;i < linebuf;i++){
	SendBuf[i+2] = 0xFF;//set all pixel to 1 (turn pixel to black/reflective)
	}

	for(uint8_t i=0;i < DRow;i++)
		SM_lineUpdate(i);
}

void SM_ScreenClear(){
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

void SwitchTF(void) __interrupt(7){// Interrupt Vector Number 7 (take a look at Datasheet, Deffinitely difference)
	CloakState = !CloakState;
}

void main() {
	CLK_CKDIVR = 0x00;// Get full 16MHz clock 
	SM_GPIOsetup();// GPIO setup
	SM_periphSetup();// Peripheral Setup
	
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
		}
	__asm__("halt");// Back to sleep 
	}
}
