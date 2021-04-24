/* Keyboard scanning code Example. 6x4 keypad, row (4 pins) is for Interrupt input, column (6 pins) is for scanning line 
 * Coded by TinLethax 2021/04/23 +7
 */

#include <stm8l.h>
//#include "int_handler.h"

/* Readback Pins 
 * Input internal pull-up
 */
#define IN0     0 //PD0
#define IN1	4 //PC4
#define IN2	5 //PC5
#define IN3 	6 //PC6

/* Scanning Pins
 * Open drain output (for pulling scanline LOW)
 */
#define SCN0	0 //PB0
#define SCN1	1 //PB1
#define SCN2	2 //PB2
#define SCN3	3 //PB3
#define SCN4	4 //PB4
#define SCN5	5 //PB4

uint8_t scan_pin = 0;
uint8_t HOLDER = 0;

#define ESC 0x10
#define SHIFT 0x11
#define CTRL 0x12
#define ALT 0x13 
#define SPBAR 0x14

static const uint8_t keys[4][6] = {
{ESC,'Q','W','E','R','T'},
{0,'A','S','D','F','G'},
{SHIFT,'1','2','3','4','B'},
{CTRL,0,0,ALT,0,0}
};

void int_line_0(void) __interrupt(8){ //Interrupt No. 8 (EXTI_0) 
	HOLDER = keys[0][scan_pin];
	EXTI_SR1 = 0x01;// clear interrupt pending of pin PD0
}

void int_line_1(void) __interrupt(12){ //Interrupt No. 12 (EXTI_4) 
	HOLDER = keys[1][scan_pin];
	EXTI_SR1 = 0x10;// clear interrupt pending of pin PC4
}

void int_line_2(void) __interrupt(13){ //Interrupt No. 13 (EXTI_5) 
	HOLDER = keys[2][scan_pin];
	EXTI_SR1 = 0x20;// clear interrupt pending of pin PC5
}

void int_line_3(void) __interrupt(14){ //Interrupt No. 14 (EXTI_6) 
	HOLDER = keys[3][scan_pin];
	EXTI_SR1 = 0x40;// clear interrupt pending of pin PC6
}

void gpio_setup(){
/* Readback pin setup*/
	PC_DDR |= (0 << IN1) | (0 << IN2) | (0 << IN3); // PC4-6 as Input
	PC_CR1 |= (1 << IN1) | (1 << IN2) | (1 << IN3); // Pull-up input	
	PC_CR2 |= (1 << IN1) | (1 << IN2) | (1 << IN3); // Pull-low interrupt detection 

	PD_DDR |= (0 << IN0); // PD0 as Input
	PD_CR1 |= (1 << IN0); // Pull-up input
	PD_CR2 |= (1 << IN0); // Pull-low interrupt detection 

/* Scanning pin setup */
	PB_DDR |= (1 << SCN0) | (1 << SCN1) | (1 << SCN2) | (1 << SCN3) | (1 << SCN4) | (1 << SCN5); // PB0-5 as  output 
	PB_CR1 |= (0 << SCN0) | (0 << SCN1) | (0 << SCN2) | (0 << SCN3) | (0 << SCN4) | (0 << SCN5); // Open-drain (Active Pull down)
}

void int_setup(){
/* set interrupt priority */
	ITC_SPR2 &= ~(3 << 4);
	ITC_SPR2 |= 1 << 4; 

	EXTI_CR1 |= (1 << 1);// Falling edge detection on PD0
	EXTI_CR2 |= 0x2A;// Falling edge detection on PC4-6
	__asm__("rim");// enble interrupt
}

void main() {
	CLK_CKDIVR = 0x00;// FULL 16 MEGHERTZ CPU!
	gpio_setup();
	int_setup();

    
	while (1) {// Now relying on crappy while(loop), soon will use timer interrupt 
		PB_ODR |= 0x77;// set PB0-5 to 1 (open-drain)
		PB_ODR &= (0 << scan_pin);// pull specific pin low 
		__asm__("sim");
		scan_pin = (scan_pin > 5) ? 0 : scan_pin + 1;// increase scanning counter 
		__asm__("rim");
	}
}
