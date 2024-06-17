// Example code for interfacing with ADNS 5050 Optical Mouse sensor
// Coded by TinLethax 2027/01/03 +7 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
#include <usart.h>

#define REG_PRODID	0x00 // Use to verify the Serial communication
#define REG_MOTION	0x02 // Motion detection register
#define REG_DELTAX	0x03 // Delta X movement 
#define REG_DELTAY 	0x04 // Delta Y movement
#define REG_GETPIX	0x0B // Get Raw pixel reading
#define REG_RDBRST	0x63 // Motion Burst, Read Delta X and Y in single command
#define REG_LEDCTR	0x22 // LED control (strobe mode or always on)

#define RAW_IMG_SIZE	19*19
uint8_t raw_img[RAW_IMG_SIZE] = {0};

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

#define NCS_UP	__asm__("bset 0x5005, #4")
#define NCS_DN	__asm__("bres 0x5005, #4")


#define MATLAB_VISUALIZER

// Initialize Bi-directional SPI 
// Reference https://stackoverflow.com/questions/76378530/how-to-configurate-an-external-adc-chip-in-conjunction-with-stm-8
void init_BDSPI(){
	CLK_PCKENR1 |= (1 << 4);// Enable SPI1 clock

	//Enable SPI I/O GPIOs
	PB_DDR |= (1 << 5) | (1 << 4);// SCK and NCS as output 
	PB_CR1 |= (1 << 6) | (1 << 5) | (1 << 4);// with Push-Pull Mode
	PB_CR2 |= (1 << 5) | (1 << 4);// Plus HIGH SPEEEEEEDDDDDD.
	NCS_UP;

	SPI1_CR1 |= (1 << SPI1_CR1_MSTR) | (1 << 3);// SPI clock == 16MHz/4 = 4MHz
	SPI1_CR1 |= (1 << SPI1_CR1_CPOL) | (1 << SPI1_CR1_CPHA);// SPI mode 3
	SPI1_CR2 |= (1 << SPI1_CR2_SSM) | (1 << SPI1_CR2_SSI);

	SPI1_CR2 |= (1 << SPI1_CR2_BDM)| (1 << SPI1_CR2_BDOE);// Bi directional mode
	SPI1_CR1 |= (1 << 6);// Enable SPI1 
	
}

uint8_t adns5050_readReg(uint8_t reg){
	uint8_t ret = 0;
	
	SPI1_CR2 |= (1 << SPI1_CR2_BDOE);// Enable Transmitter
	//SPI1_CR1 |= (1 << 6);// Enable SPI1 
	
	NCS_DN;
	
	// Write Data 
	SPI1_DR = reg;
	while (!(SPI1_SR & (1 << SPI1_SR_TXE)));
	
	delay_us(4);// tSARD in datasheet
	
	//Read Data
	// When Switch to Read mode, clock start automatically
	SPI1_CR2 &= ~(1 << SPI1_CR2_BDOE);// Disable Transmitter
	
	while (!(SPI1_SR & (1 << SPI1_SR_RXNE)));// wait until we receive data.
	
	NCS_UP;
	SPI1_CR2 |= (1 << SPI1_CR2_BDOE);// Enable Transmitter
	ret = SPI1_DR;
	// Dummy read the remain phantom byte.
	if(SPI1_SR & (1 << SPI1_SR_RXNE))
		(void)SPI1_DR;

	return ret;
	
}

void adns5050_writeReg(uint8_t reg, uint8_t val){
	SPI1_CR2 |= (1 << SPI1_CR2_BDOE);// Enable Transmitter
	//SPI1_CR1 |= (1 << 6);// Enable SPI1 
		
	NCS_DN;

	// Write Reg and Data
	SPI1_DR = reg | (1 << 7);// Set MSB bit to indicate Write mode
	while (!(SPI1_SR & (1 << SPI1_SR_TXE)));
	
	SPI1_DR = val;
	while (!(SPI1_SR & (1 << SPI1_SR_TXE)));
	
	NCS_UP;
}

uint8_t adns5050_Probe(){
	return adns5050_readReg(REG_PRODID) != 0x12;
}

void adns5050_setLED(){
	adns5050_writeReg(REG_LEDCTR, 0x80);
}

void adns5050_getdXdY(int8_t *deltaX, int8_t *deltaY){ 
	SPI1_CR2 |= (1 << SPI1_CR2_BDOE);// Enable Transmitter
	
	NCS_DN;

	// Write Data 
	SPI1_DR = REG_RDBRST;
	while (!(SPI1_SR & (1 << SPI1_SR_TXE)));

	delay_us(4);// tSARD in datasheet

	//Read Data
	// When Switch to Read mode, clock start automatically
	SPI1_CR2 &= ~(1 << SPI1_CR2_BDOE);// Disable Transmitter
	while (!(SPI1_SR & (1 << SPI1_SR_RXNE)));// wait until we receive data.
	
	*deltaX = SPI1_DR;
	while (!(SPI1_SR & (1 << SPI1_SR_RXNE)));// wait until we receive data.
	
	SPI1_CR2 |= (1 << SPI1_CR2_BDOE);// Enable Transmitter
	*deltaY = SPI1_DR;
	
	NCS_UP;
	// Dummy read the remain phantom byte.
	if(SPI1_SR & (1 << SPI1_SR_RXNE))
		(void)SPI1_DR;
}

// TODO : Detect valid pixel
void adns5050_getImg(){
	uint8_t read_out = 0;
	adns5050_writeReg(REG_GETPIX, 0x00);// Write to Pixel_Grab register to reset the pixel counter
	for(uint16_t i = 0;i < RAW_IMG_SIZE; i++){
		usart_write(adns5050_readReg(REG_GETPIX) & 0x7F);
		delay_us(800);
	}
	
	//delay_us(20);// tSWR 20us 
}

void pre_usart(){
	CLK_CKDIVR = 0x00;// Full 16MHz
	usart_init(115200); // usart using baudrate at 115200.
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
#ifndef MATLAB_VISUALIZER	
	usart_write(0x0C);// clear terminal
	usart_write(0x0C);
#endif
}
int8_t x_pos, y_pos;
int16_t x_accmu, y_accmu;
void main() {
	pre_usart();
	init_BDSPI();
	delay_ms(100);
#ifndef MATLAB_VISUALIZER
	printf("ADNS 5050 example by TinLethax\n");
#endif
	if(adns5050_Probe()){
#ifndef MATLAB_VISUALIZER
		printf("Probe failed!\n");
#endif
		while(1);
	}else{
#ifndef MATLAB_VISUALIZER
		printf("Probe completed!\n");
#endif
	}
	adns5050_setLED();
#ifndef MATLAB_VISUALIZER
	printf("LED set!\n");
#endif 

    while (1) {
		//adns5050_getImg();
		adns5050_getdXdY(&x_pos, &y_pos);
		x_accmu += x_pos;
		y_accmu += y_pos;
		usart_write((uint8_t)x_accmu);
		usart_write((uint8_t)(x_accmu >> 8));
		usart_write((uint8_t)y_accmu);
		usart_write((uint8_t)(y_accmu >> 8));
		//printf("X pos : %d\n", x_accmu);
		//printf("Y pos : %d\n", y_accmu);
		delay_ms(10);
    }
	
	
}
