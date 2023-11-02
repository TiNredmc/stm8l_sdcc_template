// IMX316 Set file A example code with STM8L
// Ported by TinLethax 2023/10/31 +7
#include <stm8l.h>
#include <i2c.h>
#include <usart.h>
#include <delay.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "imx316_setfile_a.h"

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

// I2C address of the IMX317
#define IMX316_ADDR  0x1A

// Read data from IMX316
void imx_read_reg(uint16_t addr, uint8_t *data, uint16_t len) {

	i2c_start();// Generate start condition.
	
	i2c_write_addr(IMX316_ADDR);
	i2c_write((uint8_t)(addr >> 8));// Write MSB byte address.
	i2c_write((uint8_t)addr);// Write LSB byte address.
	
	i2c_start();// Regenerate start condition.
	
	i2c_read_addr(IMX316_ADDR);
	i2c_readPtr(data, len);
	
	i2c_stop();// Generate stop condition.

}

// Write data to IMX316
void imx_write_reg(uint16_t addr, uint8_t data) {

	i2c_start();// Generate start condition.
	
	i2c_write_addr(IMX316_ADDR);
	i2c_write((uint8_t)(addr >> 8));// Write MSB byte address.
	i2c_write((uint8_t)addr);// Write LSB byte address.
	i2c_write(data);
	
	i2c_stop();// Generate stop condition

}

uint8_t laser_on_flag = 0xAA;
void imx_set_laser_ctrl(uint8_t laser_on){
	
	if(laser_on == 0){
		laser_on_flag = 0xAA;
		
	}else{
		laser_on_flag = 0x00;
		
	}
	
	imx_write_reg(0x2134, laser_on_flag);
	imx_write_reg(0x2135, laser_on_flag);
}

uint8_t imx_get_laser_ctrl(){

	return laser_on_flag ? 0 : 1;
}

void imx_set_laser_current(uint8_t current_val){
	
	for(uint16_t idx = 0; idx < IMX316_CURRENT_SETTING_ARRAY_SIZE; idx+=1){ 
		imx_write_reg(current_setting_setfile_a_reg[idx], current_setting_setfile_a_val[idx]);
	}
	
	// Set Auto Power Control (APC)
	imx_write_reg(0x0436, 0x00);
	imx_write_reg(0x0437, 0x00);
	imx_write_reg(0x043A, 0x02);
	imx_write_reg(0x0500, 0x02);
	imx_write_reg(0x0501, 0x08);
	imx_write_reg(0x0502, current_val);
	
	imx_write_reg(0x0431, 0x01);
	imx_write_reg(0x0430, 0x01);
	imx_write_reg(0x0431, 0x00);
	
}

void imx_set_auto_exposure(uint16_t exposure){
	uint32_t exposure_set;
	if(exposure < 7 || exposure > 1000)
		return;
	
	exposure_set = 120 * exposure;
	
	// integ_clk
	imx_write_reg(0x2110, (uint8_t)(exposure_set >> 24));
	imx_write_reg(0x2111, (uint8_t)(exposure_set >> 16));
	imx_write_reg(0x2112, (uint8_t)(exposure_set >> 8));
	imx_write_reg(0x2113, (uint8_t)(exposure_set));
}

void imx_set_init(){
	uint16_t temp = 0;
	// Global Setfile A
	for(uint16_t idx = 0; idx < IMX316_GLOBAL_SET_FILE_ARRAY_SIZE; idx += 1){
		imx_write_reg(global_setfile_a_reg[idx], global_setfile_a_val[idx]);
	}
	
	// Front ToF Setfile A
	for(uint16_t idx = 0; idx < IMX316_SET_FILE_A_TOP_SIZE; idx += 1){
		imx_write_reg(setfile_a_top_reg[idx], setfile_a_top_val[idx]);
	}
	delay_ms(100);
	for(uint16_t idx = 0; idx < IMX316_SET_FILE_A_BOT_SIZE; idx += 1){
		imx_write_reg(setfile_a_bot_reg[idx], setfile_a_bot_val[idx]);
	}
	
	imx_read_reg(0x0001, (uint8_t *)&temp, 2);
	printf("ID 0x%04X\n", temp);
}

#define FPS_30	0
#define FPS_20	2
#define FPS_15	4
#define FPS_10	6
#define FPS_5	8
const static uint8_t fps_lut[10]={
	0x03, 0x68,	
	0x08, 0x7E,
	0x92, 0x0D,
	0x17, 0xBC,
	0x36, 0x3C
};

void imx_set_fps(uint8_t fps){
	
	imx_write_reg(0x0104, 0x01);// Turn on Group hold
	imx_write_reg(0x2154, fps_lut[fps]);
	imx_write_reg(0x2155, fps_lut[fps+1]);
	imx_write_reg(0x0104, 0x00);// Turn off Group hold
	
}

void imx_set_stream(uint8_t on_off){
	
	if(on_off == 0){
		imx_write_reg(0x1001, 0x00);
	}else{
		imx_write_reg(0x1001, 0x01);
	}
	
}

void imx_get_depth_map(){
	uint8_t depth_map_num;
	imx_read_reg(0x152C, &depth_map_num, 1);
	printf("Depth map : 0x%02X\n", depth_map_num);
}

void main(){
	usart_init(115200); // usart using baudrate at 115107(actual speed)
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);// clear terminal
	usart_write(0x0C);
	i2c_init(0x69, I2C_100K);// Set I2C master address to 0x69, I2C speed to 100kHz.
	printf("IMX316 Set file A on STM8L by TinLethax\n");

	imx_set_init();
	imx_set_laser_current(127);
	imx_set_laser_ctrl(1);
	imx_set_fps(FPS_5);
	imx_set_stream(1);
	
	printf("Done!\n");
	
	while(1){
		imx_get_depth_map();
		delay_ms(100);
	}
}
