#include <stdint.h>
#include <stdio.h>
#include <stm8l.h>
#include <string.h>
#include <delay.h>
#include <usart.h>

uint16_t REMAP_Pin = 0x011C; 
uint8_t Hour, Min, Sec;

int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

static uint8_t ByteToBcd2(uint8_t Value){
  uint8_t bcdhigh = 0;

  while (Value >= 10)
  {
    bcdhigh++;
    Value -= 10;
  }
  return  (uint8_t)((uint8_t)(bcdhigh << 4) | Value);
}


static uint8_t Bcd2ToByte(uint8_t Value){
  uint8_t tmp = 0;

  tmp = (uint8_t)((uint8_t)((uint8_t)(Value & (uint8_t)0xF0) >> 4) * (uint8_t)10);

  return (uint8_t)(tmp + (Value & (uint8_t)0x0F));
}

void liteRTC_Init(){ // initialize the clock stuff, and Start RTC
CLK_ICKCR=(1 << CLK_ICKCR_LSION);// turn LowSpeedInternal Clock
while (!(CLK_ICKCR & (1 << CLK_ICKCR_LSIRDY)));  // enable LSI oscillator.  

delay_ms(1000); // wait for the LSI clock to stable 	

CLK_CRTCR = ( ((uint8_t)0x04) | ((uint8_t)0x00) ); //set the RTC clock to LSI clock and set the divider at 1 
CLK_PCKENR2 |= (1 << 2);// enable rtc clock

	//unlock the writing protection
	RTC_WPR = 0xCA;
	RTC_WPR = 0x53;

 uint16_t initfcount = 0;

  /* Check if the Initialization mode is set, By checking bit 6*/
  if (!(RTC_ISR1 & 0x40))
  {
    /* Set the Initialization mode */
    RTC_ISR1 |= (uint8_t)(1 << 7);
    printf("RTC inited/n");
    /* Wait until INITF flag is set */
    while ((RTC_ISR1 & (1 << 6)) && ( initfcount != 0xFFFF))
    {
      initfcount++;
    }
  }

	//set Hour format to 24 hour
	RTC_CR1 &= (0 << 6);
	//Direct R/W on the TRs and DRs reg, By setting bit 4 (BYPSHAD) to 1 
	RTC_CR1 |= (1 << 4);

	//set the Prescalers regs
	RTC_SPRERH = 0x00;
	RTC_SPRERL = 0xFF;
	RTC_APRER  = 0x7F;	
	
	//exit init mode
	RTC_ISR1 &= (0 << 7);

	//lock write protection
	RTC_WPR = 0xFF; 
}

void liteRTC_SetDMY(uint8_t SetDate, uint8_t SetMonth, uint8_t SetYear){// Set Date Month and Year (Year number 1 is Jan and Year nuber 12 is Dec : Very simple All human can do it ;D)

	//unlock the writing protection
	RTC_WPR = 0xCA;
	RTC_WPR = 0x53;

 uint16_t initfcount = 0;

  /* Check if the Initialization mode is set, By checking bit 6*/
  if (!(RTC_ISR1 & 0x40))
  {
    /* Set the Initialization mode */
    RTC_ISR1 |= (uint8_t)(1 << 7);

    /* Wait until INITF flag is set */
    while ((RTC_ISR1 & (1 << 6)) && ( initfcount != 0xFFFF))
    {
      initfcount++;
    }
  }
	//dummy reading the TR1 reg
	(void)(RTC_TR1);

	//Set Date
	RTC_DR1 = ByteToBcd2(SetDate & 0x40);
	//Set Month
	RTC_DR2 = ByteToBcd2(SetMonth);
	//Set Year
	RTC_DR3 = ByteToBcd2(SetYear);

	//exit init mode
	RTC_ISR1 &= (0 << 7);

	//lock write protection
	RTC_WPR = 0xFF; 
}

void liteRTC_SetHMS(uint8_t SetHour, uint8_t SetMinute, uint8_t SetSecond){// Set the Hour (24Hrs format), Minute, Second
	//unlock the writing protection
	RTC_WPR = 0xCA;
	RTC_WPR = 0x53;

 uint16_t initfcount = 0;

  /* Check if the Initialization mode is set, By checking bit 6*/
  if (!(RTC_ISR1 & 0x40))
  {
    /* Set the Initialization mode */
    RTC_ISR1 |= (uint8_t)(1 << 7);
    printf("RTC Inited for time update\n");
    /* Wait until INITF flag is set */
    while ((RTC_ISR1 & (1 << 6)) && ( initfcount != 0xFFFF))
    {
      initfcount++;
    }
  }

	//Set Second
	RTC_TR1 = (uint8_t)ByteToBcd2(SetSecond);
	//Set Minute 
	RTC_TR2 = (uint8_t)ByteToBcd2(SetMinute);
	//Set Hour
	RTC_TR3 = (uint8_t)ByteToBcd2(SetHour);

	//Dummy reading DR3 reg to unfreeze calendar
	(void)(RTC_DR3);

	//exit init mode
	RTC_ISR1 &= (0 << 7);

	//lock write protection
	RTC_WPR = 0xFF; 
}

void liteRTC_grepTime(uint8_t *rH, uint8_t *rM, uint8_t * rS){
	*rS = Bcd2ToByte(RTC_TR1);
	*rM = Bcd2ToByte(RTC_TR2);
	*rH = Bcd2ToByte(RTC_TR3 & 0x3F);// Grep only BCD2 part (we exclude AM/PM notation bit).
}

void liteRTC_grepDate(uint8_t *rDa, uint8_t *rMo, uint8_t *rYe){
	*rDa = Bcd2ToByte(RTC_DR1);
	*rMo = Bcd2ToByte(RTC_DR2);
	*rYe = Bcd2ToByte(RTC_DR3 & 0x1F);// Grep only BCD2 part (we exclude WeekDay notation bit).
}

void main() {
	CLK_CKDIVR = 0x00;// No clock Divider, MAX 16MHz
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); //remap the non-exit pin of Tx and Rx of the UFQFPN20 package to the exit one.
	SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);
	delay_ms(100);
	liteRTC_Init();// init RTC system clock
	printf(" Starting Lite RTC...\n");
	delay_ms(1000);
	liteRTC_SetHMS(6, 9, 6);// Set time
    while (1) {
	liteRTC_grepTime(&Hour, &Min, &Sec);
	printf("Current Time is : %d : %d : %d\n",Hour, Min, Sec);
	delay_ms(999);
    }
}
