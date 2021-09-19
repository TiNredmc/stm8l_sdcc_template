#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

// This delay will work with lease accurately when CPU freqency is 1MHz, NOT recommended using this for any slower cpu clock.

#ifndef F_CPU
#warning "F_CPU not defined, using 16MHz by default"
#define F_CPU 16000000UL
#endif

//#define T_COUNT(x) (( x * (F_CPU / 1000000UL) )-5)/5)

#ifdef CPU1M //cycles calculation for 1MHz CPU
#warning "Using delay of 1MHz CPU"
	#define T_COUNT(X) (X / 8) // minimum delay is 8us, only multiple of 8 is allowed (calculate them here)
	#define DelayMSCONST 800 //(1000us - ~8us of while loop)

#elif defined CPU2M //cycles calculation for 2MHz CPU
#warning "Using delay of 2MHz CPU"
	#define T_COUNT(X) (X / 4) // minimum delay is 4us, only multiple of 4 is allowed (calculate them here)
	#define DelayMSCONST 830 //(1000us - ~4us of while loop)

#elif defined CPU4M //cycles calculation for 4MHz CPU
#warning "Using delay of 4MHz CPU"
	#define T_COUNT(X) (X / 2) // minimum delay is 2us, only multiple of 2 is allowed (calculate them here)
	#define DelayMSCONST 950 //(1000us - ~2us of while loop)

#elif defined CPU8M //cycles calculation for 8MHz CPU
#warning "Using delay of 8MHz CPU"
	#define T_COUNT(X) X
	#define DelayMSCONST 999
	
#else //cycles calculation for 16MHz CPU
#warning "Using delay of 16MHz CPU"
	#define T_COUNT(X) (X * 2)
	#define DelayMSCONST 1000
#endif 

inline void _delay_cycl( uint16_t __ticks )
{
	// at 16MHz, this do-while will thake 500ns per 1 __tick decrement
	// at other F_CPU freq will takes 8/F_CPU ns (I counted each cpy cycle ;D).
    do {    
      __ticks--;
    } while ( __ticks );

}

inline void delay_us( uint16_t __us )
{
    _delay_cycl((uint16_t)T_COUNT(__us));
}

inline void delay_ms( uint16_t __ms )
{
    while ( __ms-- )
    {
        delay_us( DelayMSCONST );
    }
}

#endif /* DELAY_H */
