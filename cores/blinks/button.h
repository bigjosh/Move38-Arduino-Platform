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


// TODO: This will be replaced with proper button functions. 
// Returns 1 if button pressed since the last time this was called

uint8_t buttonPressed(void);

// TODO: Add enable/disable button functions so we can thoughtfully deal with a button that is
// jammed down without killing the battery. 

#endif /* BUTTON_H_ */