/* 2 phase SPWM for Apple iPhone 6s Taptic Engine LRA on STM8L
 * Coded by TinLethax 2023/06/07 +7
 */

// TODO : 180 Out-of-phase sine Braking.

#include <stdint.h>
#include <stm8l.h>
#include <delay.h>

#define PWM_A	0 // U phase PWM PB0 (TIM2_CH1)
#define PWM_B	2 // W phase PWM PB2 (TIM2_CH2)

// Half - Sine Full 255 scale Look-Up table
const uint8_t lut[256] = {
  0,  6, 13, 19, 25, 31, 37, 44, 50, 56, 62, 68, 74,
 80, 86, 92, 98,103,109,115,120,126,131,136,142,147,
152,157,162,167,171,176,180,185,189,193,197,201,205,
208,212,215,219,222,225,228,231,233,236,238,240,242,
244,246,247,249,250,251,252,253,254,254,255,255,255,
255,255,254,254,253,252,251,250,249,247,246,244,242,
240,238,236,233,231,228,225,222,219,215,212,208,205,
201,197,193,189,185,180,176,171,167,162,157,152,147,
142,136,131,126,120,115,109,103, 98, 92, 86, 80, 74,
 68, 62, 56, 50, 44, 37, 31, 25, 19, 13,  6,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0 };

volatile uint8_t sinA = 0;
volatile uint8_t sinB = 0;

void TIM4isr(void) __interrupt(25){// ~55 clock cycle
	TIM2_CCR2L = lut[sinA];
	TIM2_CCR1L = lut[sinB];
	sinA++;
	sinB++;
	
	TIM4_SR = (uint8_t)~0x01;// clear peding bit
}

void initTim4(uint16_t HZ){
	// Prescaler vs frequency 
	// 0 -> 16 MHz to 62.5 kHz
	// 1 -> 8 Mhz to 31.25 kHz
	// 2 -> 4 Mhz to 15.625 kHz
	// 3 -> 2 MHz to 7812.5 Hz
	// 4 -> 1 Mhz to 3906.25 Hz
	// 5 -> 500 kHz to 1953.125 Hz
	// 6 -> 250 kHz to 976.5625 Hz
	// 7 -> 125 kHz to 488.28125 Hz
	// 8 -> 62.5 kHz to 244.140625 Hz
	// 9 -> 31.250 kHz to 122.0703125 Hz
	// 10 -> 15.625 kHz to 61.03515625 Hz
	// 11 -> 7812.5 Hz to 30.5 Hz
	// 12 -> 3906.25 Hz to 15.3 Hz
	// 13 -> 1953.125 Hz to 7.6 Hz
	
	if(HZ < 7)// Minimum freq is 7Hz
		return;
		
	if(HZ > 400)// Maximum freq is 400Hz
		return;
	
	HZ *= 256;
		
	if ((125000 / HZ) < 256){
		TIM4_ARR = (uint8_t)((125000 / HZ) - 1);
		TIM4_PSCR = 0x08;
	}else if ((62500 / HZ) < 256){
		TIM4_ARR = (uint8_t)((62500 / HZ) - 1);
		TIM4_PSCR = 0x08;
	}else if ((31250 / HZ) < 256){
		TIM4_ARR = (uint8_t)((31250 / HZ) - 1);
		TIM4_PSCR = 0x09;
	}else if ((15625 / HZ) < 256){
		TIM4_ARR = (uint8_t)((15625 / HZ) - 1);
		TIM4_PSCR = 0x0A;
	}else if ((7812 / HZ) < 256){
		TIM4_ARR = (uint8_t)((7812 / HZ) - 1);
		TIM4_PSCR = 0x0B;
	}else if ((3906 / HZ) < 256){
		TIM4_ARR = (uint8_t)((3906 / HZ) - 1);
		TIM4_PSCR = 0x0C;
	}else if ((1953 / HZ) < 256){
		TIM4_ARR = (uint8_t)((1953 / HZ) - 1);
		TIM4_PSCR = 0x0D;
	}

	TIM4_EGR = 0x01;// event source update 
	TIM4_CR1 |= 0x01;

	TIM4_SR = (uint8_t)~0x01;
	TIM4_IER |= 0x01;//Enable overflow interrupt of TIM4

	TIM4_CR1 |= 0x01;
}

void stopTimer(){
	TIM4_CR1 = 0x00;
}

void vibrateMs(uint16_t ms){
	initTim4(200);// ~200Hz resonant frequency of the iPhone 6s Taptic engine (LRA).
	delay_ms(ms);
	TIM2_CCR2L = 0;
	TIM2_CCR1L = 0;
	sinA = 0;
	sinB = 127;
	stopTimer();
}

void main() {
	CLK_CKDIVR = 0x00;// 16MHz system clock
	CLK_PCKENR1 |= 0x05;// Enable TIM2 TIM3 and TIM4 peripheral clock
	
	PB_DDR |= (1 << PWM_A) | (1 << PWM_B);// Set PWM pins as output
	PB_CR1 |= (1 << PWM_A) | (1 << PWM_B);// Push pull mode
	PB_CR2 |= (1 << PWM_A) | (1 << PWM_B);// with fast mode

	//Timer 2 init 
	
	// TIM2
	TIM2_ARRH = 0x00;
	TIM2_ARRL = 0xFF;
	TIM2_PSCR = 0x07; 

	TIM2_CCMR1 |= 0x60;// PWM1 mode on CH1
	TIM2_CCMR2 |= 0x60;// PWM1 mode on CH2
	TIM2_CCER1 |= 0x11; // Enable TIM2_CH1 and TIM2_CH2
	
	TIM2_OISR |= 0x00;
	
	TIM2_CCR1H = 0x00;
	TIM2_CCR1L = 0x00;// A phase 
	TIM2_CCR2H = 0x00;
	TIM2_CCR2L = 0x00;// B phase
	
	// Enable TIM2 peripheral
	TIM2_BKR |= 0x80;

	//Enable TIM2
	TIM2_CR1 = 0x61;
	
	// Initial sine angle 
	sinA = 0;// 0 degree
	sinB = 127;// 180 degree
	
	__asm__("rim");
    while (1) {
		vibrateMs(35);
		delay_ms(500);
    }
}
