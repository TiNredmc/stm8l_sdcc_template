/* GPIOs manipulation for driving Circular ring LED found on eBay and Aliexpress
 * Coded by TinLethax, updated 2021/08/08 +7
 */
#include <stdint.h>
#include <stdbool.h>
#include <stm8l.h>
#include <delay.h>

uint8_t dataSet[4] = {0,0,0,0} ;// 4bit binary for displaying on the segment (1st block,2nd block,3rd block,4th block)
uint8_t scanarray[4] = {0x07,0x0B,0x0D,0x0E};// 0111 1011 1101 1110 (B7 left to B4 right), Scan Array
uint8_t mux = 0;// multiplexing counter from 0 to 3.
uint8_t bar[4] = {0, 1, 3, 7};// number for turning bar on orderly.

// Initialize app Port B GPIO.
void initLED(){
    PB_DDR |= 255;
    PB_CR1 |= 255;
  }

// Init TIM4 for Timer interrupt LED Muxxing.
void TIM4init(){
	CLK_PCKENR1 |= 0x04;// enable the TIM4 clock
	/* TIM4_TimeBaseInit(pscrVal, arrVal)*/
	TIM4_ARR = 125 - 1; //set prescaler period
	TIM4_PSCR = 0x07;// set prescaler 
	TIM4_EGR = 0x01;// event source update 
	/*TIM4_cmd(ENABLE)*/
	TIM4_CR1 |= 0x01 ;
	/*TIM4_ClearFlag()*/
	TIM4_SR = (uint8_t)~0x01;
	/*TIM4_ITConfig(TIM4_IT_Update, ENABLE)*/	
	TIM4_IER |= 0x01;// make sure that I enable interrupt for the TIM4
	/*TIM4_cmd(ENABLE)*/
	TIM4_CR1 |= 0x01 ;
}

/* Interrupt handler for TIM4 */
void TIM4isr(void) __interrupt(25){// the interrupt vector is 25 (from the STM8L151F36U datasheet page 49)
	if (mux > 3)// reset mux counter when it's 4th time.
		mux = 0;
	PB_ODR = (scanarray[mux] << 4) | dataSet[mux];
	mux++;
	/*TIM4_ITClearPendingBit()*/
	TIM4_SR = (uint8_t)~0x01;
}

// Clear Display.
void displayClr(){
	PB_ODR = 0xF0 ;// set PB4 to PB7 to 1 (clear the scanning)and set PB0 to PB3 to 0 (clear the segment).
	dataSet[0] = 0;
	dataSet[1] = 0;
	dataSet[2] = 0;
	dataSet[3] = 0;
}

// Display bar according to number.
void displayBar(uint8_t ds)// Dislay Progress bar
{
	if(ds > 15)// Only lit green bar (not with red LED);
		return;
	displayClr();
	if(ds < 4){// 0-3 
		dataSet[0] = bar[ds];
	}else if ((ds > 3) && (ds < 8)){// 4-7
		dataSet[0] = 0x0F;// Lit all first LEDs segment
		dataSet[1] = bar[ds%4];
	}else if ((ds > 7) && (ds < 12)){// 8-11
		dataSet[0] = 0x0F;
		dataSet[1] = 0x0F;
		dataSet[2] = bar[ds%4];
	}else{
		dataSet[0] = 0x0F;
		dataSet[1] = 0x0F;
		dataSet[2] = 0x0F;
		dataSet[3] = bar[ds%4];
	}
}

// Turn red dot on or off.
void displayRedDot(bool rd){
	if(rd)
		dataSet[3] |= (1 << 4);// LED red dot is controlled by bit 3 on dataSet[3].
	else
		dataSet[3] &= ~(1 << 4);
}

// Cool transition animation from number a to b (returning to 0)
void display_ani_a2b_r20(uint8_t Num1, uint8_t Num2){
	for(uint8_t i = 0;i < Num1;i++){
		displayBar(Num1--);
		delay_ms(4*Num1);
	}
	for(uint8_t i = 0;i < Num2; i++){
		displayBar(i+1);
		delay_ms(110/(i+1));
	}
}

// Another cool transition animation from number a to b (without returning to 0).
void display_ani_a2b(uint8_t Num1, uint8_t Num2){
	if(Num1 == Num2)
		return;
	
	if(Num1 > Num2){
		for(uint8_t i = Num1;i > Num2-1;i--){// moving backward
			displayBar(i);
			delay_ms(50);
		}
	}else{
		for(uint8_t i = Num1;i < Num2+1;i++){// moving forward 
			displayBar(i);
			delay_ms(50);
		}
	}	
}
uint8_t county = 0;
void main(){
	CLK_CKDIVR = 0x00;// get full 16MHz
	initLED();
	TIM4init();
	delay_ms(100);
	displayClr();	
	__asm__("rim");
	while (1){
		displayBar(15);
		delay_ms(1000);
		display_ani_a2b_r20(15, 7);
		displayRedDot(true);
		delay_ms(1000);
		display_ani_a2b(7, 4);
		displayRedDot(false);
		delay_ms(1000);
		display_ani_a2b(4, 6);
		displayRedDot(true);
		delay_ms(1000);
		display_ani_a2b_r20(6, 15);
	}

}
