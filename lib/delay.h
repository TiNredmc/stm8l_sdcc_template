#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

#ifndef F_CPU
#warning "F_CPU not defined, using 2MHz by default"
#define F_CPU 2000000UL
#endif

#define T_COUNT(x) (( x * (F_CPU / 1000000UL) )-5)/5)

inline void _delay_cycl( unsigned short __ticks )
{

    __asm__("nop\n nop\n"); 
    do {    
      __ticks--;
    } while ( __ticks );
    __asm__("nop\n");
}
inline void delay_us( unsigned short __us )
{
    _delay_cycl((unsigned short)( T_COUNT(__us));
}

inline void delay_ms( unsigned short __ms )
{

    while ( __ms-- )
    {
        delay_us( 400 );
    }
}

#endif /* DELAY_H */
