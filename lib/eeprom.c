#include "eeprom.h"

// Unlock EEPROM by disabling write protection, writing 2 MASS keys.
void eeprom_unlock() {
	FLASH_DUKR = FLASH_DUKR_KEY1;
	FLASH_DUKR = FLASH_DUKR_KEY2;
	while (!(FLASH_IAPSR & (1 << FLASH_IAPSR_DUL)));
}

// Unlock Option Byte.
void option_bytes_unlock() {
	FLASH_CR2 |= (1 << FLASH_CR2_OPT);
}

// Enable write protection to EEPROM.
void eeprom_lock() {
	FLASH_IAPSR &= ~(1 << FLASH_IAPSR_DUL);
}

// Wait for Writing complete flag before next byte erase / write.
void eeprom_wait_busy() {
	while (!(FLASH_IAPSR & 0x41));
}

/* Parameter descriptions
 * Buf -> pointer to the data array that you want to write to EEPROM 
 * offset -> starting point on EEPROM 0 - 255
 * len -> length of data to be written to EEPROM
 */
void eeprom_write(uint8_t *Buf, uint8_t offset, uint8_t len){
	uint8_t *ee;
	ee = (uint8_t *)EEPROM_START_ADDR + offset;
	eeprom_unlock();// Unlock EEPROM

	while(len--){
	*ee++ = *Buf++;
	eeprom_wait_busy();
	}

	eeprom_lock();// Lock EEPROM
}

/* Parameter descriptions
 * Buf -> pointer to the data array that you want to store readings from EEPROM
 * offset -> starting point on EEPROM 0 - 255
 * len -> length of data to be read from EEPROM
 */
void eeprom_read(uint8_t *Buf, uint8_t offset, uint8_t len){
	uint8_t *ee;
	ee = (uint8_t *)EEPROM_START_ADDR + offset;

	while(len--){
	*Buf++ = *ee++;
	}

}