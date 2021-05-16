#include "spi.h"
#include "stm8l.h"

void SPI_init() {
	CLK_PCKENR1 |= (1 << 4);// Enable SPI1 clock

	//Enable SPI I/O GPIOs
	PB_DDR |= (1 << 5) | (1 << 6);// SCK and MOSI pin are output
	PB_CR1 |= (1 << 5) | (1 << 6);// with Push-Pull Mode
	PB_CR2 |= (1 << 5) | (1 << 6);// Plus HIGH SPEEEEEEDDDDDD.

	PB_DDR &= (0 << 7);// MISO pin is input
	PB_CR1 |= (1 << 7);// with nice internal pull-up

	SPI1_CR1 = (1 << SPI1_CR1_MSTR) | (1 << SPI1_CR1_BR1);
	SPI1_CR2 = (1 << SPI1_CR2_SSM) | (1 << SPI1_CR2_SSI) | (1 << SPI1_CR2_BDM) | (1 << SPI1_CR2_BDOE);
	SPI1_CR1 |= (1 << 6);// Enable SPI1 
}

uint8_t SPI_read() {
    SPI_write(0xFF);
    while (!(SPI1_SR & (1 << SPI1_SR_RXNE)));
    return SPI1_DR;
}

void SPI_write(uint8_t data) {
    SPI1_DR = data;
    while (!(SPI1_SR & (1 << SPI1_SR_TXE)));
}
