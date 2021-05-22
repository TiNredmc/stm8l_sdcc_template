#include "flash.h"

// Unlock Flash by disabling write protection, writing 2 MASS keys.
void flash_unlock() {
	FLASH_PUKR = FLASH_PUKR_KEY1;
	FLASH_PUKR = FLASH_PUKR_KEY2;
	while (!(FLASH_IAPSR & (1 << FLASH_IAPSR_PUL)));
}

// Enable write protection to Flash.
void flash_lock() {
	FLASH_IAPSR &= ~(1 << FLASH_IAPSR_PUL);
}

// Wait for Writing complete flag before next byte erase / write.
void flash_wait_busy() {
	while (!(FLASH_IAPSR & 0x03));
}

/* Parameter descriptions
 * Buf -> pointer to the data array that you want to write to Flash 
 * offset -> starting point on Flash 0 - 255
 * len -> length of data to be written to Flash
 */
void flash_write(uint8_t *Buf, uint8_t offset, uint8_t len){
	uint8_t *fl;
	fl = (uint8_t *)FLASH_START_ADDR + offset;
	flash_unlock();// Unlock Flash

	while(len--){
	*fl++ = *Buf++;
	flash_wait_busy();
	}

	flash_lock();// Lock Flash
}

/* Parameter descriptions
 * Buf -> pointer to the data array that you want to store readings from Flash
 * offset -> starting point on Flash 0 - 255
 * len -> length of data to be read from Flash
 */
void flash_read(uint8_t *Buf, uint8_t offset, uint8_t len){
	uint8_t *fl;
	fl = (uint8_t *)FLASH_START_ADDR + offset;

	while(len--){
	*Buf++ = *fl++;
	}

}