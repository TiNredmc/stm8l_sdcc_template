#ifndef FLASH_H
#define FLASH_H

#include "stm8l.h"

#define FLASH_START_ADDR      0x8000
#define FLASH_END_ADDR        0x9FFF

// Disable Flash write protection with MASS keys
void flash_unlock();
// Enable Flash write protection 
void flash_lock();
// Wait for Writing complete flag before next byte erase / write.
void flash_wait_busy();

void flash_write(uint8_t *Buf, uint8_t offset, uint8_t len);
void flash_read(uint8_t *Buf, uint8_t offset, uint8_t len);

#endif 
