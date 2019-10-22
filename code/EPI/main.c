/* By TinLethax, Require EPIstm8l library from my repo, Please follow the instruction in that repo to install the library */
#include <stdio.h>
#include <stdint.h>
#include <stm8l.h>
#include <delay.h>
#include <EPI.h>
#include "BMP.h"

void EPI_gpio_init();

void main() {
	EPI_gpio_init();
    while (1) {
		Full_updata_setting();
		Full_image(gImage_1);
		delay_ms(25000);
		//Clean
		Full_image_Clean();
		delay_ms(1000);
		//Part
		Part_updata_setting();
		Part_image_first(gImage_2);		  //Part_image1 
		delay_ms(15000);
		Part_image(gImage_2,gImage_3);  //Part_image2
		delay_ms(15000);

		Part_image(gImage_3,gImage_4);	//Part_image3 
		delay_ms(25000);
		
    //Clean
		Full_updata_setting();
    		Full_image_Clean();

    }
}
