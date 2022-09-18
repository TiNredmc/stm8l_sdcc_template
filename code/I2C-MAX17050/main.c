// MAX17050 Driver. 
// Ported from https://android.googlesource.com/kernel/msm/+/android-msm-dory-3.10-kitkat-wear/drivers/power/max17050_battery.c
// Ported by TinLethax 2022/9/08 +7
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
#include <usart.h>
#include <i2c.h>

// Device address (7-bit)
#define MAX17050_DEV_ADDR	0x36 //7-bit address.

#define CMD_STATUS	0x00 // Chip status.
#define CMD_VCELL	0x09 // Cell voltage.
#define CMD_CURRNT	0x0A // Current draw.
#define CMD_SOC		0x0D // State of charge (Battery percentage).
#define CMD_REMCAP	0x0F // Remaining Capacity.
#define CMD_FULLCAP	0x10 // Full capacity.
#define CMD_CYCLES	0x17 // Battery cycle.
#define CMD_DEFCAP	0x18 // Designed capacity.
#define CMD_CONF	0x1D // COnfig register.

#define CMD_VERSION  0x21 // Fuel gauge version


// Change according to your battery capacity.
// Calculation fomular
// BAT_FULLCAP = Battery capacity (mAh) * sens resistor value (Ohm).
// Example :
// BAT_FULLCAP = 500 mAh * 0.010 Ohm <- This code has sens resistor fixed as 10mOhm
// So BAT_FULLCAP = 5
#define BAT_FULLCAP 1 // 100mAh battery

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

void max17050_write_reg(uint8_t reg, uint16_t value){
	i2c_start();// Generate start condition.
	
	i2c_write_addr(MAX17050_DEV_ADDR);// Request write to MAX17050.
	i2c_write(reg);// Write address to MAX17050.
	i2c_write((uint8_t)value);
	i2c_write((uint8_t)(value >> 8));
	
	i2c_stop();// Generate stop condition
}

uint16_t max17050_read_reg(uint8_t reg){
	
	uint8_t temp[2];
	
	i2c_start();// Generate start condition.
	
	i2c_write_addr(MAX17050_DEV_ADDR);// Request write to MAX17050.
	i2c_write(reg);// Write address to MAX17050.
	
	i2c_start();// Regenerate start condition.
	
	i2c_read_addr(MAX17050_DEV_ADDR);// Request read from MAX17050.
	i2c_readPtr(temp, 2);
	
	i2c_stop();// Generate stop condition.
	
	printf("DEBUG READ : MSB 0x%02X LSB 0x%02X\n", temp[1], temp[0]);
	
	return temp[0] | (temp[1] << 8);
}

uint8_t max17050_write_verify_reg(uint8_t reg, uint16_t value){
	uint8_t temp[2];
	max17050_write_reg(reg, value);
	
	i2c_start();// Regenerate start condition.
	
	i2c_read_addr(MAX17050_DEV_ADDR);// Request read from MAX17050.
	i2c_readPtr(temp, 2);
	
	i2c_stop();// Generate stop condition.
	
	if((temp[0] | (temp[1] << 8)) != value)
		return 1;
	
	return 0;
	
}

uint8_t max17050_probe(){
	uint8_t returnValue = 1;

	i2c_start();
	I2C1_DR = (MAX17050_DEV_ADDR << 1) & 0xFE;// send address to I2C.
	delay_ms(2);
	if(I2C1_SR1 & 0x02){// if address found, shout out the address.
		returnValue = 0;
	}
	i2c_stop();

	return returnValue;
}

uint8_t max17050_init(){
	// Probe for chip
	if(max17050_probe())
		return 1;
	
	// Check device version
	if(max17050_read_reg(CMD_VERSION) == 0)
		return 2;

	// Set the Designed capacity of the battery
	if(max17050_write_verify_reg(CMD_DEFCAP, (uint16_t)BAT_FULLCAP))
		return 3;

  return 0;
}

uint16_t max17050_voltage(){
	
	uint32_t volt = max17050_read_reg(CMD_VCELL);
	
	volt *= 625;// Convert raw word into microvolt unit (uV)
	volt >>= 3;// Get rid of unused bit
	volt /= 1000;// Convert microvolt (uV) to millivolt (mV)
	
	return (uint16_t)volt; 
}

int32_t max17050_current(){
	int32_t real_current = 0;

	real_current = (int16_t)max17050_read_reg(CMD_CURRNT);
	real_current *= 1562;
	
	return real_current / (int32_t)10;
}

uint16_t max17050_cycles(){
	
	return max17050_read_reg(CMD_CYCLES) / 100;
}

uint8_t max17050_soc(){
  
	return (uint8_t)(max17050_read_reg(CMD_SOC) >> 8);
	
}

uint16_t max17050_fullcap(){// Full capacity 
	
	return max17050_read_reg(CMD_FULLCAP) / 10000;
	
}

uint16_t max17050_remaincap(){// remain capacity
	
	return max17050_read_reg(CMD_REMCAP) / 10000;
	
}

void main() {
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);// clear terminal
	usart_write(0x0C);
	i2c_init(0x00, I2C_100K);// Set I2C master address to 0x00, I2C speed to 100kHz.
	printf("MAX17050 on STM8L by TinLethax\n");
	uint8_t error = max17050_init();
	printf("Init error code : %u\n", error);

    while (1) {
		printf("Voltage : %u mV\n", max17050_voltage());
		printf("Current : %ld uA\n", max17050_current());
		printf("State of charge : %u %%", max17050_soc());
		delay_ms(1000);
		usart_write(0x0C);
    }
}
