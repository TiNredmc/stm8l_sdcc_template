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

/* I2C 7bit address of MXT112S */
#define MXT_ADDR  0x4B

// Interrupt input.
#define MXT_INT 4// PB4

#define MXT_ID_BLK_SIZ  7// Query ID size of 7 bytes

// Number of elements in the Object Table
uint8_t num_obj = 0;
// Object query address.
uint16_t obj_addr_cnt = 0;

// Store address of each object.
uint16_t T5_addr = 0;
uint16_t T9_addr = 0;
//uint16_t T100_addr = 0;

// Store message size of each object.
uint8_t T5_msg_size = 0;
uint8_t T9_msg_size = 0;
//uint8_t T100_msg_size = 0;

// Store number of report instances of each report.
uint8_t T9_instances = 0;
//uint8_t T100_instances = 0;

// Store number of report ID per instance.
uint16_t T9_report_cnt = 0;
//uint16_t T100_report_cnt = 0;

// Genral purpose buffer array
uint8_t MXT_BUF[256];

uint16_t xpos, ypos;
uint8_t amplitude;

// Read data from MXT112S
void mxt_read(uint16_t addr, uint8_t *data, uint16_t len) {

	i2c_start();// Generate start condition.
	
	i2c_write_addr(MXT_ADDR);// Request write to MXT112S.
	i2c_write((uint8_t)addr);// Write LSB byte address to MXT112S.
	i2c_write((uint8_t)(addr >> 8));// Write MSB byte address to MXT112S.
	
	i2c_start();// Regenerate start condition.
	
	i2c_read_addr(MXT_ADDR);// Request read from MXT112S.
	i2c_readPtr(data, len);
	
	i2c_stop();// Generate stop condition.

}

// Write data to MXT112S
void mxt_write(uint16_t addr, uint8_t *data, uint8_t len) {

	i2c_start();// Generate start condition.
	
	i2c_write_addr(MXT_ADDR);// Request write to MXT112S.
	i2c_write((uint8_t)addr);// Write LSB byte address to MXT112S.
	i2c_write((uint8_t)(addr >>8));// Write MSB byte address to MXT112S.
	
	i2c_writePtr(data, len);// Write the rest of data to MXT112S.
	
	i2c_stop();// Generate stop condition

}

// Query basic informations of MXT112S
void mxt_identify() {
	uint8_t buf[MXT_ID_BLK_SIZ] = {0};

	mxt_read(0x0000, buf, MXT_ID_BLK_SIZ);

	num_obj = buf[6];// save the object table elements count.

	printf("Family ID 0x%02X\n", buf[0]);
	printf("Variant ID 0x%02X\n", buf[1]);\
	printf("Version : %u.%u\n", ((buf[2] & 0xF0) << 4), (buf[2] & 0x0F));
	printf("Build Number : 0x0%02X\n", buf[3]);

	printf("X channels : %d\n", buf[4]);
	printf("Y channels : %d\n", buf[5]);

	printf("Object table elements count : %d elements\n", buf[6]);

	num_obj = buf[6];
	obj_addr_cnt = 0x0007;
  // maXTouch can have many object. But we specifically looking for T5 A.K.A Message Processor
  // and T100 A.K.A Multitouch
  //
	for (uint8_t i = 0; i < num_obj; i ++) {
	mxt_read(obj_addr_cnt, buf, MXT_ID_BLK_SIZ - 1);

	printf("Object : %d , ID : 0x%02X\n", i, buf[0]);

	switch (buf[0]) {
	  case 5:// T5 message processor.
		printf("Found T5 object\n");
		T5_addr = (buf[2] << 8) | buf[1];
		printf("T5 object address : 0x%04X\n", T5_addr);
		T5_msg_size = buf[3] + 1;
		printf("T5 message size : %u\n", T5_msg_size);
		break;

	  case 9:// T9 Multi touch (Assuming the maXTouch S series have this instead of T100 from U series).
		printf("Found T9 object\n");
		T9_addr = (buf[2] << 8) | buf[1];
		printf("T9 object address : 0x%04X\n", T9_addr);
		T9_msg_size = buf[3] + 1;
		T9_instances = buf[4] + 1;
		T9_report_cnt = (buf[4] + 1) * buf[5];
		printf("T9 message size : %u\nT9 report count %u\nT9 report IDs : %u\n", T9_msg_size, T9_report_cnt, buf[5]);
		break;

	//      case 100:// T100 Multi touch
	//        printf("Found T100 object\n");
	//        T100_addr = (buf[2] << 8) | buf[1];
	//        T100_msg_size = buf[3] + 1;
	//        T100_instances = buf[4] + 1;
	//        T100_report_cnt = (buf[4] + 1) * buf[5];
	//        break;
	}
	
	obj_addr_cnt += 6;// move on to next object.

	}
	
	// read CRC
	mxt_read(obj_addr_cnt, buf, 3);
	printf("CRC : 0x%02X 0x%02X 0x%02X\n", buf[0], buf[1], buf[2]);

}

// Report Multi touch from T9 object.
// But data is read from T5 message object.
// Since T9 can report Upto 10 fingers. 
// The report ID can be more than 1 Report ID.
// Starting from 0x09 (T9 base report ID).
void mxt_report_t9(){
	if(!(PB_IDR & (1 << MXT_INT))){
	mxt_read(T5_addr, MXT_BUF, T5_msg_size * 2);

	printf("We got data! ID : 0x%02X\n", MXT_BUF[0]);
	

	if(MXT_BUF[0] != 9){// If we didn't get T9 report (check for the T9 report ID).
	  // Just ignore the interrupt.
	  return;      
	}

	// Check status from first byte.
	if(MXT_BUF[1] & 0x02)// If touch has been suppressed by Grip or Face (imagine put your phone to your ear).
	 // Just ignore
	  return;

	// Convert 2 bytes to 16bit uint X and Y position 
	xpos = (MXT_BUF[0x02] << 4) | ((MXT_BUF[0x04] >> 4) & 0x0F);
	ypos = (MXT_BUF[0x03] << 4) | (MXT_BUF[0x04] & 0x0F);
	
	printf("X pos : %ul Ypos : %ul\n", xpos, ypos);

	// Check for touch amplitude (Pressure?)
	if(MXT_BUF[1] & 0x04)
	  amplitude =  MXT_BUF[0x06];
	}
}


void main(){
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);// clear terminal
	usart_write(0x0C);
	i2c_init(0x69, I2C_100K);// Set I2C master address to 0x69, I2C speed to 100kHz.
	printf("MXT112S on STM8L by TinLethax\n");
	PB_CR1 |= 1 << MXT_INT;// Internul pull up on interrupt pin.
	delay_ms(100);
	mxt_identify();
	
	while(1){// Polling instead of interrupt (Slow down enought for the readable serial print).
		mxt_report_t9();
		delay_ms(10);
	}
}

/* read/write process
- Set page with s3501_setPage
- read or write data with s3501_read or s3501_write
*/
