/* Black Berry passport keyboard scanner based on STM8L151F3U6 as i2c device.
 * Coded by TinLethax 2022/3/24 +7
 */
 
/* Data report structure : report ASCII code 6 bytes (keys) per 1 packet
 * 		report_byte -> MSB[Key 1][Key 2][Key 3][Key 4][Key 5][Key 6]LSB.
 */
 
#include <stm8l.h>
#include <stdio.h>
#include <usart.h>
#include <delay.h>

#define devID (uint8_t)0x69 // slave address for the stm8l

#define INT_PIN 0 // PD0 as Interrupt output pin (for host).

volatile uint16_t Event = 0x00;// I2C event value keeper 

volatile uint8_t kb_report[6] = {0};
uint8_t report_order = 0;// Counter for saving key press
volatile uint8_t dumpbin = 0;// dumpbin
volatile uint8_t b_cnt = 0;

// Matrix keycodes. NOT in ASCII!
const uint8_t row1_left[5] = {0x14, 0x1A, 0x08, 0x15, 0x17};//q w e r t
const uint8_t row1_right[5] = {0x1C, 0x18, 0x0C, 0x12, 0x13};//y u i o p
const uint8_t row2_left[5] = {0x04, 0x16, 0x07, 0x09, 0x0A};// a s d f g
const uint8_t row2_right[5] = {0x0B, 0x0D, 0x0E, 0x0F, 0x2A};// h j k l backspace
const uint8_t row3_left[5] = {0x1D, 0x1B, 0x06, 0x19, 0x2C};// z x c v spacebar
const uint8_t row3_right[5] = {0, 0x05, 0x11, 0x10, 0x28};// NOKEY B N M enter

// Interrupt related stuffs
volatile uint8_t kbd_flag = 0;
 
#define DEBUG

#ifdef DEBUG
static void prntf(char *txt){
	while(*txt)
		usart_write(*txt++);
}
#endif

void GPIOinit(){ // GPIO init
	// PB0-4 as input pull up
	PB_CR1 |= 0x1F;// PB0-4 as input pull up
	
	// PB5-7 as output with open drain mode
	//PB_DDR |= 0xE0;
	// PB4-6 as output with open drain mode
	//PC_DDR |= 0x70;
	
	// PD0 as a interrupt output. Active low with push pull.
	PD_DDR |= 0x01;// PD0 as output.
	//PD_CR1 |= 0x01;// push pull mode.
	PD_ODR |= 0x01;// default state is logic high.
}

// For I2C
//////////////////////////////////////////////////
// uint16_t SCLSpeed = 0x0050; 100kHz I2C
void I2CInit() {
	CLK_PCKENR1 |= (uint8_t)(1 << 0x03);// enable the I2C clock 
    I2C1_FREQR |= 16;// 16MHz/10^6
	
    I2C1_CR1 &= ~0x01;// cmd disable for i2c configurating

    I2C1_TRISER |= (uint8_t)(17);// Riser time  

    I2C1_CCRL = 0x50;
    I2C1_CCRH = 0x00;

    I2C1_CR1 |= (0x00 | 0x01);// i2c mode not SMBus
	
    I2C1_OARL = (devID << 1);
    I2C1_OARH = (1 << 6);

    I2C1_CR1 |= (1 << 0);// cmd enable
	I2C1_CR2 |= (1 << 2);

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
  while(!(I2C1_SR1 & 0x40));
  return ((uint8_t)I2C1_DR);
}

void I2CWrite(uint8_t dat){
	while(!(I2CReadStat() == 0x0484));
	I2C1_DR = dat;
}

/* Interrupt handlers */

void i2cirq(void) __interrupt(29){
	
	switch(I2CReadStat()){

    	case 0x0202 : //I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED 
#ifdef DEBUG
			prntf("rec addr match\n");
#endif
			switch(I2CRead()){// read CMD
				
			case 0xAB: // Sleep command.
				kbd_flag |= 0x02; // Sleep flag set bit 1 to 1 .
#ifdef DEBUG
				prntf("Got a sleep flag!\n");
#endif
				break;
			
			case 0x69:// Ping, reply back with "magic number".
#ifdef DEBUG			
				prntf("Got a ping! Send \"Magic Number\"\n");
#endif
				I2C1_DR = 0x69;
				break;
			
			default:
				break;
			}
			
      	break;

		case 0x0010:// I2C_EVENT_SLAVE_STOP_RECEIVED	
			I2C1_CR2 |= (1 << I2C1_CR2_ACK);// send ack 
#ifdef DEBUG
			prntf("Send ack!\n");
#endif
		
		break;
		
		case 0x0682:// I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED
			PD_ODR = 0x01;// Release Interrupt. Clear keyboard interrupt so host don't confuse.
			dumpbin = I2C1_SR1;
			dumpbin = I2C1_SR3;
			I2C1_DR = kb_report[b_cnt];
			while(!(I2C1_SR1 & 04));
			kb_report[b_cnt++] = 0;// clear key press
			I2C1_DR = kb_report[b_cnt];
			while(!(I2C1_SR1 & 04));
			kb_report[b_cnt++] = 0;// clear key press
			I2C1_DR = kb_report[b_cnt];
			while(!(I2C1_SR1 & 04));
			kb_report[b_cnt++] = 0;// clear key press
			I2C1_DR = kb_report[b_cnt];
			while(!(I2C1_SR1 & 04));
			kb_report[b_cnt++] = 0;// clear key press
			I2C1_DR = kb_report[b_cnt];
			while(!(I2C1_SR1 & 04));
			kb_report[b_cnt++] = 0;// clear key press
			I2C1_DR = kb_report[b_cnt];
			kb_report[b_cnt++] = 0;// clear key press
			b_cnt = 0;
#ifdef DEBUG
			prntf("TX done\n");
#endif
		break;
		

    	default:
		break;
	}
	
	// Clear Interrupt flag
	uint8_t i2cint = I2C1_SR2 & 0x0F;
	if(i2cint){
		I2C1_SR2 &= ~(0x0F);
	}
	
	if(I2C1_SR1 & 0x08)
		I2C1_CR2 = 0x00;

}

// cache the bad guys, interrupt that I don't event aware off (known by ComeNCapture project).
// for some reason interrupt want to enter the USART interrupt handler, but since they are not exist
// It can cause CPU to reset. and stuck in reset loop.
void USART1tx_IRQ(void) __interrupt(27){}
void USART1rx_IRQ(void) __interrupt(28){}

uint8_t report_fsm = 0;
uint8_t scanner_fsm = 0;
void main(){
	CLK_CKDIVR = 0x00;// Mo CPU clock divider. Now 16MHz.
#ifdef DEBUG
	usart_init(9600); // usart using baudrate at 9600
	usart_Half_init();// Only use TX.
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);
	usart_write(0x0C);
	prntf("starting BBKB ver 2.0\n");
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
		
		// Key matrix scanner

		__asm__("bset 0x5007, #6");// PB 6 as output
		__asm__("bres 0x5005, #6");// Pull PB6 down
						
		delay_ms(1);				
						
		if(!(PB_IDR & (1 << 0)))
			kb_report[report_order++] = row1_left[0];
	
		if(!(PB_IDR & (1 << 1)))
			kb_report[report_order++] = row1_left[1];
		
		if(!(PB_IDR & (1 << 2)))
			kb_report[report_order++] = row1_left[2];
		
		if(!(PB_IDR & (1 << 3)))
			kb_report[report_order++] = row1_left[3];
		
		if(!(PB_IDR & (1 << 4)))
			kb_report[report_order++] = row1_left[4];
		
		__asm__("bset 0x5005, #6");// Release PB6

		__asm__("bres 0x5007, #6");// PB6 as input Hi-z
		__asm__("bset 0x5007, #5");// PB5 as output 
		__asm__("bres 0x5005, #5");// Pull PB5 down
		
		delay_ms(1);	
		
		if(!(PB_IDR & (1 << 0)))
			kb_report[report_order++] = row1_right[0];
	
		if(!(PB_IDR & (1 << 1)))
			kb_report[report_order++] = row1_right[1];
		
		if(!(PB_IDR & (1 << 2)))
			kb_report[report_order++] = row1_right[2];
		
		if(!(PB_IDR & (1 << 3)))
			kb_report[report_order++] = row1_right[3];
		
		if(!(PB_IDR & (1 << 4)))
			kb_report[report_order++] = row1_right[4];
		
		__asm__("bset 0x5005, #5");// Release PB5
		
		__asm__("bres 0x5007, #5");// PB5 as input Hi-z
		__asm__("bset 0x500C, #6");// PC6 as output 
		__asm__("bres 0x500A, #6");// Pull PC6 down
		
		delay_ms(1);
		
		if(!(PB_IDR & (1 << 0)))
			kb_report[report_order++] = row2_left[0];
	
		if(!(PB_IDR & (1 << 1)))
			kb_report[report_order++] = row2_left[1];
		
		if(!(PB_IDR & (1 << 2)))
			kb_report[report_order++] = row2_left[2];
		
		if(!(PB_IDR & (1 << 3)))
			kb_report[report_order++] = row2_left[3];
		
		if(!(PB_IDR & (1 << 4)))
			kb_report[report_order++] = row2_left[4];
		
		__asm__("bset 0x500A, #6");// Release PC6
		
		__asm__("bres 0x500C, #6");// PC6 as input Hi-z
		__asm__("bset 0x500C, #5");// PC5 as output 
		__asm__("bres 0x500A, #5");// Pull PC5 down
		
		delay_ms(1);

		if(!(PB_IDR & (1 << 0)))
			kb_report[report_order++] = row2_right[0];
	
		if(!(PB_IDR & (1 << 1)))
			kb_report[report_order++] = row2_right[1];
		
		if(!(PB_IDR & (1 << 2)))
			kb_report[report_order++] = row2_right[2];
		
		if(!(PB_IDR & (1 << 3)))
			kb_report[report_order++] = row2_right[3];
		
		if(!(PB_IDR & (1 << 4)))
			kb_report[report_order++] = row2_right[4];
		
		__asm__("bset 0x500A, #5");// Release PC5

		__asm__("bres 0x500C, #5");// PC5 as input Hi-z
		__asm__("bset 0x5007, #7");// PB7 as output 
		__asm__("bres 0x5005, #7");// Pull PB7 down
		
		delay_ms(1);
		
		if(!(PB_IDR & (1 << 0)))
			kb_report[report_order++] = row3_left[0];
	
		if(!(PB_IDR & (1 << 1)))
			kb_report[report_order++] = row3_left[1];
		
		if(!(PB_IDR & (1 << 2)))
			kb_report[report_order++] = row3_left[2];
		
		if(!(PB_IDR & (1 << 3)))
			kb_report[report_order++] = row3_left[3];
		
		if(!(PB_IDR & (1 << 4)))
			kb_report[report_order++] = row3_left[4];
		
		__asm__("bset 0x5005, #7");// Release PB7

		__asm__("bres 0x5007, #7");// PB7 as input Hi-z
		__asm__("bset 0x500C, #4");// PC4 as output 
		__asm__("bres 0x500A, #7");// Pull PC4 down
		
		delay_ms(1);
						
		if(!(PB_IDR & (1 << 0)))
			kb_report[report_order++] = row3_right[0];
	
		if(!(PB_IDR & (1 << 1)))
			kb_report[report_order++] = row3_right[1];
		
		if(!(PB_IDR & (1 << 2)))
			kb_report[report_order++] = row3_right[2];
		
		if(!(PB_IDR & (1 << 3)))
			kb_report[report_order++] = row3_right[3];
		
		if(!(PB_IDR & (1 << 4)))
			kb_report[report_order++] = row3_right[4];
		
		__asm__("bset 0x500A, #4");// Release PC4
		__asm__("bres 0x500C, #4");// PC4 as input Hi-z

		if((report_order != 0) && (report_fsm == 2))// No need for key null when button is still press on next scan.
			report_fsm = 1;
		
		// Generate hardware interrupt
		switch(report_fsm){
			case 0:// report key Null once at start up.
#ifdef DEBUG
				prntf("Init Key NULL\n");
#endif
				kb_report[0] = 0;
				kb_report[1] = 0;
				kb_report[2] = 0;
				kb_report[3] = 0;
				kb_report[4] = 0;
				kb_report[5] = 0;
				PD_ODR = 0x00;// Generate interrupt pull down.
				report_order = 0;
				
				report_fsm = 1;// Next stage Idle. Wait for report_order != 0.
			break;
			
			case 1:// Report Key when report_order != 0.
				if(report_order != 0){
#ifdef DEBUG
				prntf("Key Report\n");
#endif				
				PD_ODR = 0x00;// Generate interrupt.
				report_order = 0;
				report_fsm = 2;
				}
				
			break;
			
			case 2:// Report Null key after key release.
#ifdef DEBUG
				prntf("Key NULL\n");
#endif
				kb_report[0] = 0;
				kb_report[1] = 0;
				kb_report[2] = 0;
				kb_report[3] = 0;
				kb_report[4] = 0;
				kb_report[5] = 0;
				PD_ODR = 0x00;// Generate interrupt pull down.
				report_order = 0;
				
				report_fsm = 1;// Next stage Idle. Wait for report_order != 0.
			break;
		}
		
		delay_ms(5);// delay.
	}

}// main 
