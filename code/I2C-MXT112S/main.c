// Atmel maXTouch MXT112S example code with STM8L
// Ported by TinLethax 2022/08/11 +7
#include <stm8l.h>
#include <i2c.h>
#include <usart.h>
#include <delay.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//#define MATLAB_VISUALIZER


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
#define INT_CHK !(PB_IDR & (1 << MXT_INT))

// Reset pin
#define MXT_RST 5// PB5

#define MXT_ID_BLK_SIZ  7// Query ID size of 7 bytes

// Number of elements in the Object Table
uint8_t num_obj = 0;
// Object query address.
uint16_t obj_addr_cnt = 0;

/* Register map*/
// Store address of each object.
uint16_t T5_addr = 0;
uint16_t T7_addr = 0;
uint16_t T9_addr = 0;

// Genral purpose buffer array.
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
	i2c_write((uint8_t)(addr >> 8));// Write MSB byte address to MXT112S.
	
	i2c_writePtr(data, len);// Write the rest of data to MXT112S.
	
	i2c_stop();// Generate stop condition

}

// Query basic informations of MXT112S
void mxt_identify() {

	mxt_read(0x0000, MXT_BUF, MXT_ID_BLK_SIZ);

	num_obj = MXT_BUF[6];// save the object table elements count.

	printf("Family ID 0x%02X\n", MXT_BUF[0]);
	printf("Variant ID 0x%02X\n", MXT_BUF[1]);
	printf("Version : %u.%u\n", ((MXT_BUF[2] & 0xF0) >> 4), (MXT_BUF[2] & 0x0F));
	printf("Build Number : 0x%02X\n", MXT_BUF[3]);

	printf("X channels : %d\n", MXT_BUF[4]);
	printf("Y channels : %d\n", MXT_BUF[5]);

	printf("Object table elements count : %d elements\n", MXT_BUF[6]);

	num_obj = MXT_BUF[6];
	obj_addr_cnt = 0x0007;
  // maXTouch can have many object. But we specifically looking for T5 A.K.A Message Processor
  // and T9 Multitouch object
 
	for (uint8_t i = 0; i < num_obj; i ++) {
	mxt_read(obj_addr_cnt, MXT_BUF, MXT_ID_BLK_SIZ - 1);

	printf("Object : %u , ID : %u\n", i, MXT_BUF[0]);

	switch (MXT_BUF[0]) {
		case 5:// T5 message processor.
			printf("Found T5 object\n");
			T5_addr = (MXT_BUF[2] << 8) | MXT_BUF[1];
			printf("T5 object address : 0x%04X\n", T5_addr);
			break;

		case 7:// T7 power config stuffs.
			printf("Found T7 Object\n");
			T7_addr = (MXT_BUF[2] << 8) | MXT_BUF[1];
			break;

		case 9:// T9 Multi touch (Assuming the maXTouch S series have this instead of T100 from U series).
			printf("Found T9 object\n");
			T9_addr = (MXT_BUF[2] << 8) | MXT_BUF[1];
			printf("T9 object address : 0x%04X\n", T9_addr);
			break;

	}
	
	obj_addr_cnt += 6;// move on to next object.

	}
	
	// read CRC
	mxt_read(obj_addr_cnt, MXT_BUF, 3);
	printf("CRC : 0x%02X 0x%02X 0x%02X\n", MXT_BUF[0], MXT_BUF[1], MXT_BUF[2]);
	
	// Get X,Y range
	mxt_read(T9_addr + 18, MXT_BUF, 4);
	printf("X range : %u\nY range : %u\n", (MXT_BUF[1] << 8) | MXT_BUF[0], (MXT_BUF[3] << 8) | MXT_BUF[2]);
	
	// Set T7 into Free-running mode
	mxt_read(T7_addr, MXT_BUF, 3);
	MXT_BUF[0] = 255;
	MXT_BUF[1] = 255;
	mxt_write(T7_addr, MXT_BUF, 3);


}

// Report Multi touch from T9 object.
void mxt_report_t9(){
	
	// for(uint8_t i=0; i < 16; i ++){
		// printf("Dump buf[%u] : 0x%02X\n", i, MXT_BUF[i]);
	// }
	
	if(INT_CHK){
		i2c_start();// Regenerate start condition.
	
		i2c_read_addr(MXT_ADDR);// Request read from MXT112S.
		i2c_readPtr(MXT_BUF, 16);
		
		i2c_stop();// Generate stop condition.

		// Convert 2 bytes to 16bit uint X and Y position 
		xpos = (MXT_BUF[2] << 4) | ((MXT_BUF[4] >> 4) & 0x0F);
		ypos = (MXT_BUF[3] << 2) | (MXT_BUF[4] & 0x0F);
		//ypos >>= 2;// quick fix I'm lazy.

		// Check for touch amplitude (Pressure?)
		//if(MXT_BUF[1] & 0x04)
		  amplitude =  MXT_BUF[6];
#ifdef MATLAB_VISUALIZER
		// Send data packet to PC and plot the data on matlab
		// packet [Xpos LSB][Xpos MSB][Ypos LSB][Ypos MSB][Amplitude][Zero padding].
		usart_write((uint8_t)xpos);
		usart_write((uint8_t)(xpos >> 8));
		usart_write((uint8_t)ypos);
		usart_write((uint8_t)(ypos >> 8));
		usart_write(amplitude);
		usart_write(0x00);
#else		
		usart_write(0x0C);
		printf("X pos : %u Y pos : %u\n", xpos, ypos);
		printf("Pressure : %u\n", amplitude);
#endif
	}
		
	
}

void main(){
	usart_init(115200); // usart using baudrate at 115107(actual speed)
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);// clear terminal
	usart_write(0x0C);
	i2c_init(0x69, I2C_100K);// Set I2C master address to 0x69, I2C speed to 100kHz.
	printf("MXT112S on STM8L by TinLethax\n");
	PB_DDR |= (1 << MXT_RST);
	PB_CR1 |= (1 << MXT_INT) | (1 << MXT_RST);// Internul pull up on interrupt pin. And reset pin as push pull
	
	PB_ODR |= (1 << MXT_RST);
	PB_ODR &= ~(1 << MXT_RST);
	delay_ms(100);
	PB_ODR |= (1 << MXT_RST);
	delay_ms(100);
	mxt_identify();
	mxt_read(T5_addr, MXT_BUF, 16);

	while(1){// Polling instead of interrupt (Slow down enought for the readable serial print).
		mxt_report_t9();
		delay_ms(15);
	}
}

/* read/write process
- Set page with s3501_setPage
- read or write data with s3501_read or s3501_write
*/
