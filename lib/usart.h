#ifndef USART_H
#define USART_H

#include <stdint.h>

#ifndef BAUDRATE
#define BAUDRATE 115200
#endif

#ifndef F_CPU // f not define F_CPU
#warning "F_CPU not defined, using 2MHz by default"
#define F_CPU 2000000UL
#endif

/**
 * Initialize USART1.
 * Mode: 8-N-1, flow-control: none.
 *
 * Default is 
 * PC3 -> TX
 * PC2 -> RX
 * But can be changed using syscfg 
 */
void usart_init();

void usart_write(uint8_t data);

uint8_t usart_read();

#endif /* UART_H */
