/*
 * power.h
 *
 * All the functions for controlling power and sleep.
 *
 */ 

#ifndef POWER_H_
#define POWER_H_

#include <avr/io.h>
#include <stdbool.h>

// Sorry, I can't figure out a better way to get Arduino IDE to compile this type...

#ifndef SLEEP_TIMEOUT_
#define SLEEP_TIMEOUT_

// Possible sleep timeout values

typedef enum {

    TIMEOUT_16MS = (_BV(WDIE) ),
    TIMEOUT_32MS = (_BV(WDIE) | _BV( WDP0) ),
    TIMEOUT_64MS = (_BV(WDIE) | _BV( WDP1) ),
    TIMEOUT_125MS= (_BV(WDIE) | _BV( WDP1) | _BV( WDP0) ),
    TIMEOUT_250MS= (_BV(WDIE) | _BV( WDP2) ),
    TIMEOUT_500MS= (_BV(WDIE) | _BV( WDP2) | _BV( WDP0) ),
    TIMEOUT_1S   = (_BV(WDIE) | _BV( WDP2) | _BV( WDP1) ),
    TIMEOUT_2S   = (_BV(WDIE) | _BV( WDP2) | _BV( WDP1) | _BV( WDP0) ),
    TIMEOUT_4S   = (_BV(WDIE) | _BV( WDP3) ),
    TIMEOUT_8S   = (_BV(WDIE) | _BV( WDP3) | _BV( WDP0) )
        
} sleepTimeoutType;    


#endif

// Goto low power sleep - get woken up by any active interrupt source
// which could include button change or IR change

void powerdown(void);
    
// Sleep with a predefined timeout.
// This is very power efficient since chip is stopped except for WDT

void powerdownWithTimeout( sleepTimeoutType timeout );


// Get everything set up for proper sleeping
void sleep_init(void);
    
#endif /* POWER_H_ */