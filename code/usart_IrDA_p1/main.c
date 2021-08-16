/* Example for the USART IrDA communication (Sender). 
 * Coded by TinLethax 2021/08/16 +7
 */
 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
#include <usart.h>

uint16_t REMAP_Pin = 0x011C; 
char txbuf[]="TinLethax!";
char rxbuf[10];

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

//self implemented scanf 
void scanf(char *t, size_t bsize){
	while(bsize--)
		*t++ = usart_read();
}

void main() {
	usart_init(9600); // usart using baudrate at 9600
	usart_IrDA_init();// using IrDA mode.
	SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); //remap the non-exit pin of Tx and Rx of the UFQFPN20 package to the exit one.
	SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);

	PB_DDR |= 1 << 4;// PB4 as LED status 
	PB_CR1 |= 1 << 4;

	delay_ms(100);
	printf("%s",txbuf);
	delay_ms(100);
	scanf(rxbuf,10);
	
	if (memcmp(txbuf, rxbuf, 10)){
		while(1){
			PB_ODR |= 1 << 4;
			delay_ms(500);
			PB_ODR &= ~(1 <<4);
			delay_ms(500);
		}

	}else{
		PB_ODR |= 1 << 4;
	}
    while (1) {

    }
}
