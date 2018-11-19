/*
 * main.cpp
 *
 * This gets called first by the C bootstrap code.
 * It initializes the hardware and then called run()
 *
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

// "used" makes sure the link doesn't throw this away since it is not referenced from anywhere else.
// "naked" gets rid of stack frame and return at the end.
// "init9" gets us running right at the very startup, but after bss initialized.


//int main(void) __attribute__ ((section (".init9"))) __attribute__((used)) __attribute__ ((naked));

int main(void)
{

    //adc_init();
    //adc_enable();           // Start ADC so we can check battery

    power_init();           // Enable sleep mode

    // TODO: CHeck battery

    power_8mhz_clock();     // Switch to high gear

    run();

    // TODO: Sleep here and only wake on new event

}
