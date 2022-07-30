/* I2C scanner code 
 * Coded by TinLethax 2022/07/29 +7 
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
#include <usart.h>
#include <i2c.h>

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
	usart_write(0x0C);// clear terminal
	usart_write(0x0C);
	i2c_init(0x00, I2C_100K);// Set I2C master address to 0x00, I2C speed to 100kHz.
	printf("I2C scanner on STM8L by TinLethax\n");
	
    while (1) {
		for(uint8_t addr = 1; addr < 127; addr++){
			i2c_start();
			I2C1_DR = (addr << 1) & 0xFE;// send address to I2C.
			delay_ms(2);
			if(I2C1_SR1 & 0x02){// if address found, shout out the address.
				printf("Found : 0x%02x\n", addr);
			}
			i2c_stop();
		}
		printf("Done! \n");
		delay_ms(1000);
    }
}
