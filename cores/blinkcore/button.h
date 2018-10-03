/*
 * button.h
 *
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include <avr/io.h>

// Call once at power up.

void button_init(void);

// Enable pullup on button

void button_enable_pu(void);

// Returns 1 if button is currently down

uint8_t button_down(void);

// Disable pull-up
// You'd want to do this to save power in the case where the
// button is stuck down and therefore shorting out the pull-up

void button_disable_pu(void);

// Enable interrupt on button change interrupt
// Typically used to wake from sleep.

// Call after button_enable_pu()

void button_ISR_on(void);

// Call before button_disable()

void button_ISR_off(void);


#endif /* BUTTON_H_ */