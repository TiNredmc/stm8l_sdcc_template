#include <stdint.h>
#include <stm8l.h>
#include <delay.h>
#include <stdio.h>
#include <string.h>
#include <eeprom.h>
#include <usart.h>

unsigned char txtBuf[4] = "TLHX";
unsigned char readBuf[4] = {0};

uint16_t REMAP_Pin = 0x011C; 

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
	SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); //remap the non-exit pin of Tx and Rx of the UFQFPN20 package to the exit one.
	SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);

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
