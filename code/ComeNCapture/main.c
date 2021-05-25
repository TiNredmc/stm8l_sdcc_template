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
uint8_t CapParam[3]={0x00,0x00,0x00};// capture parameter {Start or Stop (0x01or0x00), Arr value, Prescaler Value}
char bootText[] = {" ComeNCapture 1.0\n"};
/////////////////////////////////////////////////////////////////////////
///////// Normal I/O stuffs /////////////////////////////////////////////
/* Initialize all PortB from 0-7*/
void PBinit(){
	PB_DDR = 0; // set all pin at input
	PB_CR1 = 0; // floating input
	PB_CR2 = 0; // No interrupt
}

/////////////////////////////////////////////////////////////////////////
///////// Timer stuffs //////////////////////////////////////////////////
/* Initialize the TIM4 for timing source for the Logic capture */
void TIM4init(uint8_t arrVal, uint8_t pscrVal){
	CLK_PCKENR1 |= (uint8_t)(1 << 0x02);// enable the TIM4 clock
	/* TIM4_TimeBaseInit(pscrVal, arrVal)*/
	TIM4_ARR = arrVal; //set prescaler period
	TIM4_PSCR = pscrVal;// set prescaler 
	TIM4_EGR = 0x01;// event source update 
	/*TIM4_cmd(ENABLE)*/
	TIM4_CR1 |= 0x01 ;
	/*TIM4_ClearFlag()*/
	TIM4_SR = (uint8_t)~0x01;
	/*TIM4_ITConfig(TIM4_IT_Update, ENABLE)*/	
	TIM4_IER |= 0x01;// make sure that I enable interrupt for the TIM4
	/*TIM4_cmd(ENABLE)*/
	TIM4_CR1 |= 0x01 ;
}

/* Interrupt handler for TIM4 */
void TIM4isr(void) __interrupt(25){// the interrupt vector is 25 (from the STM8L151F36U datasheet page 49)
	USART1_DR = (uint8_t)PB_IDR;
    	while (!(USART1_SR & (1 << USART1_SR_TC)));
	/*TIM4_ITClearPendingBit()*/
	TIM4_SR = (uint8_t)~0x01;
}

/////////////////////////////////////////////////////////////////////////
///////// USART interrupt stuffs ////////////////////////////////////////
/* Enable Interrupt for usart rx, This require when PC wants to iterface with MCU*/
void usart_it(){
	USART1_CR2 |= 1 << (0x05);//(0x55 & 0x0F)k
}

/* When PC want to interface with the MCU for the capture request, 
It pause the TIM4 interrupt and instead receive the data packet */
void USARTisr(void) __interrupt(28){// the interrupt vector is 28 (from the STM8L151F36U datasheet page 49)
	/*TIM4_cmd(DISABLE)*/
	TIM4_CR1 &= ~0x01;// Disable the counter of TIM4
	CapParam[0] = USART1_DR;// The Start Stop indicator
	while (!(USART1_SR & (1 << USART1_SR_RXNE)));
	CapParam[1] = USART1_DR;// Arr value
	while (!(USART1_SR & (1 << USART1_SR_RXNE)));
	CapParam[2] = USART1_DR;// Prescaler value

	if(CapParam[0]){// We got Start (0x01) then
	TIM4init(CapParam[1], CapParam[2]);// Setup the timer again
	}else{// but If we got Stop (0x00) then
	// Reset the timer
	TIM4_CR1   = 0x00;
	TIM4_IER   = 0x00;
	TIM4_CNTR   = 0x00;
	TIM4_PSCR  = 0x00;
	TIM4_ARR   = 0xFF;
	TIM4_SR   = 0x00;
	}
}

void main() {
	CLK_CKDIVR = 0x00;// set clock divider to 1, So we get the 16MHz for high TX rate
	usart_init(38400);// use baud rate of 38400
	usart_it();// Enable USART Interrupt
	SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); //remap the non-exit pin of Tx and Rx of the UFQFPN20 package to the exit one.
	SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);
	PBinit();// Init the pin real quick
	__asm__("rim");// start interrupt system
	delay_ms(250);// wait for everything stable
	for(int i=0;i<18;i++){
	USART1_DR = bootText[i];
    	while (!(USART1_SR & (1 << USART1_SR_TC)));
	}
	while (1) {
	__asm__("WFI");// Wait for interrupt, Make the device sleep zZzZzZ
	}
}
