/*
 * button.h
 *
 */ 

#ifndef BUTTON_H_
#define BUTTON_H_

#include <avr/io.h>


// Setup pins, interrupts. Call once at power up.

void button_init(void);


//TODO: This will be replaced with proper button functions. 
// Returns 1 if button pressed since the last time this was called

uint8_t buttonPressed(void);

#endif /* BUTTON_H_ */