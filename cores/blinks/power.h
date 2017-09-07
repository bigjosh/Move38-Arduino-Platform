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


#endif

// Goto low power sleep - get woken up by button or IR LED
// Be sure to turn off all pixels before sleeping since
// the PWM timers will not be running so pixels will not look right.
// If wakeOnButton is true, then we will wake if the button changes (up or down)
// Each bit in wakeOnIR_bitmask represents the IR sensor on one face. If the bit is 1
// then we will wake when there is a change on that face

// TODO: We should probably ALWAYS sleep with timers on between frames to save CPU power.

void powerdown(bool wakeOnbutton,  uint8_t wakeOnIR_bitmask  );
    
// Sleep with a predefined timeout.
// This is very power efficient since chip is stopped except for WDT
// If wakeOnButton is true, then we will wake if the button changes (up or down)
// Each bit in wakeOnIR_bitmask represents the IR sensor on one face. If the bit is 1
// then we will wake when there is a change on that face

void powerdownWithTimeout( bool wakeOnbutton, uint8_t wakeOnIR_bitmask , sleepTimeoutType timeout );


// Get everything set up for proper sleeping
void sleep_init(void);
    
#endif /* POWER_H_ */