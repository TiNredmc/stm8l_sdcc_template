/* Unique ID read out example code
 * Coded by TinLethax
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
#include <dev_id.h>
#include <usart.h>


int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

void main() {
	CLK_CKDIVR = 0x00;// make sure that we don't divide our clock.
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	delay_ms(1000); //wait a sec
	usart_init(9600); // usart using baudrate at 9600
	int i = 0;
	
	for (i; i < 13; i++){
	printf("%x",req_dev_id(i));
	printf(" ");
	}
	
	
	while (1) {

	}
}
