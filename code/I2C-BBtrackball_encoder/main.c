/* Black Berry trackball encoder based on STM8L151F3U6 as i2c device.
 * Coded by TinLethax 2021/8/14 +7
 */
 
/* Data report structure :
 * 		report_byte -> [bit 7-5 - NULL][bit 4 - Button][bit 3 - Down][bit 2 - Up][bit 1 - Right][bit 0 - Left]
 * Command :
 * 		CMD_RGB_SET -> [byte1 0x03]  [byte2 0x0X]=[bit 7-4 - NULL][bit 3 - White LED][bit 2 - Green LED][bit 1 - Red LED][bit 0 - Blue LED]
 */
 
#include <stm8l.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <usart.h>
#include <delay.h>

#define devID (uint8_t)0x32 // slave address for the stm8l

#define INT_PIN 4 // PC4 as Interrupt output pin (for host).

#define CMD_RGB_SET 0x03 // command to set RGB led

volatile uint16_t Event = 0x00;// I2C event value keeper 
volatile static uint8_t report_byte = 0x00;// Keep value read from PB4-7 and PD0
volatile static uint8_t RGBstat = 0x00;// RGB led (received from I2C host).

static uint8_t cmd[2];// 1 command byte and 1 setting bytes

uint16_t REMAP_Pin = 0x011C; 

static void prntf(char *txt){
	while(*txt)
		usart_write(*txt++);
}

void GPIOinit(){ // GPIO init
	// For LED pins and Up Down Left Right input 
	PB_DDR = 0x0F;// PB0-PB3 as output for Blue Red Green and White (respectively to PB0-3).
	PB_CR1 = 0x0F;// all at push-pull mode normal speed
	PB_CR2 = 0xF0;// enable interrupt on all input pins
	// method above also sets PB4-7 as input for the Hall effect on trackball breakout board.
	
	// For interrupt output for host 
	PC_DDR |= (1 << INT_PIN);// PC4 as output
	PC_CR1 |= (1 << INT_PIN);// Push-Pull
	PC_ODR |= (1 << INT_PIN);// INT is active low, Init with high, bring low when data report needed.
	
	PD_DDR &= ~(1 << 0);// PD0 as input for push button of trackball
	PD_CR1 &= ~(1 <<0);
	PD_CR2 |= (1 << 0);// PD0 has interrupt too.
	
	EXTI_CR1 |= 0x02;// set interrupt for Rise then Fall detection for pin 0 (Used by PD0).
	EXTI_CR2 |= 0xFF;// set interrupt for Rise then Fall detection from Pin 4 to Pin 7 (Pin interrupt, Not Port interrupt).
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
    I2C1_OARH = (uint8_t)((uint8_t)(0x00 | 0x40 ) | (uint8_t)((uint16_t)( (uint16_t)devID &  (uint16_t)0x0300) >> 7)); 

    I2C1_CR2 = (uint8_t)(1 << 2);

    I2C1_CR1 |= (1 << 0);// cmd enable

    I2C1_ITR |= (1 << 0) | (1 << 1) | (1 << 2);// enable interrupt (buffer, event an error interrupt)
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
	//printf("someone calling me!\n");
      	break;

      /* Check on EV2*/
    	case 0x0240 :// I2C_EVENT_SLAVE_BYTE_RECEIVED, Receive data from Host
      	
			switch (I2CRead()){// receive first byte (command byte) from host 
			
				case CMD_RGB_SET:
					cmd[1] = I2CRead();
					PB_ODR = cmd[1] & 0x0F;// write output to control LED.
					break;
				default:
					break;
			}
			
      	break;

    	default:
	break;
	}

}
void Port4irq(void) __interrupt(12){
	prntf("Down\n");
	report_byte = PB_IDR;// read input register.
	report_byte >>= 4;// shift bit to left 4 times, filter only PD4-7.
	EXTI_SR1 = 0x10;// clear interrupt pending of pin 4
}

void Port5irq(void) __interrupt(13){
	prntf("Up\n");
	report_byte = PB_IDR;// read input register.
	report_byte >>= 4;// shift bit to left 4 times, filter only PD4-7.
	EXTI_SR1 = 0x20;// clear interrupt pending of pin 5
}

void Port6irq(void) __interrupt(14){
	prntf("right\n");
	report_byte = PB_IDR;// read input register.
	report_byte >>= 4;// shift bit to left 4 times, filter only PD4-7.
	EXTI_SR1 = 0x40;// clear interrupt pending of pin 6
}

void Port7irq(void) __interrupt(15){
	prntf("left\n");
	report_byte = PB_IDR;// read input register.
	report_byte >>= 4;// shift bit to left 4 times, filter only PD4-7.
	EXTI_SR1 = 0x80;// clear interrupt pending of pin 7
}

void PDirq(void) __interrupt(8){
	report_byte |= PD_IDR << 5;// read input register.
	prntf("PD int!\n");
	EXTI_SR1 = 0x01;// clear interrupt pensing of pin 0
}

// cache the bad guys, interrupt that I don't event aware off (known by ComeNCapture project).
// for some reason interrupt want to enter the USART interrupt handler, but since thet are not exist
// It can cause CPU to reset. and stuck in reset loop.
void USART1tx_IRQ(void) __interrupt(27){}
void USART1rx_IRQ(void) __interrupt(28){}

// Trackball report : send data over I2C to host with Host interrupting 
void TB_report(){
	__asm__("sim");// pause system wide interrupt for clean TX.
	PC_ODR &= ~(1 << INT_PIN);// Bring interrupt low.
	
	prntf("report time !\n");
	//I2CWrite(report_byte);// report the trackball movement (up down left right) and button press.
	
	// reset value to default
	report_byte = 0;
	
	PC_ODR |= (1 << INT_PIN);// Release interrupt 
	__asm__("rim");// resume system interrupt.
}

void main(){
	CLK_CKDIVR = 0x00;
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); //remap the non-exit pin of Tx and Rx of the UFQFPN20 package to the exit one.
	SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);
	prntf(" starting BBTB encoder ver 1.0\n");
	
	GPIOinit(); // init all needed GPIOs
	I2CInit();// init i2c as slave having address 0x65
	delay_ms(1000);
	__asm__("rim");// enble interrupt
	
	while(1){
		if(report_byte){// report_byte != 0, there's data to send !
		TB_report();
		delay_ms(1);
		}
	}

}// main 
