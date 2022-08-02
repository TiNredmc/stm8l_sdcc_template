/* By TinLethax, Require EPIstm8l library from my repo, Please follow the instruction in that repo to install the library */
#include <stdio.h>
#include <stdint.h>
#include <stm8l.h>
#include <delay.h>
#include <EPI.h>
#include "BMP.h"
#include <usart.h>

void prntf(char *c){
	while(*c)
		usart_write(*c++);
}

uint8_t readback;

void main() {
	CLK_CKDIVR = 0x00;
	usart_init(9600); // usart using baudrate at 9600
	//remap the non-exist pin of Tx and Rx of the UFQFPN20 package to the exist one.
	SYSCFG_RMPCR1 |= 0x10;
	EPI_gpio_init();
	
	/*prntf(" Entering Full update\n");
	Full_updata_setting();
	prntf("Full update setting done\n");*/
	//EPI_init();
	delay_ms(1000);
	prntf("Display seal status check\n");
	EPI_read(0x44, &readback, 1);// send command 0x44 and read it back 
	prntf("read back is :");
	if(readback == 0xFF)
		prntf("Read error");
	else
		prntf("It's something else");
	
	usart_write(readback);
	prntf("\nDone");
	
    while (1) {
		/*
		Full_image(gImage_1);
		delay_ms(3000);
		//Clean
		Full_image_Clean();
		delay_ms(1000);

*/
    }
}
