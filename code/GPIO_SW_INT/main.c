/* Simple GPIO input example with EXTI (interrupt).
 * Coded by TinLethax
 */
#include <stdint.h>
#include <stm8l.h>
#include <delay.h>

#define LED_PIN     4 //PB4 
#define BTN	   5 // PB5

int tog = 1;

void exti_init(){
	PB_DDR |= (0 << BTN);// init pin 5 as input
	PB_CR1 |= (0 << BTN);// floating input
	PB_CR2 |= (1 << BTN);// interrupt

      ITC_SPR2 &= (uint8_t)(~(uint8_t)(0x03U << ((6 % 4U) * 2U)));;
      ITC_SPR2 |= (uint8_t)((uint8_t)(0x01) << ((6 % 4U) * 2U));

      EXTI_CR2 &=  (uint8_t)(~0x0C);
      EXTI_CR2 |= (uint8_t)((uint8_t)(0x03) << ((uint8_t)0x12 & (uint8_t)0xEF));

	__asm__("rim");
}

void EXTIirq(void) __interrupt(13){
	if(tog == 1){
	PB_ODR = (1 << LED_PIN);
	tog = 0;
	}else{
	PB_ODR = (0 << LED_PIN);
	tog = 1;	
	}
	EXTI_SR1 = 0x20;// clear interrupt pending of pin 5
}

void main() {
	CLK_CKDIVR = 0x00;
    PB_DDR |= (1 << LED_PIN);//Data Direct register set 1 to the 3rd bit.
    PB_CR1 |= (1 << LED_PIN);//Set Control register for PB4 as output.
	exti_init();

PB_ODR = (0 << LED_PIN);
    
    while (1) {
	__asm__("wfi");
    }
}
