#include "spi.h"
#include "stm8l.h"

/* parameter description
 * spi_speed -> passing FSYS_DIV_x where x is between 2-256 (Sysclock divided by spi_speed = SPI clock speed)
 */
void SPI_Init(uint8_t spi_speed) {
	CLK_PCKENR1 |= (1 << 4);// Enable SPI1 clock

	//Enable SPI I/O GPIOs
	PB_DDR |= (1 << 5) | (1 << 6) | (1 << 4);// SCK, MOSI, CS pin are output
	PB_CR1 |= (1 << 5) | (1 << 6) | (1 << 4);// with Push-Pull Mode
	PB_CR2 |= (1 << 5) | (1 << 6);// Plus HIGH SPEEEEEEDDDDDD.

	PB_DDR &= (0 << 7);// MISO pin is input
	PB_CR1 |= (1 << 7);// with nice internal pull-up

	SPI1_CR1 = (1 << SPI1_CR1_MSTR) | (1 << SPI1_CR1_BR1) | (spi_speed << 3);
	SPI1_CR2 = (1 << SPI1_CR2_SSM) | (1 << SPI1_CR2_SSI) | (1 << SPI1_CR2_BDM) | (1 << SPI1_CR2_BDOE);
	SPI1_CR1 |= (1 << 6);// Enable SPI1 
}

/* parameter descriptions
 * *data -> pointer to the rx buffer
 * len -> lenght (size) of receive byte(s)
 */
void SPI_Read(uint8_t *data, size_t len) {
    SPI_Write((uint8_t *)0xFF, 1);

	while(len--){
		while (!(SPI1_SR & (1 << SPI1_SR_RXNE)));
		*data++ =  SPI1_DR;
	}
}

/* parameter descriptions
 * *data -> pointer to the tx buffer
 * len -> lenght (size) of transmit byte(s)
 */
void SPI_Write(uint8_t *data, size_t len) {
	while(len--){
		SPI1_DR = *data++;
		while (!(SPI1_SR & (1 << SPI1_SR_TXE)));
	}
	while (SPI1_SR & 0x80);// wait until we are not busy 
}

/* parameter descriptions
 * *TXbuf -> pointer to transmit buffer (auto-counted)
 * *RXbuf -> pointer to receive buffer
 * RXlen -> lenght (size) of receive byte(s)
 */
void SPI_WriteThenRead(uint8_t *TXbuf, uint8_t *RXbuf, size_t RXlen){
	// Transmit packet to request data from slave device
	while(*TXbuf){
		SPI1_DR = *TXbuf++;
		while (!(SPI1_SR & (1 << SPI1_SR_TXE)));
	}
	while (SPI1_SR & 0x80);// wait until we are not busy
	
	while(RXlen--){
		while (!(SPI1_SR & (1 << SPI1_SR_RXNE)));
		*RXbuf++ = SPI1_DR;
	}
	while (SPI1_SR & 0x80);// wait until we are not busy
}