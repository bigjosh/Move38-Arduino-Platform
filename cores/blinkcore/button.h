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
// TODO: We need a way to disable pull-ups in case the button is stuck down in a pocket or drawer.

void button_enable(void);

// Returns 1 if button is currently down

uint8_t button_down(void);

// Disable pull-up
// You'd want to do this to save power in the case where the
// button is stuck down and therefore shorting out the pull-up

void button_disable(void);

// User supplied callback that is called when the button state changes.
// Note that you could get multiple consecutive calls with the
// Same state if the button quickly toggles back and forth quickly enough that
// we miss one phase. This is particularly true if there is a keybounce exactly when
// an ISR is running.

// I know that you think you want to know if the button was up or down passed to the callback
// but that is false security. You never know how long the button will be down for, and you don't
// know the interrupt latency between the button change that triggers the interrupt and when
// you are going to get to sample it. Heck, even if there was 0 interrupt latency, the button could
// still contact long enough to trigger the interrupt but not to show up on the pin read.
// Much more rigorous to think in terms of state changes than absolute values.


void button_callback_onChange(void);


// Enable callback to button_callback_onChange on button change interrupt
// Typically used to wake from sleep, but could also be used for low latency
// button decoding - but remeber that there is not 1:1 mapping of changes on the
// button pin to calls to the ISR, so timer-based button decoding likely easier
// and more efficient.

// Call after button_enable()

void button_ISR_on(void);

// Call before button_disable()

void button_ISR_off(void);


#endif /* BUTTON_H_ */
