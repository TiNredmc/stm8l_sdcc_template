/* Delay using timer4. Originally by Shawon Shahryiar
 * slim down for SDCC by TinLethax
 */

#include <delaytim4.h>

void delaytim4_init()
{
	CLK_CKDIVR = 0x00;// set clock divider to 1.
	CLK_PCKENR1 |= (uint8_t)((uint8_t)1 << ((uint8_t)0x02 & (uint8_t)0x0F));// enable the TIM4 clock
}

void delaytim4_us(signed int us)
{
       /*TIM4_DeInit*//*restore to default value*/
  TIM4_CR1   = 0x00;
  TIM4_CR2   = 0x00;
  TIM4_SMCR   = 0x00;
  TIM4_IER   = 0x00;
  TIM4_CNTR   = 0x00;
  TIM4_PSCR  = 0x00;
  TIM4_ARR   = 0xFF;
  TIM4_SR   = 0x00;
       if((us <= 200) && (us >= 0))
       {
		/* TIM4_TimeBaseInit(TIM4_PRESCALER_16, 200)*/
		TIM4_ARR = 200; //set prescaler period to 200 
		TIM4_PSCR = 0x04;// set prescaler to 16
		TIM4_EGR = 0x01;// event source update 
		/*TIM4_cmd(ENABLE)*/
		TIM4_CR1 |= 0x01 ;
       }
       else if((us <= 400) && (us > 200))
       {
              us >>= 1;
		/* TIM4_TimeBaseInit(TIM4_PRESCALER_32, 200)*/
		TIM4_ARR = 200; //set prescaler period to 200 
		TIM4_PSCR = 0x05;// set prescaler to 32
		TIM4_EGR = 0x01;// event source update 
		/*TIM4_cmd(ENABLE)*/
		TIM4_CR1 |= 0x01 ; 
       }
       else if((us <= 800) && (us > 400))
       {
              us >>= 2;
		/* TIM4_TimeBaseInit(TIM4_PRESCALER_64, 200)*/
		TIM4_ARR = 200; //set prescaler period to 200 
		TIM4_PSCR = 0x06;// set prescaler to 32
		TIM4_EGR = 0x01;// event source update 
		/*TIM4_cmd(ENABLE)*/
		TIM4_CR1 |= 0x01 ; 
       }
       else if((us <= 1600) && (us > 800))
       {
              us >>= 3;
		/* TIM4_TimeBaseInit(TIM4_PRESCALER_32, 200)*/
		TIM4_ARR = 200; //set prescaler period to 200 
		TIM4_PSCR = 0x07;// set prescaler to 32
		TIM4_EGR = 0x01;// event source update 
		/*TIM4_cmd(ENABLE)*/
		TIM4_CR1 |= 0x01 ; 
       }
       while(TIM4_CNTR < us);
	/*TIM4_ClearFlag(TIM4_FLAG_UPDATE)*/
	TIM4_SR = (uint8_t)(~(0x01));
	/*TIM4_Cmd(DISABLE)*/
	TIM4_CR1 &= (uint8_t)(~(0x01));    
}

void delaytim4_ms(signed int ms)
{
       while(ms--)
       {
              delaytim4_us(1000);
       };
}