// Texas Instruments BQ27541 Driver. 
// Coded by TinLethax 2022/12/29 +7
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>
#include <usart.h>
#include <i2c.h>

// Device address (7-bit)
#define BQ27541_DEV_ADDR	0x55 //7-bit address.

// Commands

#define CMD_CONTROL			0x00 // Control command (requires sub command).
#define CMD_Temp			0x06 // Temperature read from Thermistor (unsigned int in units of .1 Kelvin).
#define CMD_Voltage			0x08 // cell voltage (unsigned int from 0 to 6000 mV).
#define CMD_Flags			0x0A // Flags 
#define CMD_NomCapa			0x0C // Norminal Available capacity.
#define CMD_FullCapa		0x0E // Full available capacity.
#define CMD_RemainCapa		0x10 // Remaining Capacity.
#define CMD_AVGcurrent		0x14 // Average current in mA uint (signed int).
#define CMD_CycleCnt		0x2A // Cycle count (unsigned int from 0 to 65535).
#define CMD_SoC				0x2C // Stage of Charge in percentage 0 to 100%

// Subcommands for CMD_CONTROL 
#define SUB_CONTROLSTAT		0x0000
#define SUB_DEVTYPE			0x0001
#define SUB_CHEMID			0x0008
#define SUB_SEALED			0x0020

// Unseal key 
#define UNSEAL_KEY1			0x0414
#define UNSEAL_KEY2			0x3672

// Extended command for accessing Flash memory
#define EXTCMD_DFClass		0x3E	// Select flash class
#define EXTCMD_DFBlock		0x3F 	// Select flash block (physical_address / 32)
#define EXTCMD_BlkData		0x40	// Access entire block data (32 bytes).
#define EXTCMD_BlkSum		0x60	// Write data checksum to complete block Data update.
#define EXTCMD_BlkCtrl		0x61	// Block data control (write 0x00 in UNSEALED mode to access flash).


int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

void bq27541_write_reg(uint8_t reg, uint16_t value){
	i2c_start();// Generate start condition.
	
	i2c_write_addr(BQ27541_DEV_ADDR);// Request write to BQ27541.
	i2c_write(reg);// Write address to BQ27541.
	i2c_write((uint8_t)value);
	i2c_write((uint8_t)(value >> 8));
	
	i2c_stop();// Generate stop condition
}

void bq27541_write_reg8(uint8_t reg, uint8_t value){
	i2c_start();// Generate start condition.
	
	i2c_write_addr(BQ27541_DEV_ADDR);// Request write to BQ27541.
	i2c_write(reg);// Write address to BQ27541.
	i2c_write((uint8_t)value);
	
	i2c_stop();// Generate stop condition
}

uint16_t bq27541_read_reg(uint8_t reg){
	
	uint8_t temp[2];
	
	i2c_start();// Generate start condition.
	
	i2c_write_addr(BQ27541_DEV_ADDR);// Request write to BQ27541.
	i2c_write(reg);// Write address to BQ27541.
	
	i2c_start();// Regenerate start condition.
	
	i2c_read_addr(BQ27541_DEV_ADDR);// Request read from BQ27541.
	i2c_readPtr(temp, 2);
	
	i2c_stop();// Generate stop condition.
	
	printf("DEBUG READ : MSB 0x%02X LSB 0x%02X\n", temp[1], temp[0]);
	
	return temp[0] | (temp[1] << 8);
}

uint8_t bq27541_unseal(){
	
	uint16_t temp = 0;
	
	bq27541_write_reg(CMD_CONTROL, UNSEAL_KEY1);// Write Key 1.
	bq27541_write_reg(CMD_CONTROL, UNSEAL_KEY2);// Write Key 2.
	
	// check seal status 
	bq27541_write_reg(CMD_CONTROL, SUB_CONTROLSTAT);// Check control status.
	
	temp = bq27541_read_reg(CMD_CONTROL);
	
	printf("Control status dumped : 0x%04X", temp);
	
	// Return 0 -> Unsealed 
	// Return 1 -> (probably) still sealed
	
	return (temp & (1 << 14)) ? 0 : 1;// Check FAS (Full Access Sealed) bit.
}

uint8_t bq27541_seal(){
	uint16_t temp = 0;
	
	bq27541_write_reg(CMD_CONTROL, SUB_SEALED);// Write command to SEAL
	
	// Check seal status 
	bq27541_write_reg(CMD_CONTROL, SUB_CONTROLSTAT);// Check control status.
	
	temp = bq27541_read_reg(CMD_CONTROL);
	
	printf("Control status dumped : 0x%04X", temp);
	
	// Return 0 -> Sealed
	// Return 1 -> (probably) sill unsealed
	
	return (temp & (1 << 13)) ? 0 : 1;// Chec SS (SEALED state) bit.
}

uint8_t bq27541_check_seal(){
	uint16_t temp = 0;
	
	// check seal status 
	bq27541_write_reg(CMD_CONTROL, SUB_CONTROLSTAT);// Check control status.
	
	temp = bq27541_read_reg(CMD_CONTROL);
	
	printf("Control status dumped : 0x%04X", temp);
	
	// Return 0 -> Unsealed 
	// Return 1 -> (probably) still sealed
	
	return (temp & (1 << 14)) ? 0 : 1;// Check FAS (Full Access Sealed) bit.
}

uint8_t bq27541_probe(){
	uint8_t returnValue = 1;

	i2c_start();
	I2C1_DR = (BQ27541_DEV_ADDR << 1) & 0xFE;// send address to I2C.
	delay_ms(2);
	if(I2C1_SR1 & 0x02){// if address found, return error 0
		returnValue = 0;
	}
	i2c_stop();

	return returnValue;
}

uint8_t bq27541_init(){
	// Probe for chip
	if(bq27541_probe())
		return 1;
	
	// Check device type
	bq27541_write_reg(CMD_CONTROL, SUB_DEVTYPE);// Check Device type
	
	if(bq27541_read_reg(CMD_CONTROL) != 0x0541)
		return 2;


  return 0;
}

uint16_t bq27541_voltage(){// Voltage in mV unit
	return (uint16_t)bq27541_read_reg(CMD_Voltage); 
}

uint16_t bq27541_temp(){// Temperature in Kelvin unit)
	return bq27541_read_reg(CMD_Temp) / 10;// Convert 0.1 Kelvin to Kelvin unit
	
}

int16_t bq27541_current(){// Current in mA unit (signed int).
	return (int16_t)bq27541_read_reg(CMD_AVGcurrent);
}

uint16_t bq27541_cycles(){// Charge cycle (0 to 65535)
	
	return bq27541_read_reg(CMD_CycleCnt);
}

uint8_t bq27541_soc(){// Stage of Charge (battery percentage 0 to 100%)
  
	return (uint8_t)(bq27541_read_reg(CMD_SoC) >> 8);
	
}

uint16_t bq27541_fullcap(){// Full capacity in mAh unit
	
	return bq27541_read_reg(CMD_FullCapa);
	
}

uint16_t bq27541_remaincap(){// remain capacity in mAh unit
	
	return bq27541_read_reg(CMD_RemainCapa);
	
}

uint8_t bq27541_open_flash(){
	
	if(bq27541_unseal())// Unseal the chip
		return 1;// return in case of fail UNSEAL
		
	bq27541_write_reg8(EXTCMD_BlkCtrl, 0x00);// Unlock flash memory
	
	return 0;
}

uint8_t bq27541_read_flash(uint8_t class_id, uint8_t block_num, uint8_t * buf){
	
	if(bq27541_check_seal())
		return 1;// return 1 in case of SEALED status
	
	bq27541_write_reg8(EXTCMD_DFClass, class_id);// Access the specific class.
	bq27541_write_reg8(EXTCMD_DFBlock, block_num);// Access the specific block.
	
	i2c_start();// Generate start condition.
	
	i2c_write_addr(BQ27541_DEV_ADDR);// Request write to BQ27541.
	i2c_write(EXTCMD_BlkData);// Write address to BQ27541.
	
	i2c_start();// Regenerate start condition.
	
	i2c_read_addr(BQ27541_DEV_ADDR);// Request read from BQ27541.
	i2c_readPtr(buf, 32);
	
	i2c_stop();// Generate stop condition.
	
	return 0;
}

uint8_t bq27541_write_flash(uint8_t class_id, uint8_t block_num, uint8_t * buf){
	
	uint8_t chksum = 0;
	
	if(bq27541_check_seal())
		return 1;// return 1 in case of SEALED status
	
	// Pre-calculate checksum
	for(uint8_t n = 0; n < 32; n++)
		chksum += *(buf + n);// summing
	
	chksum -= 255;// complement
	
	bq27541_write_reg8(EXTCMD_DFClass, class_id);// Access the specific class.
	bq27541_write_reg8(EXTCMD_DFBlock, block_num);// Access the specific block.

	/* Write Data to flash*/

	i2c_start();// Generate start condition.
	
	i2c_write_addr(BQ27541_DEV_ADDR);// Request write to BQ27541.
	i2c_write(EXTCMD_BlkData);// Write address to BQ27541.
	i2c_writePtr(buf, 32);
	
	i2c_stop();// Generate stop condition
	
	
	bq27541_write_reg8(EXTCMD_BlkSum, chksum);
	
	return 0;
	
}

void main() {
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);// clear terminal
	usart_write(0x0C);
	i2c_init(0x00, I2C_100K);// Set I2C master address to 0x00, I2C speed to 100kHz.
	printf("BQ27541 on STM8L by TinLethax\n");
	uint8_t error = bq27541_init();
	printf("Init error code : %u\n", error);
	if(error != 0)
		while(1);

    while (1) {
		printf("Voltage : %u mV\n", bq27541_voltage());
		printf("Current : %d mA\n", bq27541_current());
		printf("State of charge : %u %%\n", bq27541_soc());
		printf("Temperatur : %u K", bq27541_temp());
		delay_ms(1000);
		usart_write(0x0C);
    }
}
