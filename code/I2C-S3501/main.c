// Synaptics S3501 Touch controller driver code.
// Ported by TinLethax 2020/07/26 +7
#include <stm8l.h>
#include <i2c.h>
#include <usart.h>
#include <delay.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* I2C 7bit address of S3501 */
#define S3501_ADDR	0x20

/* Register map*/
#define RMI4_F01	0x01 // Device control
#define RMI4_F12	0x12 // 2D Touch sensor (newer gen of Synaptics use this instead of F11).
#define RMI4_F1A	0x1A // Button 
#define RMI4_F54	0x54 // Test report
#define RMI4_F55	0x55 // TX, RX configuration

#define KEEP_INFO // Keep query info. Undefine this can save memory

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}


// Address of each function.
//Page 0
static uint8_t F01_addr = 0;
static uint8_t F12_addr = 0;

// Page 2
static uint8_t F1A_addr = 0;

// 2D sensor params of F12
static uint8_t F12_maxfinger = 0;// store maximum finger supported by the chip
static uint8_t F12_report_addr = 0;// Store ACTUAL HID data report address of F12.

// 88 bytes report according to RMI4 linux.
static uint8_t F12_report[88];
// Finger 1
// F12_report[0] -> Finger 1 present (1/0)
// F12_report[1] -> ABS_X LSB
// F12_report[2] -> ABS_X MSB
// F12_report[3] -> ABS_Y LSB
// F12_report[4] -> ABS_Y MSB
// F12_report[5] -> pressure 
// F12_report[6] -> ABS_MT_TOUCH_MINOR
// F12_report[7] -> ABS_MT_TOUCH_MAJOR

// Finger 2
// F12_report[8] -> Finger 2 present (1/0)
// F12_report[9] -> ABS_X LSB
// F12_report[10] -> ABS_X MSB
// F12_report[11] -> ABS_Y LSB
// F12_report[12] -> ABS_Y MSB
// F12_report[13] -> pressure 
// F12_report[14] -> ABS_MT_TOUCH_MINOR
// F12_report[15] -> ABS_MT_TOUCH_MAJOR

// F12_report[16] to F12_report[85] are all at 0x00

// F12_report[86] -> BTN_TOUCH single finger == 1, 2 fingers == 3
// F12_report[87] is 0x00

/* F12 Query, Yet to decode this
0x11 
0x03 0x1D 0xFF 0x07 0x01 0x01 0x01 0x01 0x03 
0x03 0x1D 0xFF 0x0F 0x01 0x01 0x05 0x03 0x24 0x80 
0xBE 0xD2 0xEE 0x07 0x01 
0x01 0x03 0x03 0x09 0x96 
0x80 0x02 0x01 0x01 0x05 
0x24 0x00 0x9F 0xD4 0x7D 
0x0F 0x1F 0x13 0x47 0x07 
0x0F 0x10 0x1F 0x04 0x80 
0x08 0x07 0x83 0x02 0x1D 
0x3B 0x03 0x03 0x01 0x01 
0x03 0x07 0x02 0x01 0x01 
0x01 0x09 0x0F 0x01 0x01 
0x10 0xFF 0x01 0x10 0xFF 
0x01 0x03 0x09 0x16 0x80 
0x50 0xFF 0x07 0x01 0x01 
0x05 0x03 0x02 0x01 
*/

// I2C probe the chip
void s3501_probe(){
	i2c_start();
	I2C1_DR = (S3501_ADDR << 1) & 0xFE;// send address to I2C.
	delay_ms(2);
	if(I2C1_SR1 & 0x02){// if address found, shout out the address.
		printf("S3501/S3508 found!\n");
	}
	i2c_stop();
}

// Set page of S3501
void s3501_setPage(uint8_t page_num){
	i2c_start();// Generate Start condition
	
	i2c_write_addr(S3501_ADDR);// Request write to S3501.
	
	i2c_write(0xFF);// Write lower half byte which is dummy
	i2c_write(page_num);// Write upper half which is page number.

	i2c_stop();// Generate stop condition
}

// Read data from S3501
void s3501_read(uint8_t addr, uint8_t *data, uint16_t len){
	
	i2c_start();// Generate start condition.
	
	i2c_write_addr(S3501_ADDR);// Request write to S3501.
	i2c_write(addr);// Write address to S3501
	
	i2c_start();// Regenerate start condition
	
	i2c_read_addr(S3501_ADDR);// Request read from S3501.
	i2c_readPtr(data, len);
	
	i2c_stop();// Generate stop condition
	
}

// Write data to S3501
void s3501_write(uint8_t addr, uint8_t *data, uint8_t len){
	
	i2c_start();// Generate start condition.
	
	i2c_write_addr(S3501_ADDR);// Request write to S3501.
	i2c_write(addr);// Write address to S3501
	i2c_writePtr(data, len);// Write the rest of data to S3501.
	
	i2c_stop();// Generate stop condition
}

// Query process. Call once every power up or reset to locate all RMI4 function address.
// Static uint8_t variables F01_addr and F12_addr for further use of page address of F01 and F12.
// return 0 when no error.
uint8_t s3501_query(){
	uint8_t s3501_desc[6] = {0};
	
	// Scan for base address of each function. 
	for(uint8_t p = 0; p < 4; p++){
		s3501_setPage(p);// set page 
		for(uint8_t i = 0xE9; i > 0xD0; i-=6){
			
			s3501_read(i, s3501_desc, 6);
			if(s3501_desc[5] != 0){// check the function number (type).
			printf("Page %d Table %02X\n",p , i);
			printf("Dump 0x%02X \n", s3501_desc[5]);
			}
			switch(s3501_desc[5]){
			case 0x01: // Function 0x01, RMI device control.
				F01_addr = i;// save F01 address. 	
				break;
				
			case 0x12: // Function 0x12, 2-D sensor.
				F12_addr = i;// save F12 address.
				break;
			
			case 0x1A: // Function 0x1A
				F1A_addr = i;
				break;
				
			default:
				break;
			
			}
		}
	}
	// Sanity check
	if(F01_addr == 0)
		return 3;
	
	if(F12_addr == 0)
		return 4;
	
#ifdef KEEP_INFO
	// Keep query info of F01 
	static uint8_t F01_data[21] = {0};
	
	s3501_setPage(0);// Set to Page 0
	s3501_read(F01_addr, F01_data, 6);// Retrieve Description Table of F01
	s3501_read(F01_data[0], F01_data, 21);// Read from Query register of F01
	
	printf("F01 Query:\n");
	
	for(uint8_t i = 0; i < 21; i ++){
		printf("0x%02X ", F01_data[i]);
		if(i%5 == 4){
			printf("\n");
		}
	}
	usart_write('\n');
	// F01_data[0] -> manufacturer ID (always 1).
	// F01_data[1] -> product porperty
	// F01_data[2] F01_data[3] -> 7bit (0:6) Product info
	// F01_data[4] -> Date code year 5bit (0:4)
	// F01_data[5] -> Date code Month 4bit (0:3)
	// F01_data[6] -> Date code day 5bit (0:4)
	// F01_data[7] F01_data[8] -> tester id 14bit 
	// F01_data[9] F01_data[10] -> Serial number 14bit (0:6 and 0:6 combined to 0:13)
	// F01_data[11] to F01_data[20] -> Product id in 7bit ASCII character 1 to 10
	
	
	if(F01_data[0] != 1){// check for manufacturer id (always 1)
		return 5;
	}
	
#endif
	
	// Get F12 / 2D touch property 
	uint8_t F12_query[89];
	s3501_read(F12_addr, F12_query, 6);// Retrieve Description Table of F12
	printf("F12 return : %02X %02X %02X %02X %02X %02X\n", F12_query[0], F12_query[1], F12_query[2], F12_query[3], F12_query[4], F12_query[5]);
	F12_report_addr = F12_query[3];// save the F12 data report address. This will be the actual address that we read from (mine is 0x06).
	s3501_read(F12_query[0], F12_query, 89);// Read from Query register of F12
	
	printf("F12 Query:\n");
	
	for(uint8_t j = 0; j < 89; j ++){
		printf("0x%02X ", F12_query[j]);
		if(j%5 == 4){
			printf("\n");
		}
		
	}
	usart_write('\n');
	
	switch(F12_query[0] & 0x07){// check bit 0-2 for maximum finger detection.
	case 0: // 1 finger
		F12_maxfinger = 1;
		break;
	
	case 1: // 2 fingers
		F12_maxfinger = 2;
		break;
		
	case 2: // 3 fingers
		F12_maxfinger = 3;
		break;
		
	case 3: // 4 fingers
		F12_maxfinger = 4;
		break;
		
	case 4: // 5 fingers 
		F12_maxfinger = 5;
		break;
		
	case 5: // 10 fingers
		F12_maxfinger = 10;
		break;
		
	default:
		F12_maxfinger = 1;
		break;
	}
	
	printf("Supported finger %d\n", F12_maxfinger);
	
	return 0;
	
}

// Get HID report data (X,Y coordinates and other stuffs).
// Assuming that it has 2 fingers report (I guess).
void s3501_HIDreport(){
	s3501_read(F12_report_addr, F12_report, 88);// Read from Data register of F12

	if(F12_report[0]){
	printf("Finger 1 :\n");
	printf("X: %d Y: %d\n", (F12_report[2] << 8) | F12_report[1], (F12_report[4] << 8) | F12_report[3]);
	printf("Pressure: %d\n", F12_report[5]);
	}
	
	if(F12_report[8]){
	printf("Finger 2 :\n");
	printf("X: %d Y: %d\n", (F12_report[10] << 8) | F12_report[9], (F12_report[12] << 8) | F12_report[11]);
	printf("Pressure: %d\n", F12_report[13]);
	}
}

void s3501_rst(){
	PC_DDR |= (1 << 5);
	PC_ODR |= (1 << 5);
	PC_ODR &= ~(1 << 5);
	delay_ms(500);
	PC_ODR |= (1 << 5);
}

void main(){
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);// clear terminal
	usart_write(0x0C);
	s3501_rst();
	i2c_init(0x69, I2C_100K);// Set I2C master address to 0x00, I2C speed to 100kHz.
	printf("S3501/S3508 on STM8L by TinLethax\n");
	delay_ms(100);
	uint8_t res = s3501_query();
	printf("ERROR CODE: %d\n", res);
	
	
	while(1){
		s3501_HIDreport();
		delay_ms(500);
	}
}

/* read/write process
- Set page with s3501_setPage
- read or write data with s3501_read or s3501_write
*/