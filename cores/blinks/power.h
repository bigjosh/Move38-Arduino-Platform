/*
 * power.h
 *
 * All the functions for controlling power and sleep.
 *
 */ 

#ifndef POWER_H_
#define POWER_H_

#include <avr/io.h>

// Possible sleep timeout values

typedef enum timeout_enum {

    timeout_16MS = (_BV(WDIE) ),
    timeout_32MS = (_BV(WDIE) | _BV( WDP0) ),
    timeout_64MS = (_BV(WDIE) | _BV( WDP1) ),
    timeout_125MS= (_BV(WDIE) | _BV( WDP1) | _BV( WDP0) ),
    timeout_250MS= (_BV(WDIE) | _BV( WDP2) ),
    timeout_500MS= (_BV(WDIE) | _BV( WDP2) | _BV( WDP0) ),
    timeout_1S   = (_BV(WDIE) | _BV( WDP2) | _BV( WDP1) ),
    timeout_2S   = (_BV(WDIE) | _BV( WDP2) | _BV( WDP1) | _BV( WDP0) ),
    timeout_4S   = (_BV(WDIE) | _BV( WDP3) ),
    timeout_8S   = (_BV(WDIE) | _BV( WDP3) | _BV( WDP0) )
        
} sleepTimeoutType;    


// Goto low power sleep - get woken up by button or IR LED
// Be sure to turn off all pixels before sleeping since
// the PWM timers will not be running so pixels will not look right.

// TODO: We should probably ALWAYS sleep with timers on between frames to save CPU power.

void sleep(void);

// Sleep with a predefined timeout. Can still get woken by button or IR before timeout expires.
// This is very power efficient since chip is stopped except for WDT
// Note that if the timer was on entering here that it will stay on, so LED will still stay lit.

void sleepWithTimeout( sleepTimeoutType timeout );


#endif /* POWER_H_ */