#ifndef SPI_H
#define SPI_H

#include <stdint.h>


/* 
 * Initialize SPI in MODE1 for stm8l
 *
 * Pinout:
 * SCK  -> PB5
 * MOSI -> PB6
 * MISO -> PB7
 * CS   -> user defined 
 */
void SPI_init();

void SPI_write(uint8_t data);

uint8_t SPI_read();

#endif /* SPI_H */

