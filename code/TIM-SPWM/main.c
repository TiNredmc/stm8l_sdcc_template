/* 3 phase SPWM example on STM8L
 * Coded by TinLethax 2022/12/15 +7
 */

#include <stdint.h>
#include <stm8l.h>
#include <delay.h>

#define PWM_U	0 // U phase PWM PB0 (TIM2_CH1)
#define PWM_V	1 // V phase PWM PB1 (TIM3_CH1)
#define PWM_W	2 // W phase PWM PB2 (TIM2_CH2)

// Space Vector Sine
// Sine with 3rd Harmonic injected LUT
// sin(2piF) + 0.33sin(2piF * 2)
// sin(2*pi*t/T) + 0.33*sin(6*pi*t/T)
const uint8_t lut[256] = {
127,133,139,146,152,158,164,169,175,181,186,191,196,
201,206,210,214,218,222,225,229,231,234,237,239,241,
242,243,245,245,246,246,246,246,246,246,245,244,243,
242,241,239,238,236,235,233,232,230,228,227,225,223,
222,221,219,218,217,216,215,214,213,213,212,212,212,
212,212,213,213,214,215,216,217,218,219,221,222,223,
225,227,228,230,232,233,235,236,238,239,241,242,243,
244,245,246,246,246,246,246,246,245,245,243,242,241,
239,237,234,231,229,225,222,218,214,210,206,201,196,
191,186,181,175,169,164,158,152,146,139,133,127,121,
115,108,102, 96, 90, 85, 79, 73, 68, 63, 58, 53, 48,
 44, 40, 36, 32, 29, 25, 23, 20, 17, 15, 13, 12, 11,
  9,  9,  8,  8,  8,  8,  8,  8,  9, 10, 11, 12, 13,
 15, 16, 18, 19, 21, 22, 24, 26, 27, 29, 31, 32, 33,
 35, 36, 37, 38, 39, 40, 41, 41, 42, 42, 42, 42, 42,
 41, 41, 40, 39, 38, 37, 36, 35, 33, 32, 31, 29, 27,
 26, 24, 22, 21, 19, 18, 16, 15, 13, 12, 11, 10,  9,
  8,  8,  8,  8,  8,  8,  9,  9, 11, 12, 13, 15, 17,
 20, 23, 25, 29, 32, 36, 40, 44, 48, 53, 58, 63, 68,
 73, 79, 85, 90, 96,102,108,115,121 };

volatile uint8_t sinU = 0;
volatile uint8_t sinV = 0;
volatile uint8_t sinW = 0;

void TIM4isr(void) __interrupt(25){
	TIM3_CCR1L = lut[sinV];
	TIM2_CCR2L = lut[sinW];
	TIM2_CCR1L = lut[sinU];
	sinU++;
	sinV++;
	sinW++;
	
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
	
	if((16000000 / HZ) < 256){
		TIM4_ARR = (uint8_t)((16000000 / HZ) - 1);
		TIM4_PSCR = 0x00;
	}else if ((8000000 / HZ) < 256){
		TIM4_ARR = (uint8_t)((8000000 / HZ) - 1);
		TIM4_PSCR = 0x01;
	}else if ((4000000 / HZ) < 256){
		TIM4_ARR = (uint8_t)((4000000 / HZ) - 1);
		TIM4_PSCR = 0x02;
	}else if ((2000000 / HZ) < 256){
		TIM4_ARR = (uint8_t)((2000000 / HZ) - 1);
		TIM4_PSCR = 0x03;
	}else if ((1000000 / HZ) < 256){
		TIM4_ARR = (uint8_t)((1000000 / HZ) - 1);
		TIM4_PSCR = 0x04;
	}else if ((500000 / HZ) < 256){
		TIM4_ARR = (uint8_t)((500000 / HZ) - 1);
		TIM4_PSCR = 0x05;
	}else if ((250000 / HZ) < 256){
		TIM4_ARR = (uint8_t)((250000 / HZ) - 1);
		TIM4_PSCR = 0x06;
	}else if ((125000 / HZ) < 256){
		TIM4_ARR = (uint8_t)((125000 / HZ) - 1);
		TIM4_PSCR = 0x07;
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

void main() {
	CLK_CKDIVR = 0x00;// 16MHz system clock
	CLK_PCKENR1 |= 0x07;// Enable TIM2 TIM3 and TIM4 peripheral clock
	
	PB_DDR |= (1 << PWM_U) | (1 << PWM_V) | (1 << PWM_W);// Set PWM pins as output
	PB_CR1 |= (1 << PWM_U) | (1 << PWM_V) | (1 << PWM_W);// Push pull mode
	PB_CR2 |= (1 << PWM_U) | (1 << PWM_V) | (1 << PWM_W);// with fast mode

	//Timer 2 and 3 init 
	
	// TIM2
	TIM2_ARRH = 0x00;
	TIM2_ARRL = 0xFF;
	TIM2_PSCR = 0x00; 

	TIM2_CCMR1 |= 0x60;// PWM1 mode on CH1
	TIM2_CCMR2 |= 0x60;// PWM1 mode on CH2
	TIM2_CCER1 |= 0x11; // Enable TIM2_CH1 and TIM2_CH2
	
	TIM2_OISR |= 0x00;
	
	TIM2_CCR1H = 0x00;
	TIM2_CCR1L = 0x00;// U phase 
	TIM2_CCR2H = 0x00;
	TIM2_CCR2L = 0x00;// W phase
	
	// Enable TIM2 peripheral
	TIM2_BKR |= 0x80;

	// TIM3
	TIM3_ARRH = 0x00;
	TIM3_ARRL = 0xFF;
	TIM3_PSCR = 0x00; 

	TIM3_CCMR1 |= 0x60;// PWM1 mode 
	TIM3_CCER1 |= 0x01; // Enable TIM3_CH1 
	
	TIM3_OISR |= 0x00;
	
	TIM3_CCR1H =  0x00;
	TIM3_CCR1L =  0x00;// V phase
	// Enable TIM3 peripheral
	TIM3_BKR |= 0x80;

	//Enable TIM2
	TIM3_CR1 = 0x61;
	//Enable TIM2
	TIM2_CR1 = 0x61;
	
	
	// Initial sine angle 
	sinU = 0;
	sinV = 85;
	sinW = 170;
	
	initTim4(10000);// Init 500Hz timer 
	__asm__("rim");
	
    while (1) {
    }
}
