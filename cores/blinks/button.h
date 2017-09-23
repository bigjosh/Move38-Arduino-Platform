/*
 * button.h
 *
 */ 

#ifndef BUTTON_H_
#define BUTTON_H_

#include <avr/io.h>

// Debounce button pressed this much
#define BUTTON_DEBOUNCE_MS 50

// Delay for determining clicks
// So, a click or double click will not be registered until this timeout
// becuase we don't know yet if it is a single, double, or tripple

#define BUTTON_CLICK_TIMEOUT_MS 330

// Setup pins, interrupts. Call once at power up.

void button_init(void);

// Enable pullup and interrupts on button

void button_enable(void);

// Returns 1 if button pressed since the last time this was called
// TODO: Proper debouncing, but at what level?

uint8_t button_pressed(void);


// Returns 1 if button is currently down

uint8_t button_down(void);


// Disable pull-up and interrupts
// You'd want to do this to save power in the case where the
// button is stuck down and therefore shorting out the pull-up

void button_disable(void);


// User supplied callback that is called when the button state changes.
// Note that you could get multiple consecutive calls with the 
// Same state if the button quickly toggles back and forth quickly enough that
// we miss one phase. This is particularly true if there is a keybounce exactly when
// an ISR is running. 

void button_callback_onChange(void) __attribute__((weak));

#endif /* BUTTON_H_ */