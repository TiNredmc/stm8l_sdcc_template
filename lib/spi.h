#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stdio.h>

#define FSYS_DIV_2	0
#define FSYS_DIV_4	1 
#define FSYS_DIV_8	2
#define FSYS_DIV_16	3
#define FSYS_DIV_32	4
#define FSYS_DIV_64	5
#define FSYS_DIV_128	6
#define FSYS_DIV_256	7

/* 
 * Initialize SPI in MODE1 for stm8l
 *
 * Pinout:
 * SCK  -> PB5
 * MOSI -> PB6
 * MISO -> PB7
 * CS   -> PB4
 */

void SPI_Init(uint8_t spi_speed);

void SPI_Write(uint8_t *data, size_t len);

void SPI_Read(uint8_t *data, size_t len);

void SPI_WriteThenRead(uint8_t *TXbuf, uint8_t *RXbuf, size_t RXlen);

void SPI_WriteThenWrite(uint8_t *CMD, uint8_t *TXbuf, size_t TXlen);

#endif /* SPI_H */

