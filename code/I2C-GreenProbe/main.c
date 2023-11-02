/* Project GreenProbe.
 * STM8L as Communication controller for the SLG4DVKGSD "compatible clone".
 * Coded by TinLethax 2023/06/12 +7
 */

#include <stm8l.h>
#include <stdio.h>
#include <usart.h>
#include <delay.h>

// Pin connection
// PB0 -> GP0 of MCP2221
// PB2 -> LV pin of I2C logic level translator

#define devID (uint8_t)0x08 // slave address for the stm8l

volatile uint16_t Event = 0x00;// I2C event value keeper 
volatile uint8_t return_i2c = 0x00;
volatile uint8_t i2c_reply = 0x00;
volatile uint8_t replyF4 = 0;
const uint8_t replyD8[6] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};// Still figuring out the meaning of these.
volatile uint8_t replycnt = 0;

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

    I2C1_CR1 |= (0x00 | 0x01 | 1 << 7);// i2c mode not SMBus, No clock stretching.
	
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
	// Clear Interrupt flag
	uint8_t i2cint = I2C1_SR2 & 0x0F;
	if(i2cint){
		I2C1_SR2 &= ~(0x0F);
	}
	
	
	switch(I2CReadStat()){

    	case 0x0202 : //I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED 

			switch(I2CRead()){
			case 0xE5: 
				i2c_reply = 0xE5;
				
				break;
			
			case 0xE6:
				i2c_reply = 0xE6;
				
				break;
				
			case 0xE8:
				i2c_reply = 0xE8;
				
				break;
				
			case 0xD8:
				i2c_reply = 0xD8;
				
				break;
				
			case 0xF4:
				i2c_reply = 0xF4;
				
				break;
				
			default:
				break;
			}
      	break;
		
		case 0x0240: //I2C_EVENT_SLAVE_BYTE_RECEIVED
		
			// In case of Updating LED status.
			if(i2c_reply == 0xF4){
				replyF4 = I2CRead();// LED status parameter.
				i2c_reply = 0;
				// Recorded data that I have seen so far.
				// 0x01
				// 0x81
				// 0x91
			}
			
		break;

		case 0x0010:// I2C_EVENT_SLAVE_STOP_RECEIVED	
			I2C1_CR2 |= (1 << I2C1_CR2_ACK);// send ack 
		
		break;
		
		case 0x0682: //  I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED
		
		break;
		
		
		case 0x0680: //I2C_EVENT_SLAVE_BYTE_TRANSMITTING
			(void)I2C1_SR1;
			(void)I2C1_SR3;
			switch(i2c_reply){
			case 0xE5:// Read NVM status
				I2C1_DR = 0x45;// NVM is locked 
				i2c_reply = 0;
				
				break;
			
			case 0xE6:// Pattern ID (need more investigation).
				I2C1_DR = 0x69;
				i2c_reply = 0;
				
				break;
			
			case 0xE8:// Read from reserved... (need more investigation).
				I2C1_DR = 0x52;
				i2c_reply = 0;
				
				break;
			
			case 0xD8:// Read from SLG46582V OTP memory (6 bytes respond)
				I2C1_DR = replyD8[replycnt];
				replycnt += 1;
				if(replycnt == 6){
					replycnt = 0;
					i2c_reply = 0;
				}
				
				break;
			
			case 0xF4:// set virtual input to toggle something inside SLG46582V.
				I2C1_DR = replyF4;// Register read-back capability.
				i2c_reply = 0;
				
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
// for some reason interrupt want to enter the USART interrupt handler, but since they are not exist
// It can cause CPU to reset. and stuck in reset loop.
void USART1tx_IRQ(void) __interrupt(27){}
void USART1rx_IRQ(void) __interrupt(28){}

uint8_t current_pin_stat = 0;
uint8_t prev_pin_stat = 0;

void main(){
	CLK_CKDIVR = 0x00;// Mo CPU clock divider. Now 16MHz.
	PB_DDR |= (1 << 2);
	PB_CR1 |= (1 << 2);
	I2CInit();// init i2c as slave having address 0x65
	prev_pin_stat = 1;
	__asm__("rim");// enble interrupt
	
	while(1){
		// MCP2221 select On board GreenPAK and Target GreenPAK with GP0 pin
		// HIGH -> Select Target GreenPAK
		// LOW -> Select on-board GreenPAK (the SLG46582V)
		current_pin_stat = PB_IDR & (1 << 0);// detect pin change on GP0
		if(prev_pin_stat != current_pin_stat){
			
			if(current_pin_stat == 1){
				//De-init I2C
				__asm__("sim");
				// Enable I2C gate to Target GreenPAK
				__asm__("bset 0x5005, #2");
				//PB_ODR |= (1 << 2);
				__asm__("bres 0x5210, #0");
				//I2C1_CR1 &= ~(1 << 0);
				__asm__("rim");
				
			}else{
				//Re-init I2C
				__asm__("sim");
				// Disable I2C gate to Target GreenPAK
				__asm__("bres 0x5005, #2");
				//PB_ODR &= ~(1 << 2);
				I2CInit();
				__asm__("rim");
				
			}
			
			prev_pin_stat = current_pin_stat;
		}
		
		
	}

}// main 
