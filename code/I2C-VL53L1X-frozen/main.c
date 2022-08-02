/* Example code for VL53L1X. Using Ultralight API provided by ST
 * API ported and coded by TinLethax 2020/05/29
 */ 

#include <stm8l.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <delay.h>
#include "VL53L1X_api.h"
#include <usart.h>
#include <i2c.h>

uint16_t REMAP_Pin = 0x011C; 

int putchar(int c){
	usart_write(c);
	return 0;
}

uint8_t dev = 0x52; // i2c address
//int status=0;

void main(){
//	CLK_CKDIVR = 0x00;// get the full 16 MHz 
usart_init(9600); // usart using baudrate at 9600
SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); //remap the non-exit pin of Tx and Rx of the UFQFPN20 package to the exit one.
SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);
  uint8_t sensorState=0;
  uint16_t Distance = 0;
  //uint16_t SignalRate;
  //uint16_t AmbientRate;
  //uint16_t SpadNum; 
  //uint8_t RangeStatus = 0;
  //uint8_t dataReady = 0;
	//printf("  \n");
	i2c_init(dev, 0x0050);// i2c init at 100kHz
	delay_ms(100);

/* Those basic I2C read functions can be used to check your own I2C functions */
//  while(sensorState==0){
		/*status =*/ //VL53L1X_BootState(dev, &sensorState);
//	delay_ms(2);
//  }
  //printf("Chip booted\n");

  /* This function must to be called to initialize the sensor with the default setting  */
  /*status = */VL53L1X_SensorInit(dev);


  //printf("VL53L1X Ultra Lite Driver Example running ...\n");
  /*status =*/ VL53L1X_StartRanging(dev);   /* This function has to be called to enable the ranging */
	while(1){
	  //while (dataReady == 0){
		  /*status =*/ //VL53L1X_CheckForDataReady(dev, &dataReady);
	//	  delay_ms(2);
	  //}
	  //dataReady = 0;
	  /*status =*/ //VL53L1X_GetRangeStatus(dev, &RangeStatus);
	  /*status =*/ VL53L1X_GetDistance(dev, &Distance);
	  //status = VL53L1X_GetSignalRate(dev, &SignalRate);
	  //status = VL53L1X_GetAmbientRate(dev, &AmbientRate);
	  //status = VL53L1X_GetSpadNb(dev, &SpadNum);
	  /*status =*/ VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
	  //printf("%u, %u, %u, %u, %u\n", RangeStatus, Distance, SignalRate, AmbientRate,SpadNum);
	  printf("%u\n",Distance);
	}// While loop
  
}// main 
