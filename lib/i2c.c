/* I2C implementations on STM8L, All EV number is the event number. took from the Standard Periherial Library
 * Coded by TinLethax
 */
#include <stm8l.h>
#include <i2c.h>  

void i2c_init(uint8_t devID, uint8_t SCLSpeed) { // init I2C with Own Device address (Master address) and I2C Speed 
    CLK_CKDIVR = 0x00; // No clock divider
    CLK_PCKENR1 |= 0x08;// enable the I2C clock 
    I2C1_FREQR |= F_CPU/1000000 ;// Use peritpheral clock to generate I2C waveform, for example 16MHz/10^6
	
    I2C1_CR1 &= ~0x01;// cmd disable for i2c configurating

	if (SCLSpeed == I2C_400K){// 400KHz
    I2C1_TRISER |= 5;// Max 300ns rise time.

    I2C1_CCRL = 0x0D;// the clock speed lower bits
    I2C1_CCRH = 0x80;// the clock speed higher bits + enable High speed mode (400KHz)
	}else{// 100KHz 
    I2C1_TRISER |= 16 + 1 ;// I2C1_FREQR + 1 max 1000ns rise time.

    I2C1_CCRL = 0x50;// the clock speed lower bits
    I2C1_CCRH = 0x00;// the clock speed higher bits
	}

    I2C1_CR1 |= 0x01;// i2c mode not SMBus
	
    I2C1_OARL = devID;// own address of Master 
    I2C1_OARH = 0x00; 
	
    I2C1_CR1 |= 0x01;// cmd enable
}

void i2c_enableIt(){// Enable Interrupt
	I2C1_ITR |= (1 << 0) | (1 << 1) | (1 << 2);// enable interrupt (buffer, event an error interrupt)
}

uint8_t i2c_ChkEv(uint16_t I2C_Event)// event check, VERY CRUCIAL part, otherwise I2C won't work properly 
{
  volatile uint16_t lastevent = 0x00;
  uint8_t flag1 = 0x00;
  uint8_t flag2 = 0x00;
    flag1 = I2C1_SR1;
    flag2 = I2C1_SR3;
    lastevent = ((uint16_t)(flag2 << 8) | (uint16_t)flag1);
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

void i2c_writePtr(uint8_t *data, uint16_t len){// Write single or sequencial byte with pre defined size.
	while(len--){
		I2C1_DR = *data++;
	while (!i2c_ChkEv(0x0780));// EV8	
	}
}

void i2c_writePtrAuto(uint8_t *data){// Write single or sequencial byte with auto increment
	while(*data){
		I2C1_DR = *data++;
	while (!i2c_ChkEv(0x0780));// EV8	
	}
}
uint8_t i2c_read(){// Read data
	I2C1_CR2 |= (1 << I2C1_CR2_ACK);
    while (!i2c_ChkEv(0x0340));// EV7
    return (uint8_t)I2C1_DR;
}

void i2c_readPtr(uint8_t *data, uint16_t len){
	while(len-- > 1){
		I2C1_CR2 |= (1 << I2C1_CR2_ACK);
		while (!i2c_ChkEv(0x0340));// EV7
		*(data++) = I2C1_DR;
	}
	while (!i2c_ChkEv(0x0340));// EV7
	*(data++) = I2C1_DR;
	I2C1_CR2 &= ~(1 << I2C1_CR2_ACK);
}

void i2c_write_addr(uint8_t addr) {//request for Write with 7bit address (you need to put 8 bit address, like 0x5A (MLX90614))
 I2C1_DR = (addr << 1) & 0xFE;
 while (!i2c_ChkEv(0x0782));// EV6
}

void i2c_read_addr(uint8_t addr) {//request for Read with 7bit address (you need to put 8 bit address, like 0x5A (MLX90614))
 I2C1_DR = (addr << 1)| 0x01;
 while (!i2c_ChkEv(0x0302));// EV6
}
