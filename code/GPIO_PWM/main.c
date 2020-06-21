/* PWM Enample on Timer 2 Channel 0 on PB0
 * Coded by TinLethax
 */

#include <stdint.h>
#include <stm8l.h>
#include <delay.h>

#define LED_PIN     0 //PB0 

void main() {
	CLK_CKDIVR = 0x00;
	CLK_PCKENR1 |= 0x01;
	PB_DDR |= (1 << LED_PIN);//Direct register set 1 to the 0rd bit.
	PB_CR1 |= (1 << LED_PIN);//Set Control register for PB0 as output.
	PB_CR2 |= (1 << LED_PIN);// fast mode

	//Timer 2 init for PWM with ARR = 1000 (125Hz)
	TIM2_ARRH = 0x03;
	TIM2_ARRL = 0xE8;

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
    while (1) {
	for(uint16_t p=0;p < 1000;p++){
	TIM2_CCR1H =  (uint8_t)(p >> 8);
	TIM2_CCR1L =  (uint8_t)(p);
	delay_ms(1);
	}
	for(uint16_t p=1001;p > 0;p--){
	TIM2_CCR1H =  (uint8_t)(p >> 8);
	TIM2_CCR1L =  (uint8_t)(p);
	delay_ms(1);
	} 
    }
}
