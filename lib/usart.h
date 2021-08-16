#ifndef USART_H
#define USART_H

#include <stdint.h>


/**
 * Initialize USART1.
 * Mode: 8-N-1, flow-control: none.
 *
 * Default is 
 * PC3 -> TX
 * PC2 -> RX
 * But can be changed using syscfg 
 */
void usart_init(uint32_t BAUDRATE);

void usart_IrDA_init();

void usart_write(uint8_t data);

uint8_t usart_read();

#endif /* UART_H */
