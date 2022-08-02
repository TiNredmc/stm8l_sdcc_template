/* Flash read / write example 
 * Coded by TinLethax
 */

#include <stdint.h>
#include <stm8l.h>
#include <delay.h>
#include <stdio.h>
#include <string.h>
#include <flash.h>
#include <usart.h>

unsigned char txtBuf[9] = "LoveCat69";
unsigned char txtBuf2[9] = "TinLethax";
unsigned char readBuf[9] = {0};

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
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).

	printf(" %cFlash write begin!\n",12);
	flash_unlock();// Permanently unlock the flash mem.

	flash_write(txtBuf, 0x969F - 0x8000, 9);// Write from txtBuf at offset = 0x969F (the actual flash off is 0x8000)

	printf("Flash wrote!\n");
	printf("The read out is : ");	
	
	flash_read(readBuf, 0x969F - 0x8000, 9);// Read from offset = 0x969F
	printf("%.9s\n", readBuf);

	flash_write(txtBuf2, 0x969F - 0x8000, 9);// Write from txtBuf at offset = 0x969F (the actual flash off is 0x8000)

	printf("Flash wrote!\n");
	printf("The read out is : ");	
	
	flash_read(readBuf, 0x969F - 0x8000, 9);// Read from offset = 0x969F
	printf("%.9s\n", readBuf);
	
	printf("DONE");
	
    while (1) {

    }
}
