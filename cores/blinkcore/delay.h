/*
 * timer.h
 *
 * Timer related functions
 *
 * Note that the RGB pixel PWM uses timer0 and timer2, so we piggy back on timer0 overflow to
 * provide the timer callback. That means all this code lives in pixel.h
 *
 */ 

#ifndef DELAY_H_
#define DELAY_H_

#include "blinkcore.h"

#include "timer.h"

/* 

THIS DOES NOT WORK!...

// C++ implementations should define these macros only when __STDC_LIMIT_MACROS is defined before <stdint.h> is included
// http://www.nongnu.org/avr-libc/user-manual/group__avr__stdint.html

#define __STDC_LIMIT_MACROS

#include <stdint.h>


...so instead we define BLINKCORE_UINT16_MAX in blinkcore.h. Arhg. 

*/

#define DELAY_PRESCALER 1024

// Maximum number of ms you can specify to delay_ms() - currently about 25 seconds


#define cycles_per_tick  DELAY_PRESCALER
#define ticks_per_ms   ( CYCLES_PER_MS / cycles_per_tick )
#define max_ticks BLINKCORE_UINT16_MAX


const unsigned max_delay_ms = (unsigned) ( ( max_ticks * ( (unsigned long) CYCLES_PER_MS ) ) / cycles_per_tick ); 


// Dimensional Analysis:
// c=cycles 
// t=ticks
// 
// ( ( t ) * ( c/ms ) ) / ( c/t )
// ( ( t ) * ( c/ms ) ) * ( t/c )
// ( ( t ) * ( c/ms ) ) * ( t/c )


 
// Delay `ms` milliseconds using a very accurate one-shot on Timer1. 
// Unlike _delay_ms() in avr utils/delay.h, this is invariant to interrupts streching the delay since
// it is hardware timer based. 

void delay_ms( unsigned ms);    
    
#endif /* DELAY_H_ */