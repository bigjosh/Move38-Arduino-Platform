/*

    Control the 6 RGB LEDs visible on the face of the tile


    THEORY OF OPERATION
    ===================

	Button is simple. Push it, and it calls an ISR - even when in deep sleep.

*/


#include "hardware.h"

#include <avr/interrupt.h>

#include "button.h"
#include "bitfun.h"


void button_init(void) {

	// Pin change interrupt setup
	SBI( PCICR , BUTTON_PCI );          // Enable the pin group

    // Pin group is always enabled, but interrupt will not happen unless that pin is enabled via button_isr_on()

}

// Enable button pullup

void button_enable_pu(void) {

	// GPIO setup
	SBI( BUTTON_PORT , BUTTON_BIT);     // Leave in input mode, enable pull-up

}


// Disable pull-up
// You'd want to do this to save power in the case where the
// button is stuck down and therefore shorting out the pull-up
// Be sure to also turn off button ISR or you might get spurious interrupts when the pin floats

void button_disable_pu(void) {

    CBI( BUTTON_PORT , BUTTON_BIT);     // Leave in input mode, disable pull-up

}


// Returns 1 if button is currently down

uint8_t button_down(void) {

	return BUTTON_DOWN();

}



ISR(BUTTON_ISR)
{
    // More to come here
}

// Enable callback to button_callback_onChange on button change interrupt
// Typically used to wake from sleep, but could also be used for low latency
// button decoding - but remember that there is not 1:1 mapping of changes on the
// button pin to calls to the ISR, so timer-based button decoding likely easier
// and more efficient.

void button_ISR_on(void) {
	// Pin change interrupt setup
	SBI( BUTTON_MASK , BUTTON_PCINT);   // Enable pin in Pin Change Mask Register

}

void button_ISR_off(void) {
    // disable pin change interrupt
    CBI( BUTTON_MASK , BUTTON_PCINT);   // Disable pin in Pin Change Mask Register
}
