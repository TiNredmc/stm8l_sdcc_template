/* ADC with DMA example.
 * Coded by TinLethax 2021/08/? +7
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stm8l.h>
#include <usart.h>
#include <delay.h>

uint16_t REMAP_Pin = 0x011C; 

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

static uint16_t JoyXY[2];// array to store the ADC read
#define buff_addr ((uint16_t)(&JoyXY))

void DMA1_irq(void) __interrupt(2){

	DMA1_C0SPR &= ~(1 << 1);// clear pending bit (TEIF).
}

void ADC_init(){
	// setup ADC system
	CLK_PCKENR2 |= 0x01;// enable ADC1 clock
	ADC1_CR1 |= 0x04;// 12bit continuous conversion mode 
	ADC1_CR2 |= 0x80;// Set prescaler divided by 2 
	
	// setup sampling times for channel 0-23 (actaully use channel 17 and 16, PB1 and PB2).
	ADC1_CR2 |= 0x07;
	
	// enable ADC1 
	ADC1_CR1 |= 0x01;
	
	// enable 16 and 17 channel.
	ADC1_SQR2 |= 0x03;
	
	// Start conversion
	ADC1_CR1 |= 0x02;
}

void DMA_init(){
	CLK_PCKENR2 |= 0x10;// enable DMA1 clock
	
	SYSCFG_RMPCR1 |= 0x00; // remap ADC1 to DMA1 Channel 0 
	
	DMA1_C0CR |= 0x00 | 0x10 | 0x20;// DMA Peripheral to Mem, Circular mode, memory increase mode.
	
	DMA1_C0SPR |= 0x20 | 0x08;// set priority to HIGH, memory type is 16bit data.
	
	DMA1_C0NDTR |= 0x02;// Buffer size is 2 (2x16bit).
	
	// Pheriperal address of ADC1_DRH, DMA1 will automatically increase memory counter to next address which is ADC1_DRL.
	DMA1_C0PARH = 0x53;
	DMA1_C0PARL = 0x44;
	
	// Memory address of JoyXY array storing ADC value.
	DMA1_C0M0ARH = buff_addr >> 8;
	DMA1_C0M0ARL = (uint8_t)buff_addr;
	
	
	// Enable DMA1 Channel 0
	DMA1_C0CR |= 0x01;
	
	// Enable Transfer complete interrupt
	DMA1_C0CR |= 0x02;
	
	// Enable Global CMD
	DMA1_GCSR |= 0x01;
}


void main() {
	CLK_CKDIVR = 0x00;// Full 16Mhz, no clock divider
usart_init(9600); // usart using baudrate at 9600
SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); //remap the non-exit pin of Tx and Rx of the UFQFPN20 package to the exit one.
SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);

	ADC_init();
	DMA_init();
	ADC1_SQR1 &= ~(1 << 7);// enable ADC's DMA
	__asm__("rim");
    while (1) {
	printf("X: %u, Y: %u\n", JoyXY[0], JoyXY[1]);// X axis is a report of channel 16 (PB2), Y axis is a report of channel 17 (PB1).
	delay_ms(500);
    }
}
