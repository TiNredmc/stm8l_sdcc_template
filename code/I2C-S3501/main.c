// Synaptics S3501 Touch controller driver code.
// https://hackaday.io/project/184529-reverse-engineering-bb-passport-keyboard
// Ported by TinLethax 2022/07/26 +7
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

//#define MORE_INFO // Keep query info. Undefine this can save memory

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

// Save current page
static uint8_t current_page = 0;

// Address of each function.
//Page 0
static uint8_t F01_addr = 0;
static uint8_t F12_addr = 0;
//static uint8_t F34_addr = 0;// Firmware stuffs.

// Page 1
//static uint8_t F54_addr = 0;

// Page 2
static uint8_t F1A_addr = 0;

// Page 3
//static uint8_t F55_addr = 0;

// 2D sensor params of F12
static uint8_t F12_maxfinger = 0;// store maximum finger supported by the chip
static uint8_t F12_report_addr = 0;// Store ACTUAL HID data report address of F12.

// General purpose array for reading various report (Query, Control, HID data).
// Share same variable to save RAM
static uint8_t report[90];


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
	if(I2C1_SR1 & 0x02){// if address found
		printf("S3501/S3508 found!\n");
	}
	i2c_stop();
}

// Set page of S3501
void s3501_setPage(uint8_t page_num){
	if(page_num != current_page){
	current_page = page_num;
	i2c_start();// Generate Start condition
	
	i2c_write_addr(S3501_ADDR);// Request write to S3501.
	
	i2c_write(0xFF);// Write lower half byte which is dummy
	i2c_write(page_num);// Write upper half which is page number.

	i2c_stop();// Generate stop condition
	}
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
			
			// Detect RMI4 function
			switch(s3501_desc[5]){
			case RMI4_F01: // Function 0x01, RMI device control.
				F01_addr = i;// save F01 address. 	
				break;
				
			case RMI4_F12: // Function 0x12, 2-D sensor.
				F12_addr = i;// save F12 address.
				F12_report_addr = s3501_desc[3];// save the F12 data report address. This will be the actual address that we read from (mine is 0x06).
				break;
			
			case RMI4_F1A: // Function 0x1A
				F1A_addr = i;
				break;
				
			/*case 0x54: // Function 0x54
				F54_addr = i;
				break;*/
				
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
	
#ifdef MORE_INFO
	s3501_setPage(0);// Set to Page 0
	s3501_read(F01_addr, report, 6);// Retrieve Description Table of F01
	s3501_read(report[0], report, 21);// Read from Query register of F01
	
	printf("F01 Query:\n");
	
	for(uint8_t i = 0; i < 21; i ++){
		printf("0x%02X ", report[i]);
		if(i%5 == 4){
			printf("\n");
		}
	}
	usart_write('\n');
	// report[0] -> manufacturer ID (always 1).
	// report[1] -> product porperty
	// report[2] report[3] -> 7bit (0:6) Product info
	// report[4] -> Date code year 5bit (0:4)
	// report[5] -> Date code Month 4bit (0:3)
	// report[6] -> Date code day 5bit (0:4)
	// report[7] report[8] -> tester id 14bit 
	// report[9] F01_query[10] -> Serial number 14bit (0:6 and 0:6 combined to 0:13)
	// report[11] to report[20] -> Product id in 7bit ASCII character 1 to 10
#else
	s3501_setPage(0);// Set to Page 0
	s3501_read(F01_addr, report, 6);// Retrieve Description Table of F01
	s3501_read(report[0], report, 4);// Read from Query register of F01

#endif

	if(report[0] != 1){// Check for manufacturer id (always 1)
		return 5;
	}
	
	printf("Product property: 0x%02X, Product Info: 0x%02X 0x%02X\n", report[1], report[2], report[3]);
	
	// Get F12 / 2D touch property 
	s3501_read(F12_addr, report, 6);// Retrieve Description Table of F12
	s3501_read(report[2], report, 89);// Read from control register of F12
	printf("F12 parsing the HID report:\n");
	
	// for(uint8_t j = 0; j < 89; j ++){
		// printf("0x%02X ", report[j]);
		// if(j%5 == 4){
			// printf("\n");
		// }
		
	// }
	// usart_write('\n');
	
	uint16_t max_x = (report[1] << 8) | report[0];
	uint16_t max_y = (report[3] << 8) | report[2];
	
	uint16_t rx_pitch = (report[5] << 8) | report[4]; 
	uint16_t tx_pitch = (report[7] << 8) | report[6]; 
	
	printf("Max X :%u Max Y :%u\n", max_x, max_y);
	
	// TODO : Find the correct finger report.
	
	// switch(report[0] & 0x07){// check bit 0-2 for maximum finger detection.
	// case 0: // 1 finger
		// F12_maxfinger = 1;
		// break;
	
	// case 1: // 2 fingers
		// F12_maxfinger = 2;
		// break;
		
	// case 2: // 3 fingers
		// F12_maxfinger = 3;
		// break;
		
	// case 3: // 4 fingers
		// F12_maxfinger = 4;
		// break;
		
	// case 4: // 5 fingers 
		// F12_maxfinger = 5;
		// break;
		
	// case 5: // 10 fingers
		// F12_maxfinger = 10;
		// break;
		
	// default:
		// F12_maxfinger = 1;
		// break;
	// }
	
	printf("Supported finger %d\n", F12_maxfinger);
	
	return 0;
	
}

// Get HID report data (X,Y coordinates and other stuffs).
// Assuming that it has 2 fingers report (I guess).
void s3501_HIDreport(){
	// HID report (Capable upto 10 fingers, But due to the suraface area.  
	// It doesn't make any sense to report more than 2 fingers but the data is there).
	// That's why the data report size is 88 bytes, 8*10 + 8 byte.
	
	// Finger 1
	// report[0] -> Object present (0x01 == Finger)
	// report[1] -> ABS_X LSB
	// report[2] -> ABS_X MSB
	// report[3] -> ABS_Y LSB
	// report[4] -> ABS_Y MSB
	// report[5] -> pressure 
	// report[6] -> ABS_MT_TOUCH_MINOR
	// report[7] -> ABS_MT_TOUCH_MAJOR

	// Finger 2
	// report[8] -> Object present (0x01 == Finger)
	// report[9] -> ABS_X LSB
	// report[10] -> ABS_X MSB
	// report[11] -> ABS_Y LSB
	// report[12] -> ABS_Y MSB
	// report[13] -> pressure 
	// report[14] -> ABS_MT_TOUCH_MINOR
	// report[15] -> ABS_MT_TOUCH_MAJOR
	
	//.
	//.
	//.
	// Finger 10

	// report[16] to report[85] are all at 0x00

	// report[86] -> BTN_TOUCH single finger == 1, 2 fingers == 3
	// report[87] is 0x01
	
	s3501_setPage(0);// Set page 0 to get data from F12.
	s3501_read(F12_report_addr, report, 88);// Read from Data register of F12

	// Raw binary send via uart for playing around with MATLAB.
	// usart_write(report[1]);
	// usart_write(report[2]);
	// usart_write(report[3]);
	// usart_write(report[4]);
	// usart_write(report[5]);
	// usart_write(report[9]);
	// usart_write(report[10]);
	// usart_write(report[11]);
	// usart_write(report[12]);
	// usart_write(report[13]);

	usart_write(0x0c);
	if(report[0]){
	//printf("Finger 1 :\n");
	printf("X1: %d Y1: %d\n", (report[2] << 8) | report[1], (report[4] << 8) | report[3]);
	printf("Pressure1: %d\n", report[5]);
	}
	
	if(report[8]){
	//printf("Finger 2 :\n");
	printf("X2: %d Y2: %d\n", (report[10] << 8) | report[9], (report[12] << 8) | report[11]);
	printf("Pressure2: %d\n", report[13]);
	}
	
	if(report[16]){
	//printf("Finger 2 :\n");
	printf("X3: %d Y3: %d\n", (report[18] << 8) | report[17], (report[20] << 8) | report[19]);
	printf("Pressure3: %d\n", report[21]);
	}
	
	if(report[24]){
	//printf("Finger 2 :\n");
	printf("X4: %d Y4: %d\n", (report[26] << 8) | report[25], (report[28] << 8) | report[27]);
	printf("Pressure4: %d\n", report[29]);
	}
	
	if(report[32]){
	//printf("Finger 2 :\n");
	printf("X5: %d Y5: %d\n", (report[34] << 8) | report[33], (report[36] << 8) | report[35]);
	printf("Pressure5: %d\n", report[37]);
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
	
	while(1){// Polling instead of interrupt (Slow down enought for the readable serial print).
		s3501_HIDreport();
		delay_ms(200);
	}
}

/* read/write process
- Set page with s3501_setPage
- read or write data with s3501_read or s3501_write
*/