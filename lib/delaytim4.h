/* Delay using timer4. Originally by Shawon Shahryiar
 * slim down for SDCC by TinLethax
 */

#ifndef DELAYTIM4_H
#define DELAYTIM4_H

#include <stdint.h>
#include <stm8l.h>

void delaytim4_init();// Init the clock stuff in order to use timer4
void delaytim4_us(signed int us);// delay in MicroSeconds
void delaytim4_ms(signed int ms);// delay in MilliSeconds

#endif