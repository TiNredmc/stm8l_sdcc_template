#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

#ifndef F_CPU
#warning "F_CPU not defined, using 2MHz by default"
#define F_CPU 2000000UL
#endif
/* for 2MHz cpu */
inline void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ((F_CPU / 18 / 1000UL) * ms); i++) {
        __asm__("nop");
    }
}

/* for 16MHz
inline void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ((F_CPU / 91 / 1000UL) * ms / 2); i++) {
        __asm__("nop");
    }
}
*/

#endif /* DELAY_H */
