/*

    Power control.

    THEORY OF OPERATION
    ===================

    We put the MCU into "powerdown" mode to save batteries when the unit is not in use.


*/


#include "blinks.h"
#include "hardware.h"

#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "power.h"
#include "utils.h"

// Don't do anything, just the interrupt itself will wake us up
// TODO: If we ever need more code space, this could be replaced by an IRET in the VECTOR table.

EMPTY_INTERRUPT( WDT_vect );
      
// Goto low power sleep - get woken up by button or IR LED 
// Be sure to turn off all pixels before sleeping since
// the PWM timers will not be running so pixels will not look right.

// TODO: We should probably ALWAYS sleep with timers on between frames to save CPU power. 

void sleep(void) {
    
    set_sleep_mode( SLEEP_MODE_PWR_DOWN );
    sleep_enable();
    sleep_cpu();        // Good night
}

// Sleep with a predefined timeout. 
// This is very power efficient since chip is stopped except for WDT
// Note that if the timer was on entering here that it will stay on, so LED will still stay lit.

void sleepWithTimeout( sleepTimeoutType timeout ) {
    
    wdt_reset();
    WDTCSR =   timeout;              // Enable WDT Interrupt  (WDIE and timeout bits all included in the howlong values)
    
    sleep();
    
    WDTCSR = 0;                      // Turn off the WDT interrupt (no special sequence needed here)
                                     // (assigning all bits to zero is 1 instruction and we don't care about the other bits getting clobbered
    
}
