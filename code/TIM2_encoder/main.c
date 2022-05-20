/* Encoder example with TIM2
 * Coded by TinLethax 2022/05/20
 */
#include <stdio.h>
#include <stdint.h>
#include <stm8l.h>
#include <delay.h>
#include <usart.h>

#define ENCA	0	// PB0
#define ENCB	2	// PB2

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}


void tim2_encinit(){
	CLK_PCKENR1 |= 0x01;// Enable TIM2 timer
	//Timer 2 init 
	TIM2_ARRH = 0x03;
	TIM2_ARRL = 0xE8;

	TIM2_PSCR = (uint8_t)10; 

	TIM2_EGR = 0x01;

	// TIME2 capture mode 2 channels.
	
	// Set Capture compare 1 and 2 as input.
	uint8_t tmpccmr1 = TIM2_CCMR1;
	uint8_t tmpccmr2 = TIM2_CCMR2;
	tmpccmr1 &= ~0x03;
	tmpccmr1 |= 0x01;
	tmpccmr2 &= ~0x03;
	tmpccmr2 |= 0x01;
	
	// set Enacoder polarity detecion (Falling edge detection).
	TIM2_CCER1 |= 0x22; 
	TIM2_CCMR1 = tmpccmr1;
	TIM2_CCMR2 = tmpccmr2;
	TIM2_SMCR |= 0x03;// Set encoder mode to 2 channels mode.
	
	//Enable TIM2
	TIM2_CR1 = 0x01;
}

void main() {
	CLK_CKDIVR = 0x00;
	
	PB_CR1 |= (1 << ENCA) | (1 << ENCB);// set all encoder pin to have pull up.
	usart_init(9600);// usart using baudrate at 9600.
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).

	uint16_t encoder_cnt = 0;
	uint16_t encoder_prev = 0;
    while (1) {
	
	encoder_cnt = (TIM2_CNTRH << 8) | TIM2_CNTRL; 
	
	if(encoder_cnt != encoder_prev){
		printf("Encoder : %d\n", encoder_cnt);
		
	}
	
	encoder_prev = encoder_cnt;
	
    }
}
