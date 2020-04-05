#include <stm8l.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <delay.h>
#include <usart.h>
#include <i2c.h>

uint16_t REMAP_Pin = 0x011C; 

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

// For Reading Temp in Kelvin and convert to Degree celcius
//////////////////////////////////////////////////

int16_t ReadTemp (){
	i2c_start();// generate start condition
	i2c_write_addr(devID); // request for register writing
	i2c_write(0x07);// write to 0x07 (Object Temperature request)
	i2c_start();// re-gen the start condition
	i2c_read_addr(devID); // request for reading
	uint8_t lsb = i2c_read(); // read lower bits
	uint8_t msb = i2c_read(); // read higher bits
	uint8_t pec = i2c_read(); // dummy reading the PEC
	i2c_stop();// gen the stop condition
	return ((((msb&0x007f)<<8)+lsb)*2)-27315; // Unit conversion from Kelvin to Celcius
}
//////////////////////////////////////////////////

void main(){
  usart_init(19200);
  SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); //remap the non-exit pin of Tx and Rx of the UFQFPN20 package to the exit one.
SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);
	int16_t temp = 0;
	i2c_init();
	while(1){
  temp = ReadTemp();
  /*float implementing... Please wait*/
	}// While loop
  
}// main 
