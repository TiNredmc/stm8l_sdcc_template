/* Simple LED blinking on PB4 
 * Coded by TinLethax
 */
#include <stdint.h>
#include <stm8l.h>
// Always define before include delay.h
#define CPU4M	//Use clock divider to get 1MHz and define this to use 1MHz delay
#include <delay.h>

#define LED_PIN     4 //PB4 
 
void main() {
	CLK_CKDIVR = 0x02;// Full 16Mhz, no clock divider
	PB_DDR |= (1 << LED_PIN);//Direction register set 1 to the 3rd bit.
	PB_CR1 |= (1 << LED_PIN);//Set Control register for PB4 as output.
    while (1) {
        PB_ODR ^= (1 << LED_PIN);//toggle the pin
        delay_ms(1000);
    }
}
