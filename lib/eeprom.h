#ifndef EEPROM_H
#define EEPROM_H

#include "stm8l.h"

#define EEPROM_START_ADDR      0x1000
#define EEPROM_END_ADDR        0x10FF

/* Option bytes */
#define OPT0		_MEM_(0x4800)
#define OPT1		_MEM_(0x4802)
#define OPT3		_MEM_(0x4808)
#define OPT4		_MEM_(0x4809)
#define OPT5		_MEM_(0x480A)
#define OPTBL_H		_MEM_(0x480B)
#define OPTBL_L		_MEM_(0x480C)

/**
 * Enable write access to EEPROM.
 */
void eeprom_unlock();


/**
 * Enable write access to option bytes.
 * EEPROM must be unlocked first.
 */
void option_bytes_unlock();


/**
 * Disable write access to EEPROM.
 */
void eeprom_lock();

/**
 * Wait until programming is finished.
 * Not necessary on devices with no RWW support.
 */
void eeprom_wait_busy();

void eeprom_write(uint8_t *Buf, uint8_t offset, uint8_t len);
void eeprom_read(uint8_t *Buf, uint8_t offset, uint8_t len);

#endif /* EEPROM_H */
