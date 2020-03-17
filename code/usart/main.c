/* Example for the USART communication */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
#include <usart.h>


uint16_t REMAP_Pin = 0x011C; 

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}


void main() {
usart_init(9600); // usart using baudrate at 9600
SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); //remap the non-exit pin of Tx and Rx of the UFQFPN20 package to the exit one.
SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);

    while (1) {
printf("%s","TinLethax!\n");
delay_ms(1000);
    }
}
