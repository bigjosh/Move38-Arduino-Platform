/*
 * button.h
 *
 */ 

#ifndef BUTTON_H_
#define BUTTON_H_

#include <avr/io.h>

// Debounce button pressed this much
#define BUTTON_DEBOUNCE_MS 40

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




#endif /* BUTTON_H_ */