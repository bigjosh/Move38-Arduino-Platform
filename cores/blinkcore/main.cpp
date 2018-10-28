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
#include "timers.h"
#include "button.h"
#include "adc.h"
#include "power.h"

#include "callbacks.h"          // External callback to next higher software layer (here we use `run()`)

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

    mhz_init();				// switch to 8Mhz. TODO: Some day it would be nice to go back to 1Mhz for FCC, but lets just get things working now.

    sei();					// Let interrupts happen. For now, this is the timer overflow that updates to next pixel.

}


// Initialize the hardware and pass the flag to run()
// Weak so that a user program can take over immediately on startup and do other stuff.

/* Function Prototypes
 * The main() function is in init9, which removes the interrupt vector table
 * we don't need. It is also 'OS_main', which means the compiler does not
 * generate any entry or exit code itself (but unlike 'naked', it doesn't
 * suppress some compile-time options we want.)
 * https://github.com/Optiboot/optiboot/blob/master/optiboot/bootloaders/optiboot/optiboot.c
 */
/*
void pre_main(void) __attribute__ ((naked)) __attribute__ ((section (".text0"))) __attribute__((used));
int main(void) __attribute__ ((OS_main)) __attribute__ ((section (".init9"))) __attribute__((used));

*/
/* everything that needs to run VERY early */

/*
void pre_main(void) {
    // Allow convenient way of calling do_spm function - jump table,
    //   so entry to this function will always be here, independent of compilation,
    //   features etc
    asm volatile (
        "	rjmp	main\n"
    );
}
*/


void mainx(void) __attribute__ ((section (".init9"))) __attribute__((used)) __attribute__ ((naked));

void mainx(void)
{
	init();

    run();
        // TODO: Sleep here and only wake on new event

}
