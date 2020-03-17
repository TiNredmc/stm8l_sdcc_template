/* This is example 1 second LED blinking, delay with precise TIM4*/ 
#include <stdint.h>
#include <stm8l.h>
#include <delaytim4.h>

#define LED_PIN     4 //PB4 

void main() {
    delaytim4_init();// initthe clock for timer4
    PB_DDR |= (1 << LED_PIN);//Direct register set 1 to the 3rd bit.
    PB_CR1 |= (1 << LED_PIN);//Set Control register for PB4 as output.
    while (1) {
        PB_ODR ^= (1 << LED_PIN);//toggle the pin
        delaytim4_ms(1000);
    }
}
