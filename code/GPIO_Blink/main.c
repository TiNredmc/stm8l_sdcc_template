#include <stdint.h>
#include <stm8l.h>
#include <delay.h>

#define LED_PIN     4 //PB4 

void main() {
	  PB_DDR |= (1 << LED_PIN);//Direct register set 1 to the 3rd bit.
    PB_CR1 |= (1 << LED_PIN);//Set Control register for PB4 as output.
    while (1) {
        PB_ODR ^= (1 << LED_PIN);//toggle the pin
        delay_ms(1000);
    }
}
