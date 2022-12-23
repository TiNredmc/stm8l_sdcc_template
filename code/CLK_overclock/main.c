// Attempt to overclock stm8l to 20MHz
// Coded by TinLethax 2022/12/23 +7

// Connect 20Mhz output from Xtal osc or Function gen to pin PA2

#include <stm8l.h>

// Avoid using delay_ms and delay_us (for now)

void main(){
	CLK_CKDIVR = 0x00;
	// Set PB4 to high when clock setup is done.
	PB_DDR |= (1 << 4);
	PB_CR1 |= (1 << 4);
	PB_ODR |= (1 << 4);
	
	CLK_ECKCR |= 0x11;// Enable external HSE clock to bypass the oscillator.
	
	while(!(CLK_ECKCR & 0x02));// Wait until HSE is ready

	PC_DDR |= (1 << 4);// PC4 as output
	PC_CR1 |= (1 << 4);// Push-pull output
	PC_CR2 |= (1 << 4);// High speed mode
	
	CLK_CCOR = 0x48;// Set CCO clock source from HSE and divide the clock by 4 (we should get 5MHz)

	CLK_SWR = 0x04;// Select HSE as system clock

	CLK_SWCR |= 0x02;// Enable clock switch
	
	while(1){
	__asm__("bset 0x5005, #4");
	__asm__("bres 0x5005, #4");
	__asm__("bset 0x5005, #4");
	__asm__("bres 0x5005, #4");
	}
}