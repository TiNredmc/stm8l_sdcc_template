// Atmel maXTouch MXT112S example code with STM8L
// Ported by TinLethax 2022/08/11 +7
#include <stm8l.h>
#include <i2c.h>
#include <usart.h>
#include <delay.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* I2C 7bit address of MXT112S */
#define MXT_ADDR	0x4B

#define MXT_ID_BLK_SIZ	7// Query ID size of 7 bytes

/* Register map*/


int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

// Save current page
uint8_t current_page = 0;

// Number of elements in the Object Table
uint8_t num_obj = 0;

// Set page of MXT112S
void mxt_setPage(uint8_t page_num){
	if(page_num != current_page){
	current_page = page_num;
	i2c_start();// Generate Start condition.
	
	i2c_write_addr(MXT_ADDR);// Request write to MXT112S.
	
	i2c_write(0xFF);// Write lower half byte which is dummy.
	i2c_write(page_num);// Write upper half which is page number.

	i2c_stop();// Generate stop condition.
	}
}

// Read data from MXT112S
void mxt_read(uint16_t addr, uint8_t *data, uint16_t len){
	
	i2c_start();// Generate start condition.
	
	i2c_write_addr(MXT_ADDR);// Request write to MXT112S.
	i2c_write((uint8_t)(addr << 8));// Write LSB byte address to MXT112S.
	i2c_write((uint8_t)addr);// Write MSB byte address to MXT112S.
	
	i2c_start();// Regenerate start condition.
	
	i2c_read_addr(MXT_ADDR);// Request read from MXT112S.
	i2c_readPtr(data, len);
	
	i2c_stop();// Generate stop condition.
	
}

// Write data to MXT112S
void mxt_write(uint16_t addr, uint8_t *data, uint8_t len){
	
	i2c_start();// Generate start condition.
	
	i2c_write_addr(MXT_ADDR);// Request write to MXT112S.
	i2c_write((uint8_t)(addr << 8));// Write LSB byte address to MXT112S.
	i2c_write((uint8_t)addr);// Write MSB byte address to MXT112S.
	
	i2c_writePtr(data, len);// Write the rest of data to MXT112S.
	
	i2c_stop();// Generate stop condition
}

// Query basic informations of MXT112S
void mxt_identify(){
	uint8_t buf[MXT_ID_BLK_SIZ] = {0};
	
	mxt_read(0x0000, buf, MXT_ID_BLK_SIZ);
	
	num_obj = buf[6];// save the object table elements count.
	
	printf("Family ID 0x%02X\n", buf[0]);
	printf("Variant ID 0x%02X\n", buf[1]);
	printf("Version %d.%d\n", (uint8_t)((buf[2]&0xF0) << 4), buf[2] & 0x0F);
	printf("Build number 0x%02X\n", buf[3]);
	printf("X channels %d\n", buf[4]);// Expecting 14
	printf("Y channels %d\n", buf[5]);// Expecting 8
	printf("OJB table elements count %d\n", buf[6]);
	
}

void main(){
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);// clear terminal
	usart_write(0x0C);
	i2c_init(0x69, I2C_100K);// Set I2C master address to 0x69, I2C speed to 100kHz.
	printf("MXT112S on STM8L by TinLethax\n");
	delay_ms(100);

	
	while(1){// Polling instead of interrupt (Slow down enought for the readable serial print).

		delay_ms(200);
	}
}

/* read/write process
- Set page with s3501_setPage
- read or write data with s3501_read or s3501_write
*/