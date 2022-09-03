// CDM102 Bed clock. FINALLY, I HAVE A DESK CLOCK!
// Coded by TinLethax 2022/09/01 +7 
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stm8l.h>
#include <delay.h>

#include <spi.h>
#include <liteRTC.h>

#define ENCA	0	// PB0
#define ENCB	2	// PB2
#define ENCBTN	1 	// PB1

// Pinout and connection.
// CDM102      STM8L151F3
// 1. Load		PC4
// 2. Vcc		Vcc (+3v3 works)
// 3. SCLK		PB5
// 4. GND		GND
// 5. SDI		PB6
// 6. Reset		PC5

#define SPI_CS	4// PC4 will be used as CS pin
#define SPI_RST	5// PC5 for display reset.

#define SPI_SEL		PC_ODR &= ~(1 << SPI_CS)
#define SPI_UNSEL   PC_ODR |= (1 << SPI_CS)

uint16_t encoder_cnt = 0;
uint16_t encoder_prev = 1;

volatile uint8_t Hrs, Mins, Secs;

#define SCREEN_TIMEOUT	5 // screen timeout 5 secs. since we use .5s counter (1 second counter is counting up 2 times).
volatile uint8_t countdown = 0;// screen timout keeper.
volatile uint8_t readPin = 0;

uint16_t blink_cnt = 0;

uint8_t TimeUpdate_lock = 0;
volatile uint8_t digitVal[6] = {0};

const uint8_t timeMaxVal[6] = {2, 9, 5, 9, 5, 9};

/* USER CODE BEGIN PV */
const uint8_t byte_rev_table[256] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

// 5x3 pixel fonts. Designed by me UwU.
const uint8_t Num_font[36] =
{	0x0F, 0x11, 0x1E,// 0
	0x02, 0x1F, 0x00,// 1
	0x19, 0x15, 0x17,// 2
	0x15, 0x15, 0x0F,// 3
	0x03, 0x04, 0x1F,// 4
	0x17, 0x15, 0x19,// 5
	0x1F, 0x15, 0x19,// 6
	0x01, 0x01, 0x1D,// 7
	0x0B, 0x15, 0x1A,// 8
	0x07, 0x05, 0x1E, // 9
	0x00, 0x00, 0x00,// Blank
	0x1F, 0x1F, 0x1F// lit all LED.
};

// Init GPIOs for Joy stock button and CS pin
void GPIO_init(){
	// Init PC4 as push-pull output and set at High level
	PC_DDR |= (1 << SPI_CS) | (1 << SPI_RST);
	PC_CR1 |= (1 << SPI_CS) | (1 << SPI_RST);
	PC_ODR |= (1 << SPI_CS) | (1 << SPI_RST);
	
	PB_CR1 |= (1 << ENCA) | (1 << ENCB) | (1 << ENCBTN);// set all encoder pin to have pull up.
	PB_CR2 |= (1 << ENCBTN);// Push button as interrupt

	// Enable Port B interrupt.
	ITC_SPR2 |= (3 << 4);// set VECT6SPR (Entire portB interrupt) priority to high.
	
	// We need to do the EXTI setup in sequence.
	EXTI_CR3 = 0x02;// set interrupt sensitivity to detect falling edge(PortB).
	EXTI_CONF1 |= 0x03;// Enable Port B interrupt source.
	EXTI_CONF2 &= (uint8_t)~0x20;// Port B instead of Port G is used for interrupt generation.
	
}

void tim2_encinit(){
	CLK_PCKENR1 |= 0x01;// Enable TIM2 timer
	//Timer 2 init 
	TIM2_ARRH = 0x03;
	TIM2_ARRL = 0xE7;

	TIM2_PSCR = (uint8_t)4; 

	TIM2_EGR = 0x01;

	// TIME2 capture mode 2 channels.
	
	// Set Capture compare 1 and 2 as input.
	uint8_t tmpccmr1 = TIM2_CCMR1;
	uint8_t tmpccmr2 = TIM2_CCMR2;
	tmpccmr1 &= ~0x03;
	tmpccmr1 |= 0xA1;
	tmpccmr2 &= ~0x03;
	tmpccmr2 |= 0xA1;
	
	// set Enacoder polarity detecion (Falling edge detection).
	TIM2_CCER1 |= 0x22; 
	
	TIM2_SMCR |= 0x03;// Set encoder mode to 2 channels mode.
	TIM2_CCMR1 = tmpccmr1;
	TIM2_CCMR2 = tmpccmr2;
	
	
	//Enable TIM2
	TIM2_CR1 = 0x01;
}

void cdm102_rst(){
	PC_ODR |= (1 << SPI_RST);
	PC_ODR &= ~(1 << SPI_RST);
	delay_ms(100);
	PC_ODR |= (1 << SPI_RST);
}

// Send byte to the CDM102.
void cdm102_tx(uint8_t data){
	data = byte_rev_table[data];
	SPI_SEL;
	SPI_Write(&data, 1);
	SPI_UNSEL;
}

// Send multiple byte to the display.
void cdm102_tx_multiple(uint8_t *data, uint8_t len){
	for(uint8_t i=0; i < len; i++)
		cdm102_tx(*(data + i));
}


// Write number to the display.
void cdm102_numrndr(uint8_t digit1, uint8_t digit2, uint8_t digit3, uint8_t digit4){

	if((digit1 > 11) || (digit2 > 11) || (digit3 > 11) || (digit4 > 11))
		return;
	
	// Convert number into pointer address, pointing to the font of that number.
	digit1 *= 3;
	digit2 *= 3;
	digit3 *= 3;
	digit4 *= 3;

	cdm102_tx(0xA2);
	cdm102_tx_multiple((uint8_t *)(Num_font + digit1), 3);
	cdm102_tx_multiple((uint8_t *)(Num_font + digit2), 3);
	cdm102_tx(0xA1);
	cdm102_tx_multiple((uint8_t *)(Num_font + digit3), 3);
	cdm102_tx_multiple((uint8_t *)(Num_font + digit4), 3);
}

// Every 1 Minute, RTC will trigger the interrupt controller.
// Then we'll update the number on the display.
void cdm102_displayUpdate() __interrupt(4){
	RTC_ISR2 = 0x00;// Clear WUTF interrupt flag.
	Secs = RTC_TR1;
	Mins = RTC_TR2;
	Hrs = RTC_TR3 & 0x3F;
	cdm102_numrndr(Hrs >> 4, Hrs & 0x0F,
					Mins >> 4, Mins & 0x0F);
}

// Handle Joy stick button press.
void joystick_irq(void) __interrupt(6){
	readPin = PB_IDR;// quickly read the GPIO for further mode select.

	//reset_countdown_timer
	countdown = RTC_TR1;// recount timer.
	
	EXTI_SR2 = 0x01;// clear interrupt pending bit of PortB interrupt.
}

uint8_t Switcher = 0;
uint8_t enc_fsm = 0 ;

void watch_Update(){
	uint8_t Xcol = 1;
	readPin = 0xFF;// reset pin read state
	cdm102_tx(0xC0);// Display clear
	countdown = RTC_TR1;// first lap
	
	Secs = RTC_TR1;
	Mins = RTC_TR2;
	Hrs = RTC_TR3 & 0x3F;
	digitVal[0] = Hrs >> 4;
	digitVal[1] = Hrs & 0x0F;
	digitVal[2] = Mins >> 4;
	digitVal[3] = Mins & 0x0F;
	digitVal[4] = Secs >> 4;
	digitVal[5] = Secs & 0x0F;
	
	while ((RTC_TR1 - countdown) < (SCREEN_TIMEOUT*2)){
		// FSM for Rotary Encoder 
		switch(enc_fsm){
		case 0: // Encoder select digit
			if(!(PB_IDR & (1 << ENCBTN))){// If detected key press. Move to next FSM state.
				while(!(PB_IDR & (1 << ENCBTN)));
				enc_fsm = 1;
			}
			
			// Sample the Encoder 
			encoder_cnt = (TIM2_CNTRH << 8) | TIM2_CNTRL; 

			if(encoder_cnt > encoder_prev){// We have increment
				countdown = RTC_TR1;// first lap
				// Move Cursor forward.
				Xcol++;
					
				if (Xcol > 6)// Cursor never goes beyond 6 (4 digits 1-4 plus 2 digit for seconds).
					Xcol = 6;
				
			}else if(encoder_cnt < encoder_prev){// We have decrement
				countdown = RTC_TR1;// first lap
				// move cursor backward.
				if (Xcol == 1)// Cursor never goes backward beyond 1.
					break;
					
				Xcol--;
				
			}

			encoder_prev = encoder_cnt;
			
			
			break;
			
		case 1:
			if(!(PB_IDR & (1 << ENCBTN))){// If detected key press. Move back to first FSM state.
				__asm__("sim");// spinlock
				liteRTC_SetHMSBCD(((digitVal[0]) << 4) | (digitVal[1]), ((digitVal[2]) << 4) | (digitVal[3]), ((digitVal[4]) << 4) | (digitVal[5]));
				__asm__("rim");// de-spinlock
				while(!(PB_IDR & (1 << ENCBTN)));
				enc_fsm = 0;
			}
		
			// Sample the Encoder 
			encoder_cnt = (TIM2_CNTRH << 8) | TIM2_CNTRL; 

			if(encoder_cnt > encoder_prev){// We have increment
				countdown = RTC_TR1;// first lap
				// Increase value (of that digit).
				// Time Update
				digitVal[Xcol-1]++;
				
				// check whether the number is out of bound. 
				if(digitVal[Xcol-1] > timeMaxVal[Xcol-1])
					digitVal[Xcol-1] = timeMaxVal[Xcol-1];
				
				// Hour maximum is 23
				if((digitVal[0] == 2) && (digitVal[1] > 3))// Hour can't greater than 23.
						digitVal[1] = 3;
						
			}else if(encoder_cnt < encoder_prev){// We have decrement
				countdown = RTC_TR1;// first lap
				// Decrease value (of that digit).
				if(digitVal[Xcol-1] == 0)// never goes below 0 and get rick roll over.
					break;
				digitVal[Xcol-1]--;
				
			}

			encoder_prev = encoder_cnt;
			
			break;
		}
			
		blink_cnt++;
		if(blink_cnt == 640){
			Switcher ^= 1;
			blink_cnt = 0;
		}

		switch(Xcol){
		case 1: //Selecting digit 1
			if(Switcher)
				cdm102_numrndr(digitVal[0], digitVal[1],
						digitVal[2], digitVal[3]);
			else
				cdm102_numrndr(10, digitVal[1],
						digitVal[2], digitVal[3]);
						
			break;
			
		case 2:// Selecting digit 2
			if(Switcher)
				cdm102_numrndr(digitVal[0], digitVal[1],
						digitVal[2], digitVal[3]);
			else
				cdm102_numrndr(digitVal[0], 10,
						digitVal[2], digitVal[3]);
			break;
			
		case 3:// Selecting digit 3
			if(Switcher)
				cdm102_numrndr(digitVal[0], digitVal[1],
						digitVal[2], digitVal[3]);
			else
				cdm102_numrndr(digitVal[0], digitVal[1],
						10, digitVal[3]);
			break;
			
		case 4:// Selecting digit 4
			if(Switcher)
				cdm102_numrndr(digitVal[0], digitVal[1],
						digitVal[2], digitVal[3]);
			else
				cdm102_numrndr(digitVal[0], digitVal[1],
						digitVal[2], 10);
			break;
			
		case 5:// Selecting Seconds digit 1
			if(Switcher)
				cdm102_numrndr(10, digitVal[4],
						digitVal[5], 10);
			else
				cdm102_numrndr(10, 10,
						digitVal[5], 10);
			break;
			
		case 6:// Selecting Seconds digit 2
			if(Switcher)
				cdm102_numrndr(10, digitVal[4],
						digitVal[5], 10);
			else
				cdm102_numrndr(10, digitVal[4],
						10, 10);
			break;
			
		default:
			break;
		}
		
		readPin = 0xFF;
		//delay_ms(1);
	}

	
	Secs = RTC_TR1;
	Mins = RTC_TR2;
	Hrs = RTC_TR3 & 0x3F;
	cdm102_tx(0xF0);
	cdm102_numrndr(Hrs >> 4, Hrs & 0x0F,
				Mins >> 4, Mins & 0x0F);
	
}

void Clock_handler(){
						
	countdown = RTC_TR1;// snapshot of start time.
	while ((RTC_TR1 - countdown) < SCREEN_TIMEOUT){// find delta for how long does this loop currenty take.
		
		if(!(readPin & (1 << ENCBTN))){
			TimeUpdate_lock +=1;
			if(TimeUpdate_lock == 2){// If pressed 2 times.
				TimeUpdate_lock = 0;// release Time update lock.
				watch_Update();// enter time/date update mode.
			}
		}
		readPin = 0xFF;// reset pin read state
	}
	
	__asm__("wfi");
}

void main() {
	CLK_CKDIVR = 0x00;// Full 16MHz
	GPIO_init();
	tim2_encinit();
	__asm__("rim");
	SPI_Init(FSYS_DIV_4);// 16MHz/16 = 1Mhz SPI clock
	cdm102_rst(); //reset display for clean init.
	cdm102_tx(0xDF);
	liteRTC_Init();// Turn LSI on and Initialize RTC
	delay_ms(1000);
	Secs = RTC_TR1;
	Mins = RTC_TR2;
	Hrs = RTC_TR3 & 0x3F;
	cdm102_numrndr(Hrs >> 4, Hrs & 0x0F,
					Mins >> 4, Mins & 0x0F);
	readPin = PB_IDR;
	TimeUpdate_lock = 0;
    while (1) {
		Clock_handler();
    }
	
	
}
/*
		encoder_cnt = (TIM2_CNTRH << 8) | TIM2_CNTRL; 
		if(encoder_cnt != encoder_prev){// We have increment
				cdm102_numrndr((uint8_t)((encoder_cnt/1000) % 10), (uint8_t)((encoder_cnt/100) % 10), (uint8_t)((encoder_cnt/10) % 10), (uint8_t)(encoder_cnt % 10));
		}
		
		encoder_prev = encoder_cnt;
*/