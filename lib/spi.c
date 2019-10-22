#include "spi.h"
#include "stm8l.h"

void SPI_init() {
    SPI1_CR1 = (1 << SPI1_CR1_MSTR) | (1 << SPI1_CR1_SPE) | (1 << SPI1_CR1_BR1);
    SPI1_CR2 = (1 << SPI1_CR2_SSM) | (1 << SPI1_CR2_SSI) | (1 << SPI1_CR2_BDM) | (1 << SPI1_CR2_BDOE);
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
