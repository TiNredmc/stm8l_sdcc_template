/* UNTESTED : Example code, Interfacing with FM25V01A Serial (SPI) F-RAM
 * Coded by TinLethax 2021/05/25
 */

#include <stm8l.h>
#include <spi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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

uint8_t FM25_WRSR	= 0x01;// Werite Status Register

// Device ID of FM25V01A
const uint8_t FMID[9] = {
	0x7f,0x7f,0x7f,
	0x7f,0x7f,0x7f,
	0xC2,0x21,0x08
};

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
	PB_ODR &= (0 << SCS);// \_
	SPI_WriteThenRead((uint8_t *)FM25_RDSR, RSR, 1);
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
	
	PB_ODR &= (0 << SCS);// \_
	SPI_Write(wrtSR, 2);
	PB_ODR |= (1 << SCS);// _/
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
	wrtBF[0]= FM25_WRITE;
	wrtBF[1]= (uint8_t)(offset >> 8);
	wrtBF[2]= (uint8_t)offset;

	PB_ODR &= (0 << SCS);// \_
	SPI_Write(wrtBF, 3);
	
	// Write data to F-RAM
	SPI_Write(Src, len);
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
	wrtBF[0]= FM25_READ;
	wrtBF[1]= (uint8_t)(offset >> 8);
	wrtBF[2]= (uint8_t)offset;

	PB_ODR &= (0 << SCS);// \_
	// Send command and offset to F-RAM, F-RAM respond with data reading
	SPI_WriteThenRead(wrtBF, Dest, len);
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
	PB_ODR &= (0 << SCS);// \_
	SPI_Write((uint8_t *)FM25_SLEEP, 1);
	PB_ODR |= (1 << SCS);// _/
}

// Return device match 
/* 1 -> FM25V01A is found
 * 0 -> FM25V01A is not found
 */
int FM25_devMatch(){
	uint8_t match_data[9];
	PB_ODR &= (0 << SCS);// \_
	SPI_WriteThenRead((uint8_t *)FM25_RDID, match_data, 9);
	PB_ODR |= (1 << SCS);// _/
	
	return memcmp(match_data, FMID, 9);
}

void main(){
	CLK_CKDIVR = 0x00;// Don't divide the system and make it slower than 16MHz
	setpin();// init CS pin (using PB4 as CS pin)
	SPI_Init(FSYS_DIV_2);// Get maximum SPI speed at SYSclock/2
	
	if(FM25_devMatch()){
	// No Device match, Check the connection ?
	}

	while(1){}

}