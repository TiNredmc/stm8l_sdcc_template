/* liteRTC, Using Real time clock of STM8L151F3 easily
 * Coded By TinLethax way back in May. Converted to library at 2021/09/07 +7
 */
 
#include <stdint.h>
#include <stdio.h>
#include <stm8l.h>
#include <string.h>

#include <delay.h>

#define RTC_WAKEUP // Use periodic auto wakeup to update the display every 1 minute.


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

//delay_ms(1000); // wait for the LSI clock to stable 	

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

    /* Wait until INITF flag is set */
    while ((RTC_ISR1 & (1 << 6)) && ( initfcount != 0xFFFF))
    {
      initfcount++;
    }
  }

	//set Hour format to 24 hour
	// RTC_CR1 &= ~(1 << 6);
	//Direct R/W on the TRs and DRs reg, By setting bit 4 (BYPSHAD) to 1
#ifdef RTC_WAKEUP	
	// Using 1 Minutes wake up interrupt for updating Display.
	RTC_CR1 = 0x10 | 0x04;// Bypass shadown regs, 1Hz wake up clock.
		
	RTC_WUTRH = 0x00;
	RTC_WUTRL = 60;// 60 * 1Hz == Wake up every 1 minute.
	
	RTC_CR2 |= 0x44;// Enable Wakeup timer and its interrupt.
#else
	RTC_CR1 = 0x10;// Bypass shadow regs.
#endif

	//set the Prescalers regs
	RTC_SPRERH = 0x00;
	RTC_SPRERL = 0xFF;
	RTC_APRER  = 0x7F;	
	
	//exit init mode
	RTC_ISR1 &= ~(1 << 7);

	//lock write protection
	RTC_WPR = 0xFF; 
}

/* Parameter descriptions (In BCD format).
 * SetDay : Day of week, Starting with Monday as 0x01 and so on, MAx is 0x07 (Sunday)
 * SetDate : Date of that day. 1 to 31 (mind the leap year on Feb too, This one actually manage by calendar itself unless you update your own)
 * SetMonth : Starting from January (0x01) to December (0x12) 
 * SetYear : starting from 2000 (0) to 2099 (99) 
 */
void liteRTC_SetDMYBCD(uint8_t SetDay,uint8_t SetDate, uint8_t SetMonth, uint8_t SetYear){// Set Date Month and Year (Year number 1 is Jan and Year nuber 12 is Dec : Very simple All human can do it ;D)
	//unlock the writing protection
	RTC_WPR = 0xCA;
	RTC_WPR = 0x53;

 uint16_t initfcount = 0;

  /* Check if the Initialization mode is set, By checking bit 6*/
  if (!(RTC_ISR1 & 0x40))
  {
    /* Set the Initialization mode */
    RTC_ISR1 |= (uint8_t)(1 << 7);
	delay_ms(10);// Our CPU is too fast, so we need to wait a bit
    /* Wait until INITF flag is set */
    while ((RTC_ISR1 & (1 << 6)) && ( initfcount != 0xFFFF))
    {
      initfcount++;
    }
  }
	//dummy reading the TR1 reg to unfreze the calendar
	(void)(RTC_TR1);

	//Set Date
	RTC_DR1 = (uint8_t)SetDate;
	//Set Month
	RTC_DR2 = (uint8_t)SetMonth | (uint8_t)(SetDay << 5);
	//Set Year
	RTC_DR3 = (uint8_t)SetYear;

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
void liteRTC_SetDMY(uint8_t SetDay,uint8_t SetDate, uint8_t SetMonth, uint8_t SetYear){// Set Date Month and Year (Year number 1 is Jan and Year nuber 12 is Dec : Very simple All human can do it ;D)
	liteRTC_SetDMYBCD(SetDay, ByteToBcd2(SetDate), ByteToBcd2(SetMonth), ByteToBcd2(SetYear));
}

/* Parameter descriptions (In BCD format).
 * SetHour : Set Hour (0-23). This code is using 24 hour format
 * SetMinute : Set Minute (0-59)
 * SetSecond : Set Second (0-59)
 */
void liteRTC_SetHMSBCD(uint8_t SetHour, uint8_t SetMinute, uint8_t SetSecond){// Set the Hour (24Hrs format), Minute, Second
	//unlock the writing protection
	RTC_WPR = 0xCA;
	RTC_WPR = 0x53;

 uint16_t initfcount = 0;

  /* Check if the Initialization mode is set, By checking bit 6*/
  if (!(RTC_ISR1 & 0x40))
  {
    /* Set the Initialization mode */
    RTC_ISR1 |= (uint8_t)(1 << 7);
    delay_ms(10);// Our CPU is too fast, so we need to wait a bit 
    /* Wait until INITF flag is set */
    while ((RTC_ISR1 & (1 << 6)) && ( initfcount != 0xFFFF))
    {
      initfcount++;
    }
  }

	//Set Second
	RTC_TR1 = (uint8_t)SetSecond;
	//Set Minute 
	RTC_TR2 = (uint8_t)SetMinute;
	//Set Hour
	RTC_TR3 = (uint8_t)SetHour;

	//Dummy reading DR3 reg to unfreeze the calendar
	(void)(RTC_DR3);

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
void liteRTC_SetHMS(uint8_t SetHour, uint8_t SetMinute, uint8_t SetSecond){// Set the Hour (24Hrs format), Minute, Second
	liteRTC_SetHMSBCD(ByteToBcd2(SetHour), ByteToBcd2(SetMinute), ByteToBcd2(SetSecond));
}

/* Parameter descriptions
 * *rH : pointer to the variable storing Hour
 * *rM : pointer to the variable storing Minute
 * *rS : pointer to the variable storing Second
 */
void liteRTC_grepTime(uint8_t *rH, uint8_t *rM, uint8_t * rS){
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
void liteRTC_grepDate(uint8_t *rDay, uint8_t *rDate, uint8_t *rMo, uint8_t *rYe){
	//dummy reading the TR1 reg to unfreze the calendar
	(void)(RTC_TR1);
	
	*rDate = Bcd2ToByte(RTC_DR1);
	*rMo = Bcd2ToByte(RTC_DR2);
	*rYe = Bcd2ToByte(RTC_DR3);
	
	*rDay = *rMo << 5;// Grep Day of week number
	*rMo = *rMo & 0x1F;// Clean out Day of week number from Month variable.
}


