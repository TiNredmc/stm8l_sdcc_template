/* ComeNCapture is a quick-n-dirty logic analyzer 
 * Coded By TinLethax 2020/05/23
 * updated 2021/07/02 -> now adding compatibilities with SUMP OLS.
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
#define SCS	0 // PD4 as a CS pin

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

/*capture mode*/
#define CAP8M 	8
#define CAP4M 	4
#define CAP2M 	2
#define CAP1M 	1
#define CAP500K 50
#define CAP200K	20
#define CAP100K	10

// For incoming capture command/params
uint8_t CapParam[5];// capture parameter received from host PC running SUMP OLS

// For capture timing
uint32_t divider = 0;// divider using for capturing frequency 
uint16_t useMicro = 0;
uint16_t delayTime  = 0;

/* USART */
// pin remap
const uint16_t REMAP_Pin = 0x011C; //constant value for sysconf to remap USART pin

/* for FM25V01A */

// FM25V01A necessary opcode
// Opcode (commands)
const static uint8_t FM25_WREN		= 0x06;// Set write enable latch

const static uint8_t FM25_READ		= 0x03;// Read memory data

const static uint8_t FM25_WRITE		= 0x02;// Write memory data 

// capture loop locking status
static uint8_t capture = 1;

// F-RAM address counter keeper for reading and slow write 
uint16_t ramADDR = 0x3FFF;// maximum F-RAM address for roll-counting 
// Related with Sample size for capturing speed lower than 100KHz 
// (that will make more sense as lower the speed, longer it takes to fills 16KBytes).
uint16_t readCount = 0x3FFF;
uint16_t delayCount = 0;

/////////////////////////////////////////////////////////////////////////
///////// Normal I/O stuffs /////////////////////////////////////////////
/* Initialize all PortB from 0-4 as input
 * Initialize PC4 as SCS pin also Init SPI stuffs too
 */
void IOinit(){
	PB_DDR = 0; // set all pin at input
	PB_CR1 = 0; // floating input
	PB_CR2 = 0; // No interrupt

	// Setup PD0 as CS pin for SPI 
	PD_DDR |= (1 << SCS);
	PD_CR1 |= (1 << SCS);

	PD_ODR |= (1 << SCS);

	//then re initialize SPI-needed pins
	SPI_Init(FSYS_DIV_2);// Init SPI peripheral with 8MHz clock (Max speed tho).
}

// usart print function
// I don't use the printf becuase that code takes a lot of space.
void prntf(char *p){
	__asm__("sim");
	while(*p){
		usart_write(*p++);
	}
	__asm__("rim");
}

/////////////////////////////////////////////////////////////////////////
///////// Timer stuffs //////////////////////////////////////////////////
/* Initialize the TIM2 for timing source for the Logic capture */
void TIM2init(){
	CLK_PCKENR1 |= 0x01;// enable TIM2 clock
	
	// auto reload when counter reach 0x3FFF
	// this will trigger interrupt to change state that while loop checks
	// so we can stop the cnc_capture_start's while loop
	//TIM2_ARRH = 0x03;
	//TIM2_ARRL = 0xFF;

	// prescaler is 2^1 = 2, which is 8MHz Timer !
	//TIM2_PSCR = 1; 

	// enable update interrupt, interrupt fire whe counter reached Auto reload value
	TIM2_IER |= 0x01;

	// We will not start counter unless we want capturing to start
	TIM2_CR1 = 0x00;
}

/* re init the TIM2 everytime before capturing. */
void TIM2_capture(uint8_t pscr){
	// auto reload when counter reach 0x3FFF
	// this will trigger interrupt to change state that while loop checks
	// so we can stop the cnc_capture_start's while loop
	TIM2_ARRH = 0x03;
	TIM2_ARRL = 0xFF;

	// prescaler is used for dividing System clock (16MHz)
	// for running the counter at certain speed
	// Can be calculated by F_SYS/(2 ^ TIM2_PSCR) (floor value, integer only, ignore decimal).
	
	// pscr = 0 -> 2^0 = 1 pactically no divider (16MHz)
	// pscr = 1 -> 8MHz capturing 
	// pscr = 2 -> 4MHz capturing
	// pscr = 3 -> 2MHz capturing
	// pscr = 4 -> 1MHz capturing
	// pscr = 5 -> 500KHz capturing 
	// pscr = 6 -> 200KHz capturing
	// pscr = 7 -> 100KHz capturing 
	TIM2_PSCR = pscr;

	// Clear prescaler everytime after counter reach ARR value.
	TIM2_EGR = 0x01;// event source update 	
}
 
/* Interrupt handler for TIM2 */
void TIM2isr(void) __interrupt(19){// the interrupt vector is 19 (from the STM8L151F36U datasheet page 49)
	capture = 0;
	//USART1_DR = 0x69;
	//while (!(USART1_SR & (1 << USART1_SR_TC)));
	// clear TIM2's update interrupt flag
	TIM2_SR1 &= ~0x01;
}

/* trap the bad guy iqr */
// I swear, I haven't turn on the USART interrupt yet
// But It's turns out that CPU reset itself when there's no irq handler for the
// Interrupt that I'M NOT AWARE OF (or event enable it D:).
void usart1TX_irq(void) __interrupt(27){}
void usart1RX_irq(void) __interrupt(28){}
void spi_irq(void) __interrupt(26){}
// ComeNCapture functions  

// receive extra bytes from host after first byte command
void cnc_getcmd(){
	while (!(USART1_SR & (1 << USART1_SR_RXNE)));
	CapParam[1] = USART1_DR;
	while (!(USART1_SR & (1 << USART1_SR_RXNE)));
	CapParam[2] = USART1_DR;
	while (!(USART1_SR & (1 << USART1_SR_RXNE)));
	CapParam[3] = USART1_DR;
	while (!(USART1_SR & (1 << USART1_SR_RXNE)));
	CapParam[4] = USART1_DR;
}

// transmit extrabyte to host, for debugging purpose only.
void cnc_prntcmd(){
	USART1_DR = CapParam[0];
	while (!(USART1_SR & (1 << USART1_SR_TC)));
	USART1_DR = CapParam[1];
	while (!(USART1_SR & (1 << USART1_SR_TC)));
	USART1_DR = CapParam[2];
	while (!(USART1_SR & (1 << USART1_SR_TC)));
	USART1_DR = CapParam[3];
	while (!(USART1_SR & (1 << USART1_SR_TC)));
	USART1_DR = CapParam[4];
	while (!(USART1_SR & (1 << USART1_SR_TC)));
}

void cnc_cmdcleanup(){
	for(uint8_t i=0;i < 5;i++)
		CapParam[i] = 0;
}
// Report Metadata to SUMP OLS
void cnc_mtdtReport(){
	usart_write(0x01);// name report
	prntf("ComeNCapture");
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
void cnc_capture_start(uint8_t samspd){
	uint8_t wrtBF[3];
	wrtBF[0] = FM25_WRITE;
	wrtBF[1] = 0;
	wrtBF[2] = 0;

	//prntf("capture started!\n");
	PD_ODR &= (0 << SCS);

	SPI_Write(wrtBF, 3);// start writing F-RAM at offset = 0 

	switch(samspd){
	case CAP8M:
		// start counter
		TIM2_CR1 = 0x01;
		do{		
		__asm__("mov 0x5204, 0x5006");// load directly from mem2mem, 1 cpu cycle
		}while(capture);
		// stop counter. It's auto-reset.
		TIM2_CR1 = 0x00;
		break;
		
	case CAP4M:
		// start counter
		TIM2_CR1 = 0x01;
		do{		
		__asm__("mov 0x5204, 0x5006");// load directly from mem2mem, 1 cpu cycle
		__asm__("nop\n nop\n");
		}while(capture);
		// stop counter. It's auto-reset.
		TIM2_CR1 = 0x00;
		break;
		
	case CAP2M:
		// start counter
		TIM2_CR1 = 0x01;
		do{		
		__asm__("mov 0x5204, 0x5006");// load directly from mem2mem, 1 cpu cycle
		__asm__("nop\n nop\n nop\n nop\n");
		}while(capture);
		// stop counter. It's auto-reset.
		TIM2_CR1 = 0x00;
		break;
		
	case CAP1M:
		// start counter
		TIM2_CR1 = 0x01;
		do{		
		__asm__("mov 0x5204, 0x5006");// load directly from mem2mem, 1 cpu cycle
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		}while(capture);
		// stop counter. It's auto-reset.
		TIM2_CR1 = 0x00;
		break;
		
	case CAP500K:
		// start counter
		TIM2_CR1 = 0x01;
		do{		
		__asm__("mov 0x5204, 0x5006");// load directly from mem2mem, 1 cpu cycle
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		}while(capture);
		// stop counter. It's auto-reset.
		TIM2_CR1 = 0x00;
		break;
	
	case CAP200K:
		// start counter
		TIM2_CR1 = 0x01;
		do{		
		__asm__("mov 0x5204, 0x5006");// load directly from mem2mem, 1 cpu cycle
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		}while(capture);
		// stop counter. It's auto-reset.
		TIM2_CR1 = 0x00;
		break;
	
	case CAP100K:
		// start counter
		TIM2_CR1 = 0x01;
		do{		
		__asm__("mov 0x5204, 0x5006");// load directly from mem2mem, 1 cpu cycle
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		__asm__("nop\n nop\n nop\n nop\n");
		}while(capture);
		// stop counter. It's auto-reset.
		TIM2_CR1 = 0x00;
		break;
		
	default:
		break;
	}
	
	capture = 1;// reset the loop value to true for next capture

	PD_ODR |= (1 << SCS);
}

// Read data back from F-RAM and Dump to USART byte per byte
void cnc_capture_readback(){
	uint8_t wrtBF[3];
	wrtBF[0] = FM25_READ;
	wrtBF[1] = 0;
	wrtBF[2] = 0;

	
	PD_ODR &= (0 << SCS);

	SPI_Write(wrtBF, 3);
	//prntf("ready to send\n");

	do{
	USART1_DR = SPI1_DR;
	while (!(USART1_SR & (1 << USART1_SR_TC)));
	}while(ramADDR--);

	PD_ODR |= (1 << SCS);
	ramADDR = 0x3FFF;

}

// Read data back from F-RAM and Dump to USART byte per byte
// But with variable sample size. (for capture speed < 100KHz).
void cnc_lowcapture_readback(){
	readCount--;// decrease 1 step for while loop
	
	uint8_t wrtBF[3];
	wrtBF[0] = FM25_READ;
	wrtBF[1] = 0;
	wrtBF[2] = 0;

	
	PD_ODR &= (0 << SCS);

	SPI_Write(wrtBF, 3);
	//prntf("ready to send\n");

	do{
	USART1_DR = SPI1_DR;
	while (!(USART1_SR & (1 << USART1_SR_TC)));
	}while(readCount--);

	PD_ODR |= (1 << SCS);
}

//FM25V01A : enable writing
void FM25_unlock(){
	PD_ODR &= ~(1 << SCS);
	SPI_Write(&FM25_WREN, 1);
	PD_ODR |= (1 << SCS);
}

void main() {
	CLK_CKDIVR = 0x00;// set clock divider to 1, So we get the 16MHz for high TX rate and correct delay timing.
	usart_init(57600);// uses baud rate of 57600

	//remap UART hw pin to PA2 and PA3.
	SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); 
	SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);
	prntf(" ComeNCapture 2.0\n");// Print boot text

	IOinit();// Init the GPIOs and SPI 
	TIM2init();// Start Timer2 
	FM25_unlock();// Unlock write protection of FM25V01A
	__asm__("rim");// start system interrupt
	delay_ms(500);// wait for everything stable

	while (1) {
		if(USART1_SR & (1 << USART1_SR_RXNE)){// check if receive data is ready to be read.
		CapParam[0] = usart_read();
		switch (CapParam[0]){

		case SUMP_QUERY: 
			// return Query data to SUMP OLS
			// This will be identified before start any capturing
			prntf("1ALS");
			break;

		case SUMP_ARM: 
			// Arming 
			//prntf("Arm\n");
			// Do capture stuffs 
			if (divider == 11){// divider = 11, 8MHz capture
			//prntf("capture start\n");
			// Setup timer2 
			TIM2_capture(1);
			// Start F-RAM writing and GPIO read 
			cnc_capture_start(CAP8M);

			// Transmit F-RAM data over USART
			cnc_capture_readback();
			
			} else if (divider == 24){// divider = 24, 4MHz capture
			// Setup timer2 
			TIM2_capture(2);
			// Start F-RAM writing and GPIO read 
			cnc_capture_start(CAP4M);

			// Transmit F-RAM data over USART
			cnc_capture_readback();
			
			}else if (divider == 49){// divider = 49, 2MHz capture
			// Setup timer2 
			TIM2_capture(3);
			// Start F-RAM writing and GPIO read 
			cnc_capture_start(CAP2M);

			// Transmit F-RAM data over USART
			cnc_capture_readback();
			
			}else if (divider == 99){// divider = 99, 1MHz capture
			// Setup timer2 
			TIM2_capture(4);
			// Start F-RAM writing and GPIO read 
			cnc_capture_start(CAP2M);

			// Transmit F-RAM data over USART
			cnc_capture_readback();
			
			}else if (divider == 199){// divier = 199, 500KHz capture 
			// Setup timer2 
			TIM2_capture(5);
			// Start F-RAM writing and GPIO read 
			cnc_capture_start(CAP500K);

			// Transmit F-RAM data over USART
			cnc_capture_readback();
				
			}else if (divider == 499){// divier = 499, 200KHz capture 
			// Setup timer2 
			TIM2_capture(6);
			// Start F-RAM writing and GPIO read 
			cnc_capture_start(CAP200K);

			// Transmit F-RAM data over USART
			cnc_capture_readback();
				
			}else if (divider == 999){// divier = 999, 100KHz capture 
			// Setup timer2 
			TIM2_capture(7);
			// Start F-RAM writing and GPIO read 
			cnc_capture_start(CAP100K);

			// Transmit F-RAM data over USART
			cnc_capture_readback();
					
			}else{
			// Low speed capture, relay on delay_ms and delay_us.
				if(useMicro){// Higher speed capture 10-50KHz
					for(uint16_t i = 0; i < readCount;i++){
						__asm__("mov 0x5204, 0x5006"); // use mov to copy data directly mem2mem from PB_IDR to SPI1_DR
						delay_us(delayTime);	
					}
					
				}else{// Lower speed capture 10Hz-5KHz
					for(uint16_t i = 0; i < readCount; i++){
						__asm__("mov 0x5204, 0x5006"); // use mov to copy data directly mem2mem from PB_IDR to SPI1_DR
						delay_ms(delayTime);
					}
				}
				
				// Transmit F-RAM data over USART 
				cnc_lowcapture_readback();
			}
			//prntf("arm done!\n");

			break;
		
		case SUMP_TRIGGER_MASK:
			cnc_getcmd();
			// not implemented
			//prntf("Trigger Mask\n");
			break;
		
		case SUMP_TRIGGER_VALUES:
			cnc_getcmd();
			// not implemented
			//prntf("Trigger Val\n");
			break;

		case SUMP_TRIGGER_CONFIG:
			cnc_getcmd();
			// not implemented
			//prntf("Trigger Conf\n");
			break;

		case SUMP_SET_DIVIDER:
			cnc_getcmd();
	
			divider = CapParam[3];
			divider = divider << 8;
			divider += CapParam[2];
			divider = divider << 8;
			divider += CapParam[1];

			cnc_capture_timing_setup();
			//prntf("Set Div\n");
			break;

		case SUMP_SET_READ_DELAY_COUNT:// setting up sample size, Only apply for capture < 100KHz 
			cnc_getcmd();

			readCount = 4 * (((CapParam[2] << 8) | CapParam[1]) + 1);
				if (readCount > ramADDR)
				readCount = ramADDR;
			delayCount = 4 * (((CapParam[4] << 8) | CapParam[3]) + 1);
				if (delayCount > ramADDR)
				delayCount = ramADDR;
			
			//prntf("Read delay\n");
			break;

		case SUMP_SET_FLAGS:
			cnc_getcmd();
			//prntf("Set Flags\n");
			break;
	
		case SUMP_GET_METADATA: 
			cnc_mtdtReport();
			break;

		default:
			break;
		}// switch cases	
		cnc_cmdcleanup();
		}// if(usart available)
	
	}// while loop 
}
