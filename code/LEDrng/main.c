#include <stdint.h>
#include <stm8l.h>
#include <delay.h>

int dataSet[4] = {1,2,4,8} ;// 4bit binary for displaying on the segment (1st block,2nd block,3rd block,4th block)
int scanarray[4] = {0x07,0x0B,0x0D,0x0E};// 0111 1011 1101 1110 (B7 left to B4 right), Scan Array
void initLED()
{
    for (int i = 0; i < 8; i ++){
    PB_DDR |= (1 << i);
    PB_CR1 |= (1 << i);
  }
}

void displayClr(){
	PB_ODR ^= 0xF0 ;// set PB4 to PB7 to 1 (clear the scanning).
	PB_ODR = (0 << 0) | (0 << 1) | (0 << 2) | (0 << 3);// set PB0 to PB3 to 0 (clear the segment)
}

void displayDS(int ds[4])// Dislay dataset
{
      for (int i = 0; i < 4; i ++ ){         
	displayClr();// clear the display
	PB_ODR ^= (scanarray[i] << 4);  // scanning (look at the scanarray comment)
	PB_ODR ^= ds[i]; // mux for dataset
	delay_ms(1);
	}	 
}

void main() 
{
initLED();
delay_ms(100);
displayClr();
while (1)
{
displayDS(dataSet);
}

}
