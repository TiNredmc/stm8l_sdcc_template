#include "usart.h"
#include "stm8l.h"

void usart_init(uint16_t BAUDRATE) {
/*Set clock divider to 1, because the SDCC always set to 8 */
CLK_CKDIVR = 0x00;
/*Enable clock for USART1*/
CLK_PCKENR1 |= 0x10; // enable USART1 clock.
/* reset the baudrate setting */ 	
	USART1_BRR2 = 0X0F; 
	USART1_BRR2 = 0xF0;
	USART1_BRR1 = 0xFF;
/* set the baudrate */
  uint32_t BaudRate_Mantissa = (uint32_t)(F_CPU / BAUDRATE);
  if (BaudRate_Mantissa = 0x8A)
	  BaudRate_Mantissa = 0x8B;// round the baud rate USART_DIV number (for 115200).
  if (BaudRate_Mantissa = 0x22)
	  BaudRate_Mantissa = 0x23;// round the baud rate USART_DIV number (for 460800).
  
  USART1_BRR2 = (uint8_t)(BaudRate_Mantissa & 0x0F) | (uint8_t)(BaudRate_Mantissa & 0xF000);
  USART1_BRR1 = (uint8_t)((BaudRate_Mantissa >> 4) & 0xFF);
    /* enable transmitter and receiver */
    USART1_CR2 = 0x00; //Disable it first then re-enable
    USART1_CR2 = 0x04 | 0x08;// add 1 to bit number 2 and 3 for enabling the Tx and Rx
}

void usart_write(uint8_t data) {
    USART1_DR = data;
    while (!(USART1_SR & (1 << USART1_SR_TC)));
}

uint8_t usart_read() {
    while (!(USART1_SR & (1 << USART1_SR_RXNE)));
    return USART1_DR;
}
