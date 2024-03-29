// STM8L RTC example
// Coded by TinLethax
#include <stdint.h>
#include <stdio.h>
#include <stm8l.h>
#include <string.h>
#include <delay.h>
#include <usart.h>

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

void RTC_Init(){ // initialize the clock stuff, and Start RTC
CLK_ICKCR=(1 << CLK_ICKCR_LSION);// turn LowSpeedInternal Clock
while (!(CLK_ICKCR & (1 << CLK_ICKCR_LSIRDY)));  // enable LSI oscillator.  

delay_ms(1000); // wait for the LSI clock to stable 	

CLK_CRTCR = 0x04; //set the RTC clock to LSI clock and set the divider at 1 
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
	RTC_ISR1 &= ~(1 << 7);

	//lock write protection
	RTC_WPR = 0xFF; 
}

/* Parameter descriptions
 * SetDay : Day of week, Starting with Monday as 0x01 and so on, MAx is 0x07 (Sunday)
 * SetDate : Date of that day. 1 to 31 (mind the leap year on Feb too, This one actually manage by calendar itself unless you update your own)
 * SetMonth : Starting from January (0x01) to December (0x12) 
 * SetYear : starting from 2000 (0) to 2099 (99) 
 */
void RTC_SetDMY(uint8_t SetDay,uint8_t SetDate, uint8_t SetMonth, uint8_t SetYear){// Set Date Month and Year (Year number 1 is Jan and Year nuber 12 is Dec : Very simple All human can do it ;D)

	//unlock the writing protection
	RTC_WPR = 0xCA;
	RTC_WPR = 0x53;

 uint16_t initfcount = 0;

  /* Check if the Initialization mode is set, By checking bit 6*/
  if (!(RTC_ISR1 & 0x40))
  {
    /* Set the Initialization mode */
    RTC_ISR1 |= (uint8_t)(1 << 7);
	delay_ms(10);// since the printf slowdown enough, this is not require, Go use liteRTC lib!
    /* Wait until INITF flag is set */
    while ((RTC_ISR1 & (1 << 6)) && ( initfcount != 0xFFFF))
    {
      initfcount++;
    }
  }
	//dummy reading the TR1 reg
	(void)(RTC_TR1);

	//Set Date
	RTC_DR1 = ByteToBcd2(SetDate);
	//Set Month
	RTC_DR2 = ByteToBcd2(SetMonth | (uint8_t)(SetDay << 5));
	//Set Year
	RTC_DR3 = ByteToBcd2(SetYear);

	//exit init mode
	RTC_ISR1 &= ~(1 << 7);

	//lock write protection
	RTC_WPR = 0xFF; 
}
/* Parameter descriptions
 * SetHour : Set Hour (0-23). This code is using 24 hour format
 * SetMinute : Set Minute (0-59)
 * SetSecond : Set Second (0-59)
 */
void RTC_SetHMS(uint8_t SetHour, uint8_t SetMinute, uint8_t SetSecond){// Set the Hour (24Hrs format), Minute, Second
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
	//delay_ms(10);// since the printf slowdown enough, this is not require, Go use liteRTC lib!
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
	RTC_ISR1 &= ~(1 << 7);

	//lock write protection
	RTC_WPR = 0xFF; 
}

/* Parameter descriptions
 * *rH : pointer to the variable storing Hour
 * *rM : pointer to the variable storing Minute
 * *rS : pointer to the variable storing Second
 */
void RTC_grepTime(uint8_t *rH, uint8_t *rM, uint8_t * rS){
	*rS = Bcd2ToByte(RTC_TR1);
	*rM = Bcd2ToByte(RTC_TR2);
	*rH = Bcd2ToByte(RTC_TR3 & 0x3F);// Grep only BCD2 part (we exclude AM/PM notation bit).
}

/* Parameter descriptions
 * *rDay : pointer to the variable storing Day of week (in form of number)
 * *rDate : pointer to the variable storing Date
 * *rMo : pointer to the variable storing Month (in the form of number)
 * *rYe : pointer to the variable storing Year (two last digit 20XX).
 */
void RTC_grepDate(uint8_t *rDay, uint8_t *rDate, uint8_t *rMo, uint8_t *rYe){
	*rDate = Bcd2ToByte(RTC_DR1);
	*rMo = Bcd2ToByte(RTC_DR2);
	*rYe = Bcd2ToByte(RTC_DR3);
	
	*rDay = *rDate << 5;// Grep Day of week number
	*rDate = *rDate & 0x1F;// Clean out Day of week number from Date variable.
}

void main() {
	CLK_CKDIVR = 0x00;// No clock Divider, MAX 16MHz
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	delay_ms(100);
	RTC_Init();// init RTC system clock
	printf(" Starting Lite RTC...\n");
	delay_ms(1000);
	RTC_SetHMS(6, 9, 6);// Set time
    while (1) {
	RTC_grepTime(&Hour, &Min, &Sec);
	printf("Current Time is : %d : %d : %d\n",Hour, Min, Sec);
	delay_ms(999);
    }
}
