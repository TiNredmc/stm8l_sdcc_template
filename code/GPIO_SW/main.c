#include <stm8l.h>

#define LED_PIN     4 //PB4 
#define BTN	   5 // PB5

void main() {
	CLK_CKDIVR = 0x00;
	PB_DDR |= (1 << LED_PIN);//Data Direct register set 1 to the 3rd bit.
	PB_CR1 |= (1 << LED_PIN);//Set Control register for PB4 as push-pull output.

	PB_DDR |= (0 << BTN);// init pin 5 as input
	PB_CR1 |= (0 << BTN);// floating input
	//PB_CR2 |= (1 << BTN);// interrupt


    
	while (1) {

	while(PB_IDR & (1 << BTN)){
	PB_ODR = (1 << LED_PIN);
	}

	PB_ODR = (0 << LED_PIN);
	}
}
