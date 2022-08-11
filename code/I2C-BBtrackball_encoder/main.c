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
#include <usart.h>
#include <delay.h>

#define devID (uint8_t)0x32 // slave address for the stm8l

#define INT_PIN 4 // PC4 as Interrupt output pin (for host).

#define CMD_RGB_SET 0x03 // command to set RGB led

volatile uint16_t Event = 0x00;// I2C event value keeper 
volatile static uint8_t report_byte = 0x00;// Keep value read from PB4-7 and PD0
volatile static uint8_t RGBstat = 0x00;// RGB led (received from I2C host).

static uint8_t cmd[2];// 1 command byte and 1 setting bytes

#define DEBUG

#ifdef DEBUG
static void prntf(char *txt){
	while(*txt)
		usart_write(*txt++);
}
#endif

void GPIOinit(){ // GPIO init
	// For LED pins and Up Down Left Right input 
	PB_DDR = 0x0F;// PB0-PB3 as output for Blue Red Green and White (respectively to PB0-3).
	PB_CR1 = 0x0F;// all at push-pull mode normal speed
	PB_CR2 = 0xF0;// enable interrupt on all input pins
	// method above also sets PB4-7 as input for the Hall effect on trackball breakout board.
	
	// For interrupt output for host 
	PC_DDR |= (1 << INT_PIN);// PC4 as output
	//PC_CR1 |= (1 << INT_PIN);// Push-Pull
	PC_ODR |= (1 << INT_PIN);// INT is active low, Init with high, bring low when data report needed.
	
	//PD_DDR &= ~(1 << 0);// PD0 as input for push button of trackball
	//PD_CR1 &= ~(1 <<0);
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

    I2C1_TRISER |= 17;// Rise time  

    I2C1_CCRL = 0x50;
    I2C1_CCRH = 0x00;

    I2C1_CR1 |= (0x00 | 0x01);// i2c mode not SMBus
	
    I2C1_OARL = devID << 1;
    I2C1_OARH = 0x40; 

    I2C1_CR2 = 1 << 2;

    I2C1_CR1 |= 1 << 0;// cmd enable

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
  while(!(I2C1_SR1 & 0x40));
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
				cmd[0] = I2CRead();
				switch (cmd[0]){// receive first byte (command byte) from host 
				case CMD_RGB_SET:
					cmd[1] = I2CRead();
					PB_ODR = cmd[1] & 0x0F;// write output to control LED.
					break;
				default:
					break;
			}
      	break;

		case 0x0010:// I2C_EVENT_SLAVE_STOP_RECEIVED	
			I2C1_CR2 |= (1 << I2C1_CR2_ACK);// send ack 
			
		break;
		
		case 0x0682:// I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED
			I2C1_DR = report_byte;// report the trackball movement (up down left right) and button press.	
			
			report_byte = 0;// reset value to default
	
			PC_ODR |= (1 << INT_PIN);// Release interrupt 
		break;

    	default:
	break;
	}
	
	// Clear Interrupt flag
	if(I2C1_SR1 & 0x10)
		I2C1_CR2 = 0x00;

}
void Port4irq(void) __interrupt(12){
#ifdef DEBUG
	prntf("Down\n");
#endif
	report_byte = PB_IDR;// read input register.
	report_byte >>= 4;// shift bit to left 4 times, filter only PD4-7.
	EXTI_SR1 = 0x10;// clear interrupt pending of pin 4
}

void Port5irq(void) __interrupt(13){
#ifdef DEBUG
	prntf("Up\n");
#endif
	report_byte = PB_IDR;// read input register.
	report_byte >>= 4;// shift bit to left 4 times, filter only PD4-7.
	EXTI_SR1 = 0x20;// clear interrupt pending of pin 5
}

void Port6irq(void) __interrupt(14){
#ifdef DEBUG
	prntf("right\n");
#endif
	report_byte = PB_IDR;// read input register.
	report_byte >>= 4;// shift bit to left 4 times, filter only PD4-7.
	EXTI_SR1 = 0x40;// clear interrupt pending of pin 6
}

void Port7irq(void) __interrupt(15){
#ifdef DEBUG
	prntf("left\n");
#endif
	report_byte = PB_IDR;// read input register.
	report_byte >>= 4;// shift bit to left 4 times, filter only PD4-7.
	EXTI_SR1 = 0x80;// clear interrupt pending of pin 7
}

void PDirq(void) __interrupt(8){
	report_byte |= PD_IDR << 5;// read input register.
#ifdef DEBUG
	prntf("PD int!\n");
#endif
	EXTI_SR1 = 0x01;// clear interrupt pensing of pin 0
}

// cache the bad guys, interrupt that I don't event aware off (known by ComeNCapture project).
// for some reason interrupt want to enter the USART interrupt handler, but since thet are not exist
// It can cause CPU to reset. and stuck in reset loop.
void USART1tx_IRQ(void) __interrupt(27){}
void USART1rx_IRQ(void) __interrupt(28){}

// Trackball report : generate interrupt signal
// Data transmit done in I2C IRQ.
void TB_report(){
	PC_ODR &= ~(1 << INT_PIN);// Bring interrupt low.	
#ifdef DEBUG
	prntf("It's report time !\n");
#endif
}

void main(){
	CLK_CKDIVR = 0x00;
#ifdef DEBUG
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);
	usart_write(0x0C);
	prntf("starting BBTB encoder ver 1.0\n");
#endif

	GPIOinit(); // init all needed GPIOs
	I2CInit();// init i2c as slave having address 0x65
	PB_ODR |= 0x0F;
	delay_ms(1000);
	PB_ODR &= ~0x0F;
	__asm__("rim");// enble interrupt
	
	while(1){
		if(report_byte != 0){// report_byte != 0, there's data to send !
			TB_report();// Generates interrupt to grab host attention.
			delay_ms(1);
		}
	}

}// main 
