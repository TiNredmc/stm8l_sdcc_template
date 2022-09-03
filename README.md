# stm8l_sdcc_template
the template and libraries for stm8l + SDCC

# Big Note 
This repo contain the source code from https://github.com/lujji.  
I modified to make it support with STM8L151F3.

What's working
=

GPIO	PushPull, Open drain. Input pull up and interrupt.  
ADC		(currently paired with DMA).  
DMA		(currently working with ADC, other peripheral coming soon).  
I2C		Slave / Master with iterrupt, 400KHz 100KHz (never try 1MHz).   
UART	TX RX with interrupt(IrDA is W.I.P.).    
SPI		Master tested, really easy to use. Slave code might be soon.  
EEPROM 	Read/Write.  
FLASH	Read/Write, Lock/Unlock  (Once locked, require resetting the chip 2 times (press reset button) in order to unlock it again).  
TIMER 	TIM2 TIM4 PWM example is available. Now working on quadrature encoder (with CDM102 project).  
Delay 	Not so accurate but works just fine. Available for 1MHz 2MHz 4MHz 8MHz and 16MHz Clock speed.
DelayTIM4 More accurate delay by using timer4.  
Device ID	Return X Y wafer coordinate (2 bytes) and Wafer number (1 byte).  
RTC		Time, Date setting, Periodic interrupt waking up. Alarm and tamper detection not tested.
Beeper  Working perfectly. Thought it shares pin with SWIM. Be careful.
