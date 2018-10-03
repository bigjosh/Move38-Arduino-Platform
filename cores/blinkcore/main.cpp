/*
 * main.cpp
 *
 * This gets called first by the C bootstrap code.
 * It initializes the hardware and then called run()
 */

#include "hardware.h"
#include "shared.h"

#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "utils.h"
#include "ir.h"
#include "pixel.h"
#include "timer.h"
#include "button.h"
#include "adc.h"
#include "power.h"

#include "run.h"				// Prototype for the run function we will hand off to

// Change clock prescaler to run at 8Mhz.
// By default the CLKDIV fuse boots us at 8Mhz osc /8 so 1Mhz clock
// Change the prescaler to get some more speed but still run at low battery voltage
// We could go to 8MHz, but then we would not be able to run the battery down lower than 2.4V...
// https://electronics.stackexchange.com/questions/336718/what-is-the-minimum-voltage-required-to-operate-an-avr-mcu-at-8mhz-clock-speed/336719

/*

    To avoid unintentional changes of clock frequency, a special write procedure must be followed to change the
    CLKPS bits:
    1. Write the Clock Prescaler Change Enable (CLKPCE) bit to one and all other bits in CLKPR to zero.
    2. Within four cycles, write the desired value to CLKPS while writing a zero to CLKPCE.

*/

static void mhz_init(void) {
    CLKPR = _BV( CLKPCE );                  // Enable changes
    CLKPR =				0;                  // DIV 1 (8Mhz clock with 8Mhz RC osc)

    #if (F_CPU != 8000000 )
        #error F_CPU must match the clock prescaller bits set in mhz_init()
    #endif
}


static void init(void) {

    mhz_init();				// switch to 4Mhz. TODO: Some day it would be nice to go back to 1Mhz for FCC, but lets just get things working now.

    power_init();
    button_init();

    adc_init();			    // Init ADC to start measuring battery voltage
    pixel_init();
    ir_init();

    ir_enable();

    pixel_enable();

    button_enable_pu();

    sei();					// Let interrupts happen. For now, this is the timer overflow that updates to next pixel.

}


// Initialize the hardware and pass the flag to run()
// Weak so that a user program can take over immediately on startup and do other stuff.

int __attribute__ ((weak)) main(void)
{

	init();

    while (1) {
	    run();
        // TODO: Sleep here and only wake on new event
    }

	return 0;
}
