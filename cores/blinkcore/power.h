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
// I know that it is idiomatic Arduino to use straight defines here
// but that leaves open questions. This makes is clear that you pass
// a single timeout value from the list of permitted values.

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

} power_sleepTimeoutType;


#endif

// Goto low power sleep - get woken up by any active interrupt source
// which could include button change or IR change

void power_sleep(void);

// Sleep with a predefined timeout.
// This is very power efficient since chip is stopped except for WDT

// Returns 1 if the timeout expired.

bool power_sleepWithTimeout( power_sleepTimeoutType timeout );


// Get everything set up for proper sleeping
void power_init(void);

// Execute a soft reset - almost like power up

void power_soft_reset(void);

#endif /* POWER_H_ */