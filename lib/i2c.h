/* I2C implementations on STM8L, All EV number is the event number. took from the Standard Periherial Library
 * Coded by TinLethax
 */
 
#ifndef I2C_H
#define I2C_H

#include <stdint.h>

// select the SPI speed 
#define I2C_100K	10
#define I2C_400K	40

void i2c_init(uint8_t devID, uint8_t SCLSpeed);
uint8_t i2c_ChkEv(uint16_t I2C_Event); 
void i2c_start();
void i2c_stop();
void i2c_write(uint8_t data);
void i2c_writePtr(uint8_t *data, uint16_t len);
void i2c_writePtrAuto(uint8_t *data);
uint8_t i2c_read();
void i2c_readPtr(uint8_t *data, uint16_t len);
void i2c_write_addr(uint8_t addr);
void i2c_read_addr(uint8_t addr);

#endif 
