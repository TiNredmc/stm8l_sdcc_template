// Example code for CDM101. A rare 4x16 LED matrix. Based on CDM102 code.
// Coded by TinLethax 2022/09/16 +7 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
#include <usart.h>
#include <spi.h>

// Pinout and connection.
// Identical to the CDM102
// CDM101      STM8L151F3
// 1. Load		PC4
// 2. Vcc		Vcc (+3v3 works)
// 3. SCLK		PB5
// 4. GND		GND
// 5. SDI		PB6
// 6. Reset		PC5

#define SPI_CS	4// PC4 will be used as CS pin
#define SPI_RST	5// PC5 for display reset.

#define SPI_SEL		PC_ODR &= ~(1 << SPI_CS)
#define SPI_UNSEL   PC_ODR |= (1 << SPI_CS)

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

/* USER CODE BEGIN PV */
const uint8_t byte_rev_table[256] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

uint8_t brightness = 0;

// Frame buffer
// Single byte contain 4 pixels data
// Each pixel use 2 bit.
// 0 -> Pixel off
// 1 -> Pixel turn Orange.
// 2 -> Pixel turn Green.
// 3 -> Pixel turn Yellow.
uint8_t FB[16] = {
// COL1-4  COL5-8 COL9-12 COL13-16
	0x00,  0x00,  0x00,   0x00,	// row 1
	0x00,  0x00,  0x00,   0x00, // row 2
	0x00,  0x00,  0x00,   0x00, // row 3
	0x00,  0x00,  0x00,   0x00  // row 4
};

// Init GPIOs for Joy stock button and CS pin
void GPIO_init(){
	// Init PC4 as push-pull output and set at High level
	PC_DDR |= (1 << SPI_CS) | (1 << SPI_RST);
	PC_CR1 |= (1 << SPI_CS) | (1 << SPI_RST);
	PC_ODR |= (1 << SPI_CS) | (1 << SPI_RST);
}

// Reset CDM101
void cdm101_rst(){
	PC_ODR |= (1 << SPI_RST);
	PC_ODR &= ~(1 << SPI_RST);
	delay_ms(100);
	PC_ODR |= (1 << SPI_RST);
}

// Send byte to the CDM101.
void cdm101_tx(uint8_t data){
	data = byte_rev_table[data];
	SPI_SEL;
	SPI_Write(&data, 1);
	SPI_UNSEL;
}

// Send multiple byte to the display.
void cdm101_tx_multiple(uint8_t *data, uint8_t len){
	for(uint8_t i=0; i < len; i++)
		cdm101_tx(*(data + i));
}

// Render frame buffer and send frame data to display.
void cdm101_rndr(){

	/*
	*	Left hand Group (Col 1-7) + Col15,Row4 Dot
	*/

	cdm101_tx(0xB0);// Green pixel left hand group.
	cdm101_tx( (uint8_t)((FB[12] & 0x80) >> 3) | (uint8_t)((FB[8] & 0x80) >> 4) | (uint8_t)((FB[4] & 0x80) >> 5) | (uint8_t)((FB[0] & 0x80) >> 6) );// Col1 
	cdm101_tx( (uint8_t)((FB[12] & 0x20) >> 1) | (uint8_t)((FB[8] & 0x20) >> 2) | (uint8_t)((FB[4] & 0x20) >> 3) | (uint8_t)((FB[0] & 0x20) >> 4) );// Col2 
	cdm101_tx( (uint8_t)((FB[12] & 0x08) << 1) | (uint8_t)(FB[8] & 0x08) 	    | (uint8_t)((FB[4] & 0x08) >> 1) | (uint8_t)((FB[0] & 0x08) >> 2) );// Col3 
	cdm101_tx( (uint8_t)((FB[12] & 0x02) << 3) | (uint8_t)((FB[8] & 0x02) << 2) | (uint8_t)((FB[4] & 0x02) << 1) | (uint8_t)(FB[0] & 0x02) 		  );// Col4 
	
	cdm101_tx( (uint8_t)((FB[13] & 0x80) >> 3) | (uint8_t)((FB[9] & 0x80) >> 4) | (uint8_t)((FB[5] & 0x80) >> 5) | (uint8_t)((FB[1] & 0x80) >> 6) );// Col5 
	cdm101_tx( (uint8_t)((FB[13] & 0x20) >> 1) | (uint8_t)((FB[9] & 0x20) >> 2) | (uint8_t)((FB[5] & 0x20) >> 3) | (uint8_t)((FB[1] & 0x20) >> 4) );// Col6 
	cdm101_tx( (uint8_t)((FB[13] & 0x08) << 1) | (uint8_t)(FB[9] & 0x08) 	    | (uint8_t)((FB[5] & 0x08) >> 1) | (uint8_t)((FB[1] & 0x08) >> 2) | (uint8_t)((FB[15] & 0x08) >> 3));// Col7 + Col15,Row4 Dot

	cdm101_tx(0xB2);// Orange pixel left hand group.
	cdm101_tx( (uint8_t)((FB[12] & 0x40) >> 2) | (uint8_t)((FB[8] & 0x40) >> 3) | (uint8_t)((FB[4] & 0x40) >> 4) | (uint8_t)((FB[0] & 0x40) >> 5) );// Col1 
	cdm101_tx( (uint8_t)(FB[12] & 0x10)		   | (uint8_t)((FB[8] & 0x10) >> 1) | (uint8_t)((FB[4] & 0x10) >> 2) | (uint8_t)((FB[0] & 0x10) >> 3) );// Col2 
	cdm101_tx( (uint8_t)((FB[12] & 0x04) << 2) | (uint8_t)((FB[8] & 0x04) << 1) | (uint8_t)(FB[4] & 0x04) 		 | (uint8_t)((FB[0] & 0x04) >> 1) );// Col3 
	cdm101_tx( (uint8_t)((FB[12] & 0x01) << 4) | (uint8_t)((FB[8] & 0x01) << 3) | (uint8_t)((FB[4] & 0x01) << 2) | (uint8_t)((FB[0] & 0x01) << 1) );// Col4 
	
	cdm101_tx( (uint8_t)((FB[13] & 0x40) >> 2) | (uint8_t)((FB[9] & 0x40) >> 3) | (uint8_t)((FB[5] & 0x40) >> 4) | (uint8_t)((FB[1] & 0x40) >> 5) );// Col5 
	cdm101_tx( (uint8_t)(FB[13] & 0x10) 	   | (uint8_t)((FB[9] & 0x10) >> 1) | (uint8_t)((FB[5] & 0x10) >> 2) | (uint8_t)((FB[1] & 0x10) >> 3) );// Col6 
	cdm101_tx( (uint8_t)((FB[13] & 0x04) << 2) | (uint8_t)((FB[9] & 0x04) << 1) | (uint8_t)(FB[5] & 0x04) 		 | (uint8_t)((FB[1] & 0x04) >> 1) | (uint8_t)((FB[15] & 0x04) >> 2));// Col7 + Col15,Row4 Dot
	
	
	/*
	*	Right hand Group (Col 8-14) + Col 15 and Col 1t
	*/
	
	cdm101_tx(0xB1);// Green pixel right hand group.
	cdm101_tx( (uint8_t)(FB[1] & 0x02)        | (uint8_t)((FB[5] & 0x02) << 1)  | (uint8_t)((FB[9] & 0x02) << 2) | (uint8_t)((FB[13] & 0x02) << 3) | (uint8_t)((FB[15] & 0x02) >> 1));// Col8 + Col16,Row4 Dot
	
	cdm101_tx( (uint8_t)((FB[2] & 0x80) >> 6) | (uint8_t)((FB[6] & 0x80) >> 5) | (uint8_t)((FB[10] & 0x80) >> 4) | (uint8_t)((FB[14] & 0x80) >> 3) | (uint8_t)((FB[11] & 0x02) >> 1));// Col9 + Col16,Row3 Dot
	cdm101_tx( (uint8_t)((FB[2] & 0x20) >> 4) | (uint8_t)((FB[6] & 0x20) >> 3) | (uint8_t)((FB[10] & 0x20) >> 2) | (uint8_t)((FB[14] & 0x20) >> 1) | (uint8_t)((FB[11] & 0x08) >> 3));// Col10 + Col15,Row3 Dot
	cdm101_tx( (uint8_t)((FB[2] & 0x08) >> 2) | (uint8_t)((FB[6] & 0x08) >> 1) | (uint8_t)(FB[10] & 0x08) 		 | (uint8_t)((FB[14] & 0x08) << 1) | (uint8_t)((FB[3] & 0x02) >> 1) );// Col11 + Col16,Row1 Dot	
	cdm101_tx( (uint8_t)(FB[2] & 0x02) 	      | (uint8_t)((FB[6] & 0x02) << 1) | (uint8_t)((FB[10] & 0x02) << 2) | (uint8_t)((FB[14] & 0x02) << 3) | (uint8_t)((FB[7] & 0x02) >> 1) );// Col12 + Col16,Row2 Dot
	
	cdm101_tx( (uint8_t)((FB[3] & 0x80) >> 6) | (uint8_t)((FB[7] & 0x80) >> 5) | (uint8_t)((FB[11] & 0x80) >> 4) | (uint8_t)((FB[15] & 0x80) >> 3) | (uint8_t)((FB[3] & 0x08) >> 3) );// Col13 + Col15,Row1 Dot
	cdm101_tx( (uint8_t)((FB[3] & 0x20) >> 4) | (uint8_t)((FB[7] & 0x20) >> 3) | (uint8_t)((FB[11] & 0x20) >> 2) | (uint8_t)((FB[15] & 0x20) >> 1) | (uint8_t)((FB[7] & 0x08) >> 3) );// Col14 + Col15,Row2 Dot
	
	cdm101_tx(0xB3);// Orange pixel right hand group.
	cdm101_tx( (uint8_t)((FB[1] & 0x01) << 1) | (uint8_t)((FB[5] & 0x01) << 2)  | (uint8_t)((FB[9] & 0x01) << 3) | (uint8_t)((FB[13] & 0x01) << 4) | (uint8_t)(FB[15] & 0x01)		);// Col8 + Col16,Row4 Dot
	
	cdm101_tx( (uint8_t)((FB[2] & 0x40) >> 5) | (uint8_t)((FB[6] & 0x40) >> 4) | (uint8_t)((FB[10] & 0x40) >> 3) | (uint8_t)((FB[14] & 0x40) >> 2) | (uint8_t)(FB[11] & 0x01)		);// Col9 + Col16,Row3 Dot
	cdm101_tx( (uint8_t)((FB[2] & 0x10) >> 3) | (uint8_t)((FB[6] & 0x10) >> 2) | (uint8_t)((FB[10] & 0x10) >> 1) | (uint8_t)(FB[14] & 0x10) 	   | (uint8_t)((FB[11] & 0x04) >> 2));// Col10 + Col15,Row3 Dot
	cdm101_tx( (uint8_t)((FB[2] & 0x04) >> 1) | (uint8_t)(FB[6] & 0x04)		   | (uint8_t)((FB[10] & 0x04) << 1) | (uint8_t)((FB[14] & 0x04) << 2) | (uint8_t)(FB[3] & 0x01)		);// Col11 + Col16,Row1 Dot
	cdm101_tx( (uint8_t)((FB[2] & 0x01) << 1) | (uint8_t)((FB[6] & 0x01) << 2) | (uint8_t)((FB[10] & 0x01) << 3) | (uint8_t)((FB[14] & 0x01) << 4) | (uint8_t)(FB[7] & 0x01)		);// Col12 + Col16,Row2 Dot
	
	cdm101_tx( (uint8_t)((FB[3] & 0x40) >> 5) | (uint8_t)((FB[7] & 0x40) >> 4) | (uint8_t)((FB[11] & 0x40) >> 3) | (uint8_t)((FB[15] & 0x40) >> 2) | (uint8_t)((FB[3] & 0x04) >> 2) );// Col13 + Col15,Row1
	cdm101_tx( (uint8_t)((FB[3] & 0x10) >> 3) | (uint8_t)((FB[7] & 0x10) >> 2) | (uint8_t)((FB[11] & 0x10) >> 1) | (uint8_t)(FB[15] & 0x10)		   | (uint8_t)((FB[7] & 0x04) >> 2) );// Col14 + Col15,Row4 Dot

	
}

void cdm101_fbClear(){
	for(uint8_t n=0; n < 16; n++)
		FB[n] = 0;
	
}

// Set side bar color
// 1 -> green
// 0 -> orange
void cdm101_sidebarcolor(uint8_t color){
	color = color ? 8 : 0;
	cdm101_tx(0xE0 + brightness + color);
}

uint8_t x_cnt = 0;
const uint8_t shiftOrange_LUT[4] = {6, 4, 2, 0};
const uint8_t shiftGreen_LUT[4] = {7, 5, 3, 1};
uint8_t prev_val = 0;
void plotter(uint8_t value){
	uint8_t row_sel = 0;
	value /= 64;// Down scale from 0-255 to 0-3
	value = 3 - value;// Invert.
	value *= 4;// Time 4 to select between Row 1 (0) to Row 4 (3).
	
	// Increment + wrap around
	if(prev_val != value){
		FB[(x_cnt/4) + value] |= 1 << shiftGreen_LUT[x_cnt%4];
		prev_val = value;
		x_cnt++;
	}
	
	if(x_cnt > 15){
		x_cnt = 0;	
		//cdm101_tx(0xC0);
		cdm101_rndr();
		cdm101_fbClear();
	}
		
	
}

uint8_t i = 0;
uint8_t c = 0;
void main() {
	CLK_CKDIVR = 0x00;// Full 16MHz
	usart_init(9600); // usart using baudrate at 9600(actual speed).
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);// clear terminal
	usart_write(0x0C);
	GPIO_init();
	SPI_Init(FSYS_DIV_4);// 16MHz/16 = 1Mhz SPI clock
	cdm101_rst(); //reset display for clean init.
	cdm101_tx(0xDF);// Put test pattern on the display
	delay_ms(500);

	// FB[0] = 0x9E;
	// FB[1] = 0x79;
	// FB[2] = 0xE7;
	// FB[3] = 0x9E;
	
	// FB[4] = 0x9E;
	// FB[5] = 0x79;
	// FB[6] = 0xE7;
	// FB[7] = 0x9E;
	
	// FB[8] = 0x9E;
	// FB[9] = 0x79;
	// FB[10] = 0xE7;
	// FB[11] = 0x9E;
	
	// FB[12] = 0x9E;
	// FB[13] = 0x79;
	// FB[14] = 0xE7;
	// FB[15] = 0x9E;
	// cdm101_rndr();
	
	
    while (1) {
	do{
		plotter(i);
		//cdm101_sidebarcolor(c);
		//c ^= 1;
		delay_us(100);
		i++;
	}while(i);
	i = 255;
	while(i){
		plotter(i--);
		//cdm101_sidebarcolor(c);
		//c ^= 1;
		delay_us(100);
	}
	
    }
	
	
}

// Pixel map
// LED configuration
//         COL1 ... COL7 COL8 ... COL14 COL15 COL16
// [?] [?] [1]		[1]	 [1]	   [1]   [1]   [1]	ROW1 
// [?] [?] [2]		[2]  [2]	   [2]   [2]   [2]  ROW2
// [?] [?] [3]		[3]  [3]	   [3]   [3]   [3]  ROW3
// [?] [?] [4]		[4]  [4]	   [4]   [4]   [4]  ROW4
// 
//////////////////////////////////////////////////////////

// Pixel control
// The LED is divided into 2 group. I will can them Left hand and Right hand group.
// Same as the CDM102. The command 0xAX and 0xBX is used to set the color of the pixel.
// But the major differences are that 
// 1. Column 15 and Column 16 pixels are controlled by Bit 0 of pixel data.
// 2. From Column 1 to Column 14. Row 1 is controlled by Bit 1 and so on.
// 3. Each group of CDM102 has 5x6 pixels configuration, but CDM101 has 4x7 + COL15 and COL16.
//
// Pixel control map and group
// The pixel will be reporesented in the form of CXRY
// Where X is column number (1 - 16) and Y is Row number (1 - 4).
//
//	NOTICE : The Row order and bit of Left hand and Right hand group is different.
// Left hand -> Bit 0 = Row 4, Bit 3 = Row 1
// Right hand -> Bit 0 = Row 1, Bit 3 = Row 4
//
// Left hand Group 
// 		Byte 0	Byte 1	Byte 2	Byte 3	Byte 4	Byte 5	Byte 6
//Bit
// 0	  0       0       0       0       0        0    C15R4
// 1	 C1R4	 C2R4	 C3R4	 C4R4	 C5R4	 C6R4	 C7R4
// 2	 C1R3	 C2R3	 C3R3	 C4R3	 C5R3	 C6R3	 C6R3
// 3	 C1R2	 C2R2	 C3R2	 C4R2	 C5R2	 C6R2	 C7R2
// 4	 C1R1	 C2R1	 C3R1	 C4R1	 C5R1	 C6R1	 C7R1
// 5 	  0 	  0 	  0 	  0 	  0 	  0 	  0
// 6	  0 	  0 	  0 	  0 	  0 	  0 	  0
// 7	  0 	  0 	  0 	  0 	  0 	  0 	  0
//
//
// Right hand Group
// 		Byte 0	Byte 1	Byte 2	Byte 3	Byte 4	Byte 5	Byte 6
//Bit
// 0	C16R4   C16R3   C15R3   C16R1   C16R2   C15R1   C15R2
// 1	 C1R1	 C2R1	 C3R1	 C4R1	 C5R1	 C6R1	 C7R1
// 2	 C1R2	 C2R2	 C3R2	 C4R2	 C5R2	 C6R2	 C6R2
// 3	 C1R3	 C2R3	 C3R3	 C4R3	 C5R3	 C6R3	 C7R3
// 4	 C1R4	 C2R4	 C3R4	 C4R4	 C5R4	 C6R4	 C7R4
// 5 	  0 	  0 	  0 	  0 	  0 	  0 	  0
// 6	  0 	  0 	  0 	  0 	  0 	  0 	  0
// 7	  0 	  0 	  0 	  0 	  0 	  0 	  0
//