/*

    Control the 6 RGB LEDs visible on the face of the tile


    THEORY OF OPERATION
    ===================

	Button is simple. Push it, and it calls an ISR - even when in deep sleep.

*/


#include "hardware.h"
#include "callbacks.h"

#include <avr/interrupt.h>

#include "button.h"
#include "bitfun.h"

// Callback that is called when the button state changes.
// Note that you could get multiple consecutive calls with the
// Same state if the button quickly toggles back and forth quickly enough that
// we miss one phase. This is particularly true if there is a keybounce exactly when
// and ISR is running.

// Use BUTTON_DOWN() to check buttonstate when called.

// Confirmed that all the pre/postamble pushes and pops compile away if this is left blank

// Weak reference so it (almost) compiles away if not used.
// (looks like GCC is not yet smart enough to see an empty C++ virtual invoke. Maybe some day!)
// REMOVED WEAK REFERENCE! THE COMPILER SOMETIMES WOULD LINK THE WEAK RATHER THAN THE STRONG! WTF GCC?!?!
// So for now, you must supply empty stubs. :/


struct ISR_CALLBACK_BUTTON : CALLBACK_BASE<ISR_CALLBACK_BUTTON> {

    static const uint8_t running_bit = CALLBACK_BUTTON_RUNNING_BIT;
    static const uint8_t pending_bit = CALLBACK_BUTTON_PENDING_BIT;

    static inline void callback(void) {

        button_callback_onChange();

    }

};


void button_init(void) {
	// Pin change interrupt setup
	SBI( PCICR , BUTTON_PCI );          // Enable the pin group
}

// Enable pullup and interrupts on button

void button_enable(void) {

	// GPIO setup
	SBI( BUTTON_PORT , BUTTON_BIT);     // Leave in input mode, enable pull-up

}


// Disable pull-up
// You'd want to do this to save power in the case where the
// button is stuck down and therefore shorting out the pull-up

void button_disable(void) {


    CBI( BUTTON_PORT , BUTTON_BIT);     // Leave in input mode, disable pull-up

}


// Returns 1 if button is currently down

uint8_t button_down(void) {

	return BUTTON_DOWN();

}



ISR(BUTTON_ISR)
{
    ISR_CALLBACK_BUTTON::invokeCallback();
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
