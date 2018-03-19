/*

    Power control.

    THEORY OF OPERATION
    ===================

    We put the MCU into "powerdown" mode to save batteries when the unit is not in use.

    The sleep with timeout works by enabling the watchdog timer to generate an interrupt after the specified delay.


*/


#include "hardware.h"

#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "utils.h"
#include "power.h"

// Don't do anything, just the interrupt itself will wake us up
// TODO: If we ever need more code space, this could be replaced by an IRET in the VECTOR table.

static volatile uint8_t wdt_flag=0;            // The the WDT get to fire?

ISR( WDT_vect ) {

    wdt_flag=1;         // Remember if we fired

}

// Goto low power sleep - get woken up by button or IR LED
// Be sure to turn off all pixels before sleeping since
// the PWM timers will not be running so pixels will not look right.
// If wakeOnButton is true, then we will wake if the button changes (up or down)
// Each bit in wakeOnIR_bitmask represents the IR sensor on one face. If the bit is 1
// then we will wake when there is a change on that face

// TODO: We should probably ALWAYS sleep with timers on between frames to save CPU power.

void power_sleep(void) {

    sleep_cpu();        // Good night

}

// Sleep with a predefined timeout.
// This is very power efficient since chip is stopped except for WDT
// If wakeOnButton is true, then we will wake if the button changes (up or down)
// Each bit in wakeOnIR_bitmask represents the IR sensor on one face. If the bit is 1
// then we will wake when there is a change on that face

bool power_sleepWithTimeout( power_sleepTimeoutType timeout ) {

    wdt_flag=0;     // Reset timeout flag
                    // Don't worry about a race since WDT is off now



    cli();
    wdt_reset();
    WDTCSR |= _BV(WDCE) | _BV(WDE);  // Does not actually set WDE, just enables changes to the prescaler in next instruction.  NOT DOCUMENTED THAT YOU NEED THE WDE SET ON THIS WRITE
    WDTCSR =   timeout;              // Enable WDT Interrupt  (WDIE and timeout bits all included in the timeout values)
    sei();

    sleep_cpu();                     // Good night - compiles to 1 instruction

    // Don't worry about edge where we get interrupted here and another WDT fires - it will benignly just set the already set flag.

    WDTCSR = 0;                      // Turn off the WDT interrupt (no special sequence needed here)
                                     // (assigning all bits to zero is 1 instruction and we don't care about the other bits getting clobbered

    return wdt_flag;

}

void power_init(void) {

    wdt_reset();        // reset the countdown!
    MCUSR &= ~_BV(WDRF);// You can not turn off the WDT unless this bit is cleared! (NOTE THIS IS MISNAMED IN THE DATASHEET AS WDRT!)
    wdt_disable();      // In case we just rebooted becuase of a power_reset(), this makes sure we will not timeout and
                        // and reboot again. This is necessary because on WDT reset the WDT flag is set, which automatically re-enables
                        // the watchdog timer.

    // Could save a byte here by combining these two to a single assign

    // I know data sheet repeatedly states you should only set Sleep Enable just before sleeping to avoid
    // "accidentally" sleeping, but the only way to sleep is to execute the "sleep" instruction
    // so if you are somehow executing that instruction at a time when you don't mean to sleep, then you
    // have bigger problems to worry about.

    // SIZE: These could be combined to save a few bytes
    set_sleep_mode( SLEEP_MODE_PWR_DOWN );
    sleep_enable();


}

// Execute a soft reset - almost like power up
// Based on http://www.atmel.com/webdoc/avrlibcreferencemanual/FAQ_1faq_softreset.html
// Assumes that WDRF is cleared in the startup code, which apparently it is.

void power_soft_reset(void) {
    cli();
    wdt_enable(WDTO_1S);      // This *must* be long enough that after the reset power_init() will be called before we reset again.
    while (1);
}
