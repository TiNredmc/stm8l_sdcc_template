
/* 
* This file is part of VL53L1 Platform 
* 
* Copyright (c) 2016, STMicroelectronics - All Rights Reserved 
* 
* License terms: BSD 3-clause "New" or "Revised" License. 
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met: 
* 
* 1. Redistributions of source code must retain the above copyright notice, this 
* list of conditions and the following disclaimer. 
* 
* 2. Redistributions in binary form must reproduce the above copyright notice, 
* this list of conditions and the following disclaimer in the documentation 
* and/or other materials provided with the distribution. 
* 
* 3. Neither the name of the copyright holder nor the names of its contributors 
* may be used to endorse or promote products derived from this software 
* without specific prior written permission. 
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
* 
*/

#include "vl53l1_platform.h"
#include <i2c.h>
#include <delay.h>

// dev is device i2c (8 bit) address, in this case is 0x52
// index is device's memory address 
// pdata is the single byte data or data array
// count is for multiple bytes 

uint8_t VL53L1_WriteMulti(uint8_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
	i2c_start(); //generate start condition

	i2c_write_addr(dev); // request for write to specific i2c address
	i2c_write((uint8_t)(index & 0xFF00));// send the specifix memory address that we want to do something with 
	i2c_write((uint8_t)index);// send the specifix memory address that we want to do something with

	i2c_stop(); //generate stop before next round

	for (int i=0;i < count;i++){// sequential writing
	i2c_write(*pdata);	// send multiple bytes
	*pdata++;// move to next byte on array
	}	
	i2c_stop(); //generate stop condition
	return 0; // to be implemented
}

uint8_t VL53L1_ReadMulti(uint8_t dev, uint16_t index, uint8_t *pdata, uint32_t count){
	i2c_start(); //generate start condition

	i2c_write_addr(dev); // request for write to specific i2c address
	i2c_write((uint8_t)(index & 0xFF00));// send the specifix memory address that we want to do something with 
	i2c_write((uint8_t)index);// send the specifix memory address that we want to do something with

	i2c_stop(); //generate stop before next round
	
	i2c_start(); //generate start for sequential reading

	i2c_read_addr(dev); // request for read from specific i2c address
	for (int i=0;i < count;i++){
	pdata = &i2c_read();	// send multiple bytes
	*pdata++;// move to next byte on array
	}	
	i2c_stop(); //generate stop condition
	return 0; // to be implemented
}

uint8_t VL53L1_WrByte(uint8_t dev, uint16_t index, uint8_t data) {
	i2c_start(); //generate start condition

	i2c_write_addr(dev); // request for write to specific i2c address
	i2c_write((uint8_t)(index & 0xFF00));// send the specifix memory address that we want to do something with 
	i2c_write((uint8_t)index);// send the specifix memory address that we want to do something wit

	i2c_write(data);	// send single byte

	i2c_stop(); //generate stop condition
	return 0; // to be implemented
}

uint8_t VL53L1_WrWord(uint8_t dev, uint16_t index, uint16_t data) {
	i2c_start(); //generate start condition

	i2c_write_addr(dev); // request for write to specific i2c address
	i2c_write((uint8_t)(index & 0xFF00));// send the specifix memory address that we want to do something with 
	i2c_write((uint8_t)index);// send the specifix memory address that we want to do something with

	i2c_stop(); //generate stop before next round

	i2c_write((uint8_t)(data & 0xFF00));// send MSB first
	i2c_write((uint8_t)(data & 0x00FF));// Then LSB last		

	i2c_stop(); //generate stop condition
	return 0; // to be implemented
}

uint8_t VL53L1_WrDWord(uint8_t dev, uint16_t index, uint32_t data) {
	i2c_start(); //generate start condition

	i2c_write_addr(dev); // request for write to specific i2c address
	i2c_write((uint8_t)(index & 0xFF00));// send the specifix memory address that we want to do something with 
	i2c_write((uint8_t)index);// send the specifix memory address that we want to do something with

	i2c_stop(); //generate stop before next round

	i2c_write((uint8_t)(data & 0xFF000000));// send MSB first 		
	i2c_write((uint8_t)(data & 0x00FF0000));// then data +1 
	i2c_write((uint8_t)(data & 0x0000FF00));// also data +2
	i2c_write((uint8_t)(data & 0x000000FF));// finally, LSB  

	i2c_stop(); //generate stop condition
	return 0; // to be implemented
}

uint8_t VL53L1_RdByte(uint8_t dev, uint16_t index, uint8_t *pdata) {
	i2c_start(); //generate start condition

	i2c_write_addr(dev); // request for write to specific i2c address
	i2c_write((uint8_t)(index & 0xFF00));// send the specifix memory address that we want to do something with 
	i2c_write((uint8_t)index);// send the specifix memory address that we want to do something with

	i2c_stop(); //generate stop before next round
	
	i2c_start(); //generate start for sequential reading

	i2c_read_addr(dev); // request for read from specific i2c address
	pdata = &i2c_read();	// read single byte

	i2c_stop(); //generate stop condition
	return 0; // to be implemented
}

uint8_t VL53L1_RdWord(uint8_t dev, uint16_t index, uint16_t *pdata) {
	i2c_start(); //generate start condition

	i2c_write_addr(dev); // request for write to specific i2c address
	i2c_write((uint8_t)(index & 0xFF00));// send the specifix memory address that we want to do something with 
	i2c_write((uint8_t)index);// send the specifix memory address that we want to do something with

	i2c_stop(); //generate stop before next round
	uint8_t buff[2];
	i2c_start(); //generate start for sequential reading

	i2c_read_addr(dev); // request for read from specific i2c address
	buff[0] = i2c_read();
	buff[1] = i2c_read();
	i2c_stop(); //generate stop condition

	*pdata = *((uint16_t*) buff);
	return 0; // to be implemented
}

uint8_t VL53L1_RdDWord(uint8_t dev, uint16_t index, uint32_t *pdata) {
	i2c_start(); //generate start condition

	i2c_write_addr(dev); // request for write to specific i2c address
	i2c_write((uint8_t)(index & 0xFF00));// send the specifix memory address that we want to do something with 
	i2c_write((uint8_t)index);// send the specifix memory address that we want to do something with

	i2c_stop(); //generate stop before next round
	uint8_t buff[4];
	i2c_start(); //generate start for sequential reading

	i2c_read_addr(dev); // request for read from specific i2c address
	buff[0] = i2c_read();
	buff[1] = i2c_read();
	buff[2] = i2c_read();
	buff[3] = i2c_read();
	i2c_stop(); //generate stop condition
	*pdata = *((uint32_t*) buff);
	return 0; // to be implemented
}

