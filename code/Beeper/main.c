// Beep using BEEP register
// coded by TinLethax 2022/08/01 +7

#include <stdint.h>
#include <stm8l.h>
#include <delay.h>

uint8_t div_cnt = 0;
uint8_t updown = 1;
void main() {
	CLK_CKDIVR = 0x00;// CPU running at 16Mhz
	delay_ms(500);// We have a time before our SWIM (PA0) is taken away by BEEP.
	
	PA_DDR |= (1 << 0);
	PA_CR1 |= (1 << 0);
	PA_CR2 |= (1 << 0);
	
	CLK_ICKCR=(1 << CLK_ICKCR_LSION);// turn LowSpeedInternal Clock
	while (!(CLK_ICKCR & (1 << CLK_ICKCR_LSIRDY)));  // enable LSI oscillator.
	
	CLK_PCKENR1 |= (1 << 6);// enable clock source for BEEP.
	
	CLK_CBEEPR |= (1 << 1);// Use LSI as clock source for Beep.
	while(CLK_CBEEPR & 0x01);// wait until beep clock selection is done.
	
	BEEP_CSR1 &= ~(1 << 0);// We don't need output for measurement.
	BEEP_CSR2 = 0x00;
	BEEP_CSR2 |= (1 << 7);
	BEEP_CSR2 |= (1 << 5);//toggle Beep
    while (1) {
		if(updown){
			div_cnt++;
			if(div_cnt == 0x1E)
				updown = 0;
		}else{
			div_cnt--;
			if(div_cnt == 0)
				updown = 1;
		}
		
		BEEP_CSR2 &= 0xE0;
		BEEP_CSR2 |= div_cnt;
        delay_ms(50);
    }
}
