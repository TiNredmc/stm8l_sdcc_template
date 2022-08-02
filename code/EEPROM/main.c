/* EEPROM read / write example 
 * Coded by TinLethax
 */

#include <stdint.h>
#include <stm8l.h>
#include <delay.h>
#include <stdio.h>
#include <string.h>
#include <eeprom.h>
#include <usart.h>

unsigned char txtBuf[4] = "TLHX";
unsigned char readBuf[4] = {0};

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

void main() {
	CLK_CKDIVR = 0x00;
	usart_init(9600); // usart using baudrate at 9600
	SSYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).

	printf(" %cFlash write begin!\n",12);

	eeprom_write(txtBuf, 0, 4);// Write from txtBuf at offset = 0

	printf("Flash wrote!\n");
	printf("The read out is : ");	
	
	eeprom_read(readBuf, 0, 4);// Read from offset = 0
	printf("%.4s", readBuf);
	
	printf("\nDONE");
	
    while (1) {

    }
}
