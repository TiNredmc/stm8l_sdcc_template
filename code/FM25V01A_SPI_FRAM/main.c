/* UNTESTED : Example code, Interfacing with FM25V01A Serial (SPI) F-RAM
 * Coded by TinLethax 2021/05/25
 */

/* Possible replacement
 * MB85RS128B (Fujitsu)
 */

#include <stm8l.h>
#include <spi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <usart.h>
#include <delay.h>

//Define CS pin
#define SCS	4 // PB4 as a CS pin

// Opcode (commands)
uint8_t FM25_FSTRD 	= 0x0B;// Fast Read memory data

uint8_t FM25_RDID	= 0x9F;// Read device ID (respond with 9 bytes)

uint8_t FM25_RDSR	= 0x05;// Read Status Register

uint8_t FM25_READ	= 0x03;// Read memory data

uint8_t FM25_SLEEP	= 0xB9;// Enter Sleep mode

uint8_t FM25_WRDI	= 0x04;// Reset write enable latch

uint8_t FM25_WREN	= 0x06;// Set write enable latch

uint8_t FM25_WRITE	= 0x02;// Write memory data 

uint8_t FM25_WRSR	= 0x01;// Write Status Register

// Device ID of FM25V01A
const uint8_t FMID[9] = {
	0x7f,0x7f,0x7f,
	0x7f,0x7f,0x7f,
	0xC2,0x21,0x08// ignore last byte, seems to vary through revision according to datasheet.
};
uint8_t match_data[9];// store the received device ID (for debugging).

// Status Register storing
uint8_t FMstat = 0;// buffer for status register

// Buffer for reading (for example)
uint8_t exmBuf[10];

// usart print function
// I don't use the printf becuase that code takes a lot of space.
void prntf(char *p){
	while(*p){
		usart_write(*p++);
	}
}

void prntfM(uint8_t *p, size_t len){
	while(len--){
		usart_write(*p++);
	}
}

// Initialize PB4 as Chip select pin 
void setpin(){
	PB_DDR |= (1 << SCS);
	PB_CR1 |= (1 << SCS);
	
	PB_ODR |= (1 << SCS);// _/
}

// Read Status Register from F-RAM chip
/* parameter description
 * RSR -> pointer to the byte storing Status data 
 */
void FM25_checkSR(uint8_t *RSR){
	PB_ODR &= ~(1 << SCS);// \_
	SPI_WriteThenRead(&FM25_RDSR, 1, RSR, 1);
	PB_ODR |= (1 << SCS);// _/
}

// Write Status Register to F-RAM chip
/* parameter description
 * RSR -> pointer to the byte storing Ready-to-write Status data
 */
void FM25_writeSR(uint8_t *RSR){
	// Store command and byte that will be sent
	uint8_t wrtSR[2];
	wrtSR[0] = FM25_WRSR;
	wrtSR[1] = *RSR;
	
	PB_ODR &= ~(1 << SCS);// \_
	SPI_Write(wrtSR, 2);
	PB_ODR |= (1 << SCS);// _/
}
// Write enable latch (unlock once, write as many times as you want ;D)
void FM25_unlock(){
	// Write command to unlock write protection
	PB_ODR &= ~(1 << SCS);// \_
	SPI_Write(&FM25_WREN, 1);
	PB_ODR |= (1 << SCS);// _/
	
	// Wait until WEL bit is set in the status register
	do{
	FM25_checkSR(&FMstat);
	}while(!(FMstat & (1 << 1)));

}

// Disable write enable latch (lock once, no write allowed !)
void FM25_lock(){
	// Write command to lock write protection
	PB_ODR &= ~(1 << SCS);// \_
	SPI_Write(&FM25_WRDI, 1);
	PB_ODR |= (1 << SCS);// _/
	
	// Wait until WEL bit is unset in the status register
	do{
	FM25_checkSR(&FMstat);
	}while((FMstat & (1 << 1)));

}
// Write data to F-RAM
/* parameter descriptions
 * Src -> pointer to the array to be sent 
 * offset -> offset to start writing on F-RAM
 * len -> Size of data to be sent
 */
void FM25_writeBuf(uint8_t *Src, uint16_t offset, size_t len){
	// Store command and offset before writing to F-RAM
	uint8_t wrtBF[3];
	wrtBF[0] = FM25_WRITE;
	wrtBF[1] = (uint8_t)(offset >> 8);
	wrtBF[2] = (uint8_t)offset;

	PB_ODR &= ~(1 << SCS);// \_
	SPI_WriteThenWrite(wrtBF, 3, Src, len);
	PB_ODR |= (1 << SCS);// _/
}

// Read data from F-RAM
/* parameter descriptions
 * Dest -> pointer to array storing read data from F-RAM
 * offset -> offset to start reading from F-RAM
 * len -> Size of data to be read
 */
void FM25_readBuf(uint8_t *Dest, uint16_t offset, size_t len){
	// Store command and offset before writing to F-RAM
	uint8_t wrtBF[3];
	wrtBF[0] = FM25_READ;
	wrtBF[1] = (uint8_t)(offset >> 8);
	wrtBF[2] = (uint8_t)offset;

	PB_ODR &= ~(1 << SCS);// \_
	// Send command and offset to F-RAM, F-RAM respond with data reading
	SPI_WriteThenRead(wrtBF, 3, Dest, len);
	PB_ODR |= (1 << SCS);// _/
}

/* Fast read without DMA isn't faster than FM25_readBuf
 * So I'm gonna leave it here since there's more than one way to implement 
 * Fast read with DMA
 * TODO: make it easy to user.	
 */	
//void FM25_readBufFast()

// Put F-RAM into Sleep mode
// Activity that pull Chip select pin down will wake the F-RAM from sleep
void FM25_sleep(){
	PB_ODR &= ~(1 << SCS);// \_
	SPI_Write(&FM25_SLEEP, 1);
	PB_ODR |= (1 << SCS);// _/
}

// Return device match 
/* 0 -> FM25V01A is found
 * 1 -> FM25V01A is not found
 */
int FM25_devMatch(){
	PB_ODR &= ~(1 << SCS);// \_
	SPI_WriteThenRead(&FM25_RDID, 1, match_data, 9);
	PB_ODR |= (1 << SCS);// _/
	
	return memcmp(match_data, FMID, 8);// last byte seems to vary on device revision, so just don't check them.
}

void main(){
	CLK_CKDIVR = 0x00;// Don't divide the system and make it slower than 16MHz
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	prntf(" Starting...\n");
	setpin();// init CS pin (using PB4 as CS pin)
	SPI_Init(FSYS_DIV_2);// Get maximum SPI speed at SYSclock/2
	delay_ms(100);

	if(FM25_devMatch()){
	// No Device match, Check the connection ?
	prntf("No Device match !, Please check the connection\n");
	prntf("Device ID read out:");
	prntfM(match_data, 9);
	prntf("\nProgram halted!");
	while(1);// halt the program by put into infinite loop
	}
	prntf("Device found, Unlocking...\n");
	FM25_unlock();// Unlock F-RAM, ready to write anytime.
	
	prntf("writing 'TinLethax!'...\n");
	FM25_writeBuf("TinLethax!", 0, 10);// Write "TinLethax!" to offset 0x1234 on the flash
	prntf("Done\n");
	prntf("Start reading...\n");
	FM25_readBuf(exmBuf, 0, 10);
	prntf("Done\n");
	prntf("readback is :");
	prntfM(exmBuf, 10);
	prntf("\nDone");
	
	uint8_t buf0;
	uint16_t counter = 0;
	while(1){/*
		
	}

}