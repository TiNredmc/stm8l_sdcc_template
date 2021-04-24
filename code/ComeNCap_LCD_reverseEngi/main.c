/* ComeNCapture is a quick-n-dirty logic analyzer 
 * Coded By TinLethax 2020/05/23
 * Right now the RX part (pc -> MCU) uses the Interrupt method. In the future I will implement the DMA 
 * So I can keep the timer freely running
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
#include <usart.h>

const uint16_t REMAP_Pin = 0x011C; //constant value for sysconf to remap USART pin
char bootText[] = {" ComeNCapture LCD Reverse Engineering Edition\n\n"};
/////////////////////////////////////////////////////////////////////////
///////// Normal I/O stuffs /////////////////////////////////////////////
/* Initialize all PortB from 0-7*/
void PBinit(){
	PB_DDR = 0; // set all pin at input
	PB_CR1 = 0; // floating input
	PB_CR2 = 0; // No interrupt
}


void main() {
	CLK_CKDIVR = 0x00;// set clock divider to 1, So we get the 16MHz for high TX rate
	usart_init(38400);// use baud rate of 38400
	SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); //remap the non-exit pin of Tx and Rx of the UFQFPN20 package to the exit one.
	SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);
	PBinit();// Init the pin real quick
	delay_ms(250);// wait for everything stable

	for(int i=0;i<47;i++){
	USART1_DR = bootText[i];
    	while (!(USART1_SR & (1 << USART1_SR_TC))); // wait for stable USART 
	}

	while (1) {
		while(!CS_Pin){
		USART1_DR = PB_ODR;
    		while (!(USART1_SR & (1 << USART1_SR_TC))); // wait for stable USART 
		}
	}
}
