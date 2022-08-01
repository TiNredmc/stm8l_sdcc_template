/* Black Berry passport keyboard scanner based on STM8L151F3U6 as i2c device.
 * Coded by TinLethax 2022/3/24 +7
 */
 
/* Data report structure : report ASCII code upto 6 bytes (keys) per 1 packet
 * 		report_byte -> MSB[Report size][Key 6][Key 5][Key 4][Key 3][Key 2][Key 1]LSB (Note : It's dynamics. I allow the MCU to report from 1 to 6 bytes).
 */
 
#include <stm8l.h>
#include <stdio.h>
#include <usart.h>
#include <delay.h>

#define devID (uint8_t)0x32 // slave address for the stm8l

#define INT_PIN 0 // PD3 as Interrupt output pin (for host).

volatile uint16_t Event = 0x00;// I2C event value keeper 

static uint8_t kb_report[6] = {0};
static uint8_t report_order = 0;

// const array for GPIOs scanning.
const static uint8_t kb_scanner[5] = 
{0x1E, // (PB4)[1][1][1][1][0](PB0)
0x1D, //  (PB4)[1][1][1][0][1](PB0)
0x1B, //  (PB4)[1][1][0][1][1](PB0)
0x17, //  (PB4)[1][0][1][1][1](PB0)
0x0F}; // (PB4)[0][1][1][1][1](PB0)

static uint8_t scanner_cnt = 0;// keep track of current column.

// Matrix keycodes.
const static uint8_t row1_left[5] = {'q', 'w', 'e', 'r', 't'};
const static uint8_t row1_right[5] = {'y', 'u', 'i', 'o', 'p'};
const static uint8_t row2_left[5] = {'a', 's', 'd', 'f', 'g'};
const static uint8_t row2_right[5] = {'h', 'j', 'k', 'l', 8};// last one is back space.
const static uint8_t row3_left[5] = {'z', 'x', 'c', 'v', ' '};
const static uint8_t row3_right[5] = {0, 'B', 'N', 'M', 10};// 10 -> Line feed -> new line -> "enter" likes -> enter key.

// Interrupt related stuffs
volatile static uint8_t kbd_flag = 0;

#define REMAP_Pin 0x011C 
#define DEBUG

static void prntf(char *txt){
	while(*txt)
		usart_write(*txt++);
}

void GPIOinit(){ // GPIO init
	// PB0-4 as output open drain for matrix scanning. when scanning will pull the input pin down when circuit complete by button press.
	PB_DDR = 0x1F;// PB0-4 as output open drain.
	PB_ODR |= 0x1F;// set all output scanner pins as floating (input pull up reads at logic 1).
	
	// PB5-7 as input with pull up.
	PB_CR1 |= 0xE0;// PB5-7 as input with pull up.

	// PB4-6 as input with pull up.
	PC_CR1 |= 0x70;// PC4-6 as input with pull up.
	
	// PD0 as a interrupt output. Active low with push pull.
	PD_DDR |= 0x01;// PD0 as output.
	PD_CR1 |= 0x01;// push pull mode.
	PD_ODR |= 0x01;// default state is logic high.
}

// For I2C
//////////////////////////////////////////////////
uint16_t SCLSpeed = 0x0050;
void I2CInit() {
	CLK_PCKENR1 |= (uint8_t)(1 << 0x03);// enable the I2C clock 
    I2C1_FREQR |= 16;// 16MHz/10^6
	
    I2C1_CR1 &= ~0x01;// cmd disable for i2c configurating

    I2C1_TRISER |= (uint8_t)(17);// Riser time  

    I2C1_CCRL = (uint8_t)SCLSpeed;
    I2C1_CCRH = (uint8_t)((SCLSpeed >> 8) & 0x0F);

    I2C1_CR1 |= (0x00 | 0x01);// i2c mode not SMBus
	
    I2C1_OARL = (uint8_t)(devID);
    I2C1_OARH = (uint8_t)((uint8_t)0x40  | (uint8_t)((uint16_t)( (uint16_t)devID &  (uint16_t)0x0300) >> 7)); 

    I2C1_CR2 = (uint8_t)(1 << 2);

    I2C1_CR1 |= (1 << 0);// cmd enable

    I2C1_ITR |= (1 << 0) | (1 << 1) | (1 << 2);// enable interrupt (buffer, event and error interrupt)
}

static uint16_t I2CReadStat(){
	/* Dummy reaing the SR2 */
	(void)I2C1_SR2;
	I2C1_SR2 = 0;
	
	/*event reading*/
	uint8_t flag1 = 0x00;
	uint8_t flag2 = 0x00;
	flag1 = I2C1_SR1;
	flag2 = I2C1_SR3;
	return  ((uint16_t)((uint16_t)flag2 << (uint16_t)8) | (uint16_t)flag1);
}

uint8_t I2CRead(){
  /* Return the data present in the DR register */
  return ((uint8_t)I2C1_DR);
}

void I2CWrite(uint8_t dat){
	I2C1_DR = dat;
	while(!(I2CReadStat() == 0x0780));// wait until tx is ready for next send.
}

/* Interrupt handlers */

void i2cirq(void) __interrupt(29){
	
	switch(I2CReadStat()){

    	case 0x0202 : //I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED 
			//I2CWrite(69);// ping back to host with "Special number".
      	break;

      /* Check on EV2*/
    	case 0x0240 :// I2C_EVENT_SLAVE_BYTE_RECEIVED, Receive data from Host
			switch(I2CRead()){// read CMD
				
			case 0xAB: // Sleep command.
				kbd_flag |= 0x02; // Sleep flag set bit 1 to 1 .
				break;
			
			case 0x69:// Ping, reply back with "magic number".
				I2CWrite(69);
				break;
			
			default:
				break;
			
			}
			
      	break;

    	default:
	break;
	}

}

// cache the bad guys, interrupt that I don't event aware off (known by ComeNCapture project).
// for some reason interrupt want to enter the USART interrupt handler, but since thet are not exist
// It can cause CPU to reset. and stuck in reset loop.
void USART1tx_IRQ(void) __interrupt(27){}
void USART1rx_IRQ(void) __interrupt(28){}

void main(){
	CLK_CKDIVR = 0x00;// Mo CPU clock divider. Now 16MHz.
#ifdef DEBUG
	usart_init(9600); // usart using baudrate at 9600
	usart_Half_init();// Only use TX.
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	prntf(" starting BBKB ver 1.0\n");
#endif
	
	GPIOinit(); // init all needed GPIOs
	I2CInit();// init i2c as slave having address 0x65

	__asm__("rim");// enble interrupt
	
	while(1){
		
		// check for Sleep flag
		if(kbd_flag & 0x02){
			kbd_flag &= ~0x02;// reset sleep flag
			__asm__("wfi");// enter sleep
		}
		// scanner select column 
		PB_ODR |= kb_scanner[scanner_cnt];// PB0-4
		
		// listen on all 6 pins.
		if(!(PB_IDR & (1 << 6)))// PB6
			kb_report[report_order++] = row1_left[scanner_cnt];
		
		if(!(PB_IDR & (1 << 5)))// PB5
			kb_report[report_order++] = row1_right[scanner_cnt];
		
		if(!(PC_IDR & (1 << 6)))// PC6
			kb_report[report_order++] = row2_left[scanner_cnt];
		
		if(!(PC_IDR & (1 << 5)))// PC5
			kb_report[report_order++] = row2_right[scanner_cnt];
		
		if(!(PB_IDR & (1 << 7)))// PB7
			kb_report[report_order++] = row3_left[scanner_cnt];
		
		if(!(PC_IDR & (1 << 4)))// PC4
			kb_report[report_order++] = row3_right[scanner_cnt];
		
		// If there is/are keypress(es), send the report size first.
		
		if(report_order){
			PD_ODR &= ~(1 << INT_PIN);// Host interruption. 
			I2CWrite(report_order);
		}
		
		// Then send keycode(s).
		
		while(report_order){
			I2CWrite(kb_report[report_order]);
			report_order--;
		}
		
		PD_ODR |= (1 << INT_PIN);// Release Interrupt.
		
		// increase counter, plus return to 0 check.
		
		scanner_cnt++;
		if(scanner_cnt > 5)
			scanner_cnt = 0;
		
		PB_ODR = 0;// reset all GPIOs state.

		delay_ms(1);// delay.
	}

}// main 
