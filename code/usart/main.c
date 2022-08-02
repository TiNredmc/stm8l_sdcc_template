/* Example for the USART communication 
 * Coded by TinLethax
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
#include <usart.h>

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}


void main() {
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).

    while (1) {
		printf("%s","TinLethax!\n");
		delay_ms(1000);
    }
}
