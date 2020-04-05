/* I2C implementations on STM8L, All EV number is the event number. took from the Standard Periherial Library
 * Coded by TinLethax
 */
#include "stm8l.h"
#include "i2c.h"  

uint16_t SCLSpeed = 0x00A0; // 50kHz
void i2c_init(uint8_t devID) { // init I2C with Device address 
    CLK_CKDIVR = 0x00; // No clock divider
    I2C1_FREQR |= F_CPU/1000000 ;// 16MHz/10^6
	
    I2C1_CR1 &= ~0x01;// cmd disable for i2c configurating

    I2C1_TRISER |= (uint8_t)(0);// 0 in decimal, Very Short Riser Time (suitable for MLX90614, That why i came up with this library)

    I2C1_CCRL = (uint8_t)SCLSpeed;
    I2C1_CCRH = (uint8_t)((SCLSpeed >> 8) & 0x0F);

    I2C1_CR1 |= (0x00 | 0x01);// i2c mode not SMBus
	
    I2C1_OARL = (uint8_t)(devID);
    I2C1_OARH = (uint8_t)((uint8_t)(0x00 | 0x40 ) | (uint8_t)((uint16_t)( (uint16_t)devID &  (uint16_t)0x0300) >> 7)); 
	
    I2C1_CR1 |= (1 << 0);// cmd enable
}

uint8_t i2c_ChkEv(uint16_t I2C_Event)// event check, VERY CRUCIAL part, otherwise I2C won't work properly 
{
  volatile uint16_t lastevent = 0x00;
  uint8_t flag1 = 0x00;
  uint8_t flag2 = 0x00;
    flag1 = I2C1_SR1;
    flag2 = I2C1_SR3;
    lastevent = ((uint16_t)((uint16_t)flag2 << (uint16_t)8) | (uint16_t)flag1);
  /* Check whether the last event is equal to I2C_EVENT */
  /* Return status */
  return (((uint16_t)lastevent & (uint16_t)I2C_Event) == (uint16_t)I2C_Event) ;
}

void i2c_start() { // Generate Start condition
    I2C1_CR2 |= 0x01; // start and ack
    while (!i2c_ChkEv(0x0301));// EV5
}
void i2c_stop() { // Generate Stop condition
    I2C1_CR2 |= 0x02;
}

void i2c_write(uint8_t data) {// Write data
    I2C1_DR = data;
    while (!i2c_ChkEv(0x0780));// EV8
}
uint8_t i2c_read(){// Read data
    while (!i2c_ChkEv(0x0340));// EV7
    return (uint8_t)I2C1_DR;
}

void i2c_write_addr(uint8_t addr) {//request for Write with 7bit address (you need to put 8 bit address, like 0x5A (MLX90614))
 I2C1_DR = (uint8_t)((addr << 1) & 0xFE);
 while (!i2c_ChkEv(0x0782));// EV6
}

void i2c_read_addr(uint8_t addr) {//request for Read with 7bit address (you need to put 8 bit address, like 0x5A (MLX90614))
 I2C1_DR = (uint8_t)((addr << 1)| 0x01);
 while (!i2c_ChkEv(0x0302));// EV6
}
