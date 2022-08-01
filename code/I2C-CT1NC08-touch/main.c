// Chemtronics CT1NC08 capacitive touch sensor with STM8
// Coded by TinLethax 2022/08/01 +7
#include <stm8l.h>
#include <i2c.h>
#include <usart.h>
#include <delay.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* I2C 7bit address of CT1NC08 */
#define I2C_ADDR	0x54

/* R/W reg */
#define CMD_EEPSET	0x00 // EEPROM and sysetem control related.
#define CMD_SOFRST	0x01 // Soft reset.
#define CMD_ICPSEL	0x02 // Parallel interface selection.
#define CMD_KEYENA	0x03 // Touch key enable.
#define CMD_NISKEY	0x04 // Noise Key setting.
#define CMD_SLEEPS	0x05 // Sleep mode selection.
#define CMD_I2CDID	0x06 // Set I2C device ID, Default ID is 0x54.
#define CMD_LEDCTL	0x07 // LED control mode.
#define CMD_TCHCNT	0x09 // Channel count enable time.
#define CMD_TCHDRT	0x10 // Touch duration time.
#define CMD_REFGRD	0x0B // Reference Update Guard (Threshold) setting.
#define CMD_THRGLB	0x0C // Global touch sensitivity setting.

// use CMD_THRLOC_BASE + Touch channel number (0-7) to set sensitivity of each channel.
#define CMD_THRLOC_BASE	0x0D // Local touch sensitivity setting.

#define CMD_REFGEN	0x1A // Reference Count Generation for touch on process.
#define CMD_CONTCH	0x23 // Concurrent touch handling.
/* end R/W reg */

/* R/O reg*/
#define CMD_TCHOUT	0x2A // Touch output data.
#define CMD_SCDLSB	0x2B // Scan count data LSB.
#define CMD_SCDMSB	0x2C // Scan count data MSB.
/* end R/O reg*/


int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

// Read data from CT1NC08
void CT_read(uint8_t addr, uint8_t *data, uint16_t len){
	
	i2c_start();// Generate start condition.
	
	i2c_write_addr(S3501_ADDR);// Request write to CT1NC08.
	i2c_write(addr);// Write address to CT1NC08
	
	i2c_start();// Regenerate start condition
	
	i2c_read_addr(S3501_ADDR);// Request read from CT1NC08.
	i2c_readPtr(data, len);
	
	i2c_stop();// Generate stop condition
	
}

// Write data to CT1NC08
void CT_write(uint8_t addr, uint8_t *data, uint8_t len){
	
	i2c_start();// Generate start condition.
	
	i2c_write_addr(S3501_ADDR);// Request write to CT1NC08.
	i2c_write(addr);// Write address to CT1NC08
	i2c_writePtr(data, len);// Write the rest of data to CT1NC08.
	
	i2c_stop();// Generate stop condition
}

// Read touch button status
void CT_touchRead(){
	uint8_t BTN_RD = 0;
	CT_read(CMD_TCHOUT, &BTN_RD, 1);
	
	printf("Button read: 0x%02X\n", BTN_RD);
}

void main(){
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);// clear terminal
	usart_write(0x0C);
	i2c_init(0x69, I2C_100K);// Set I2C master address to 0x00, I2C speed to 100kHz.
	printf("CT1NC08 Demo on STM8L by TinLethax\n");
	delay_ms(100);

	
	while(1){
		CT_touchRead();
		delay_ms(200);
	}
}

/* read/write process
- Set page with s3501_setPage
- read or write data with s3501_read or s3501_write
*/