/* iCEBlaster, Flash iCE40 bitstream over serial UART to SPI NOR flash.
 * Coded by TinLethax 2021/12/21 +7
 */
#include <stdint.h>
#include <stdbool.h>
#include <stm8l.h>
#include <delay.h>
#include <usart.h>
#include <spi.h>

#define SCS	4// PC4 will be used as CS pins

// Buffer for SPI DMA
volatile static uint8_t DataBuf[2][256];

// SPI Nor flash offset address send by PC to do the loop flash.
volatile uint8_t DataOff[3] = {0};

// Loop counter
volatile uint8_t bufcounter = 0;

// SPI_flag, 1 == data ready to write.
volatile uint8_t SPI_flag = 0;

// Switch between buffer 1 and 2
volatile bool Switcher = 0;

void USARTirq(void) __interrupt(28){// Interrupt of USART RX, triggered when PC send data in.
	// Address decoder
	while (!(USART1_SR & (1 << USART1_SR_RXNE)));
	DataOff[2] = USART1_DR;// receive the low byte first, (address[7:0]).
	while (!(USART1_SR & (1 << USART1_SR_RXNE)));
	DataOff[1] = USART1_DR;// then receive the mid byte. (address[15:8]).
	while (!(USART1_SR & (1 << USART1_SR_RXNE)));
	DataOff[0] = USART1_DR;// last one is high byte. (address[23:16]).

	//Switcher != Switcher;// while SPI reads from Buffer 1, USART put data on buffer 2 and vice versa.
	
	// receive data
	do{
		DataBuf[Switcher][bufcounter] = USART1_DR;
		while (!(USART1_SR & (1 << USART1_SR_TC)));
		bufcounter++;// auto overflow and roll over.
	}while(bufcounter);
	
	SPI_flag = 1;
}

// Initialize PB4 as Chip select pin 
void setpin(){
	PB_DDR |= (1 << SCS);
	PB_CR1 |= (1 << SCS);
	
	PB_ODR |= (1 << SCS);// _/
}


void main() {
	usart_init(115200);// usart using baudrate at 115200.
	USART1_CR2 |= 1 << (0x05);// enable USART interrupt.
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	
	setpin();// init CS pin
	SPI_Init(FSYS_DIV_2);// Get maximum SPI speed at 8MHz.
	delay_ms(10);
	uint8_t CMD = 0x06;// write enable command of SPI NOR.
	SPI_Write(&CMD, 1);// send write enable command to SPI NOR.
	
    while (1) {
		
		if(SPI_flag){// SPI flag set by USART interrupt after received all data.
			uint8_t CMD = 0;
			
			if((DataOff[2] == 0) && (DataOff[1] == 0) && (DataOff [0] == 0)){// If we detect the offset 0 (very beginning of the flash).
				// We erase entire chip.
				CMD = 0xC7;// Chip Erase command.
				SPI_Write(&CMD, 1);
			}
			
			CMD = 0x02;// Page Write command.
			
			SPI_WriteThenWrite(&CMD, 1, DataOff, 3);// send write command follows by 24bit address of page address.
			
			do{
				SPI1_DR = DataBuf[Switcher][bufcounter];
				while (!(SPI1_SR & (1 << SPI1_SR_TXE)));
				while (!(SPI1_SR & (1 << SPI1_SR_RXNE)));// wait until we receive (dummy) data.
				(void) SPI1_DR;
				bufcounter++;
			}while(bufcounter);// relies on counter overflow.
			
			while (SPI1_SR & 0x80);// wait until we are not busy.
			
			// send echo to start another 256 bytes transfer.
			USART1_DR = 0x10;
			while (!(USART1_SR & (1 << USART1_SR_TC)));
	
			SPI_flag = 0;
			Switcher != Switcher;// while SPI reads from Buffer 1, USART put data on buffer 2 and vice versa.
			}
		
    }
}
