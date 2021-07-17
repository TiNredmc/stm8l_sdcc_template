/* ComeNCapture is a quick-n-dirty logic analyzer 
 * Coded By TinLethax 2020/05/23
 * Right now the RX part (pc -> MCU) uses the Interrupt method. In the future I will implement the DMA 
 * So I can keep the timer freely running
 */
/* 2021/07/15 Update
 * GPIO will only available for 5 input cahnnels only, due to SPI taking up PB5-7.
 * I'm now implementing the SPI F-RAM driver into this code
 * in order to gain more Sample size to 16Ksa (1 byte per 1 sample)(16Kbytes F-RAM FM25V01A)
 * Also making this compatible with OLS 
*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <stm8l.h>
#include <delay.h>
#include <usart.h>
#include <spi.h>

// Define FM25V01A's CS pin
#define SCS	4 // PC4 as a CS pin

// Define SUMP command for capturing 
// From https://github.com/gillham/logic_analyzer

/* XON/XOFF are not supported. */
#define SUMP_RESET 0x00
#define SUMP_ARM   0x01
#define SUMP_QUERY 0x02
#define SUMP_XON   0x11
#define SUMP_XOFF  0x13

/* mask & values used, config ignored. only stage0 supported */
#define SUMP_TRIGGER_MASK 0xC0
#define SUMP_TRIGGER_VALUES 0xC1
#define SUMP_TRIGGER_CONFIG 0xC2

/* Most flags (except RLE) are ignored. */
#define SUMP_SET_DIVIDER 0x80
#define SUMP_SET_READ_DELAY_COUNT 0x81
#define SUMP_SET_FLAGS 0x82
#define SUMP_SET_RLE 0x0100

/* extended commands -- self-test unsupported, but metadata is returned. */
#define SUMP_SELF_TEST 0x03
#define SUMP_GET_METADATA 0x04

// For incoming capture command/params
uint8_t CapParam[5];// capture parameter received from host PC running SUMP OLS

// For capture timing
uint32_t divider = 0;// divider using for capturing frequency 
uint16_t useMicro = 0;
uint16_t delayTime  = 0;

/* USART */
// pin remap
const uint16_t REMAP_Pin = 0x011C; //constant value for sysconf to remap USART pin
// boot text
const static char bootText[] = {" ComeNCapture 2.0\n"};

/* for FM25V01A */

// FM25V01A necessary opcode
// Opcode (commands)
const static uint8_t FM25_FSTRD 	= 0x0B;// Fast Read memory data

const static uint8_t FM25_READ		= 0x03;// Read memory data

const static uint8_t FM25_WRITE		= 0x02;// Write memory data 

// capture loop locking status
static bool capture = true;

// F-RAM address counter keeper for reading and slow write 
uint16_t ramADDR = 0x3FFF;// maximum F-RAM address for roll-counting 

/////////////////////////////////////////////////////////////////////////
///////// Normal I/O stuffs /////////////////////////////////////////////
/* Initialize all PortB from 0-4 as input
 * Initialize PC4 as SCS pin also Init SPI stuffs too
 */
void IOinit(){
	PB_DDR &= 0; // set all pin at input
	PB_CR1 &= 0; // floating input
	PB_CR2 &= 0; // No interrupt

	//then re initialize SPI-needed pins
	SPI_Init(FSYS_DIV_2);// Init SPI peripheral with 8MHz clock (Max speed tho).
}

// usart print function
// I don't use the printf becuase that code takes a lot of space.
void prntf(char *p){
	while(*p){
		usart_write(*p++);
	}
}

/////////////////////////////////////////////////////////////////////////
///////// Timer stuffs //////////////////////////////////////////////////
/* Initialize the TIM2 for timing source for the Logic capture */
void TIM2init(){
	CLK_PCKENR1 |= 0x01;// enable TIM2 clokc
	
	// auto reload when counter reach 0x3FFF
	// this will trigger interrupt to change state that while loop checks
	// so we can stop the cnc_capture_start's while loop
	TIM2_ARRH = 0x03;
	TIM2_ARRL = 0xFF;

	// prescaler is 2^1 = 2, which is 8MHz Timer !
	TIM2_PSCR = 1; 

	//TIM2_EGR = 0x02;
	
	// enable update interrupt, interrupt fire whe counter reached Auto reload value
	TIM2_IER |= 0x01;

	// Enable TIM2 peripheral
	TIM2_BKR |= 0x80;

	// We will not start counter unless we want capturing to start
	TIM2_CR1 = 0x00;
}

/* Interrupt handler for TIM2 */
void TIM2isr(void) __interrupt(19){// the interrupt vector is 19 (from the STM8L151F36U datasheet page 49)
	capture = false;
	// clear TIM2's update interrupt flag
	TIM2_SR1 &= ~0x01;
}

// ComeNCapture functions  

// receive extra bytes from host after first byte command
void cnc_getcmd(){
	CapParam[1] = usart_read();
	CapParam[2] = usart_read();
	CapParam[3] = usart_read();
	CapParam[4] = usart_read();
}

// Report Metadata to SUMP OLS
void cnc_mtdtReport(){
	usart_write(0x01);// name report
	prntf("ComeNCap");
	usart_write(0x00);// end marker

	usart_write(0x02);// version report
	prntf("2.0");
	usart_write(0x00);// end marker

	// Sample memory size (16Kbytes)
	usart_write(0x21);// Sample size report in uint32_t format 
	usart_write(0x00);
	usart_write(0x00);
	usart_write(0x3F);
	usart_write(0xFF);

	// Sample rate, assuming it's 8MHz
	usart_write(0x23);// Sample rate report in uint32_t format 
	usart_write(0x00);
	usart_write(0x7A);
	usart_write(0x12);
	usart_write(0x00);

	// we have 5 usable channels 
	usart_write(0x40);// report channel number
	usart_write(0x05);// 5 channels
	
	// protocol version is  2 
	usart_write(0x41);// report protocol version
	usart_write(0x02);

	usart_write(0x00);// end marking
}

void cnc_capture_timing_setup(){
  if (divider >= 1500000) {
    useMicro = 0;
    delayTime = (divider + 1) / 100000;
  }
  else {
    useMicro = 1;
    delayTime = (divider + 1) / 100;
  }

}

// Start capturing 
void cnc_capture_start(){
	uint8_t wrtBF[3];
	wrtBF[0] = FM25_WRITE;
	wrtBF[1] = 0;
	wrtBF[2] = 0;

	PB_ODR &= (0 << SCS);

	SPI_Write(wrtBF, 3);// start writing F-RAM at offset = 0 
	// start counter
	TIM2_CR1 = 0x01;
	do{		
	__asm__("mov 0x5204, 0x5005");// load directly from mem2mem, 1 cpu cycle
	}while(capture);

	capture = true;// reset the loop value to true for next capture

	// stop counter. It's auto-reset.
	TIM2_CR1 = 0x00;
	
	PB_ODR |= (1 << SCS);
}

// Read data back from F-RAM and Dump to USART byte per byte
void cnc_capture_readback(){
	uint8_t wrtBF[3];
	wrtBF[0] = FM25_FSTRD;
	wrtBF[1] = 0;
	wrtBF[2] = 0;
	
	PB_ODR &= (0 << SCS);

	SPI_Write(wrtBF, 3);

	do{
	USART1_DR = SPI1_DR;
	while (!(USART1_SR & (1 << USART1_SR_TC)));
	}while(ramADDR--);

	PB_ODR |= (1 << SCS);
	ramADDR = 0x3FFF;
}

void main() {
	CLK_CKDIVR = 0x00;// set clock divider to 1, So we get the 16MHz for high TX rate and correct delay timing.
	usart_init(115200);// uses baud rate of 115200

	//remap UART hw pin to PA2 and PA3.
	SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); 
	SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);
	
	IOinit();// Init the GPIOs and SPI 
	TIM2init();// Start Timer2 
	__asm__("rim");// start interrupt system
	delay_ms(250);// wait for everything stable

	prntf(bootText);// Print boot text

	while (1) {
	if (USART1_SR & 0x20){// check if receive data is ready to be read.
	CapParam[0] = usart_read();
		switch (CapParam[0]){

		case SUMP_QUERY: 
			// return Query data to SUMP OLS
			prntf("1ALS");
			break;

		case SUMP_ARM: 
			// Arming 
			
			// Do capture stuffs 
			if (divider == 11){// 8MHz divider
			// Start F-RAM writing and GPIO read 
			cnc_capture_start();

			// Transmit F-RAM data over USART
			cnc_capture_readback();
			}

			break;
		
		case SUMP_TRIGGER_MASK:
			cnc_getcmd();
			// not implemented
			break;
		
		case SUMP_TRIGGER_VALUES:
			cnc_getcmd();
			// not implemented
			break;

		case SUMP_TRIGGER_CONFIG:
			cnc_getcmd();
			// not implemented
			break;

		case SUMP_SET_DIVIDER:
			cnc_getcmd();

			divider = CapParam[3];
			divider = divider << 8;
			divider += CapParam[2];
			divider = divider << 8;
			divider += CapParam[1];

			cnc_capture_timing_setup();
			break;

		case SUMP_SET_READ_DELAY_COUNT:
			cnc_getcmd();
			break;

		case SUMP_SET_FLAGS:
			cnc_getcmd();
			break;
	
		case SUMP_GET_METADATA: 
			cnc_mtdtReport();
			break;

		default:
			break;
		}// switch cases	
	}// if(usart available)
	
	}// while loop 
}
