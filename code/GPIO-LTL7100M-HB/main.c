// LTL-7100M-HB with STM8L GPIO.
// Coded by TinLethax 2022/09/09 +7 

#include <stm8l.h>
#include <delay.h>

void GPIO_init(){
	PB_DDR |= 255; // Init all Port B GPIO as output.
	PB_CR1 |= 255; // with push-pull mode.
}


// Set LED color of each (4) segments
// 0 -> LED off
// 1 -> Green
// 2 -> Orange
void led_setColor(uint8_t a, uint8_t b, uint8_t c, uint8_t d){
	switch(a){
		case 0:
				PB_ODR &= ~(0x03);
				
			break;
			
		case 1:
				PB_ODR &= ~(0x03);
				PB_ODR |= (0x01);
				
			break;
			
		case 2:
				PB_ODR &= ~(0x03);
				PB_ODR |= (0x02);
		
			break;
	}
	
	switch(b){
		case 0:
				PB_ODR &= ~(0x0C);
				
			break;
			
		case 1:
				PB_ODR &= ~(0x0C);
				PB_ODR |= (0x04);
				
			break;
			
		case 2:
				PB_ODR &= ~(0x0C);
				PB_ODR |= (0x08);
		
			break;
	}
	
	switch(c){
		case 0:
				PB_ODR &= ~(0x30);
				
			break;
			
		case 1:
				PB_ODR &= ~(0x30);
				PB_ODR |= (0x10);
				
			break;
			
		case 2:
				PB_ODR &= ~(0x30);
				PB_ODR |= (0x20);
		
			break;
	}
	
	switch(d){
		case 0:
				PB_ODR &= ~(0xC0);
				
			break;
			
		case 1:
				PB_ODR &= ~(0xC0);
				PB_ODR |= (0x40);
				
			break;
			
		case 2:
				PB_ODR &= ~(0xC0);
				PB_ODR |= (0x80);
		
			break;
	}
	
}

void led_loadingGreen(){
		led_setColor(1, 0, 0, 0);
		delay_ms(150);
		led_setColor(0, 1, 0, 0);
		delay_ms(150);
		led_setColor(1, 0, 1, 0);
		delay_ms(100);
		led_setColor(0, 1, 0, 1);
		delay_ms(90);
		led_setColor(1, 0, 1, 1);
		delay_ms(80);
		led_setColor(0, 1, 1, 1);
		delay_ms(70);
		led_setColor(1, 1, 1, 1);
		delay_ms(500);
		
		led_setColor(1, 1, 1, 0);
		delay_ms(70);
		led_setColor(1, 1, 0, 1);
		delay_ms(80);
		led_setColor(1, 0, 1, 0);
		delay_ms(90);
		led_setColor(0, 1, 0, 1);
		delay_ms(100);
		led_setColor(0, 0, 1, 0);
		delay_ms(150);
		led_setColor(0, 0, 0, 1);
		delay_ms(150);
		led_setColor(0, 0, 0, 0);
		delay_ms(500);
}

void main(){
	CLK_CKDIVR = 0x00;
	GPIO_init();
	led_setColor(0, 0, 0, 0);
	led_setColor(1, 1, 1, 1);
	delay_ms(200);
	led_setColor(1, 2, 1, 2);
	delay_ms(200);
	led_setColor(2, 1, 2, 1);
	delay_ms(200);
	led_setColor(1, 2, 1, 2);
	delay_ms(200);
	led_setColor(2, 1, 2, 1);
	delay_ms(200);
	led_setColor(2, 2, 2, 2);
	delay_ms(200);
	while(1){
		led_loadingGreen();
	}
	
}