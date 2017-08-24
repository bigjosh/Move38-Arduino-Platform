/*

    Control the 6 RGB LEDs visible on the face of the tile


    THEORY OF OPERATION
    ===================
    
	Button is simple. Push it, and it calls an ISR - even when in deep sleep. 

*/


#include "blinks.h"
#include "hardware.h"

#include <avr/interrupt.h>
#include <util/delay.h>					// _delay_ms() for debounce

#include "button.h"
#include "utils.h"

// TODO: Do proper debounce when we have a timer available.

static volatile uint8_t button_flag;


ISR(BUTTON_ISR)
{
	if (BUTTON_DOWN()) {
		button_flag = 1;
	}
}


void button_init(void) {
	
	// GPIO setup
	SBI( BUTTON_PORT , BUTTON_BIT);     // Leave in input mode, enable pull-up
	
	// Pin change interrupt setup
	SBI( BUTTON_MASK , BUTTON_PCINT);   // Enable pin in Pin Change Mask Register
	SBI( PCICR , BUTTON_PCI );          // Enable the pin group
	
}

// Returns 1 if button pressed since the last time this was called

uint8_t buttonPressed(void) {
	
	// TODO: Use a proper timer to debounce here? Does it really matter for this?

	_delay_ms( BUTTON_DEBOUNCE_MS );
	
	if (button_flag) {
		
		button_flag=0;
		return 1;
	} 
	
	return 0;
	
}

// Returns 1 if button is currently down

uint8_t buttonDown(void) {
	
	return BUTTON_DOWN();
	
}
