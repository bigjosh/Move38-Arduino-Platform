/*

    This library provides the operating system for the Move38 Blinks platform.
    More info at http://Move38.com

    This sits on top of the hardware abstraction layer and handles functions like

       *Startup and loading new games
       *Sleeping
       *Time keeping

    ...basically all things that a game can not do.

*/

#include <avr/pgmspace.h>
#include <stddef.h>     // NULL

#include "blinkos.h"
#include "blinkos_button.h"
#include "blinkos_timer.h"          // Class Timer 

#include "callbacks.h"              // From blinkcore, which will call into us via these

// TODO: Put this at a known fixed address to save registers
// Will require a new .section in the linker file. Argh. 

loopstate_in_t loopstate_in;
loopstate_out_t loopstate_out;

#include "pixel.h"
#include "timer.h"
#include "utils.h"
#include "power.h"
#include "button.h"

#include "ir.h"
#include "blinkos_irdata.h"


#define SLEEP_TIMEOUT_SECONDS (10*60)          // If no button press in this long then goto sleep

#define SLEEP_TIMEOUT_MS ( (unsigned long) SLEEP_TIMEOUT_SECONDS  * MILLIS_PER_SECOND) // Must cast up becuase otherwise will overflow

// When we should fall sleep from inactivity
Timer sleepTimer;



// Turn off everything and goto to sleep

static void sleep(void) {

    pixel_disable();        // Turn off pixels so battery drain
    ir_disable();           // TODO: Wake on pixel
    button_ISR_on();        // Enable the button interrupt so it can wake us

    power_sleep();          // Go into low power sleep. Only a button change interrupt can wake us

    button_ISR_off();       // Set everything back to thew way it was before we slept
    ir_enable();
    pixel_enable();

    loopstate_in.woke_flag= 1;

}


void postponeSleep() {
    sleepTimer.set( SLEEP_TIMEOUT_MS );    
}    

void timer_1000us_callback_sei(void) {

    incrementMillis1ms();
    
    if (updateButtonState1ms( loopstate_in.buttonstate )) {        
        postponeSleep();
    }        

}


// Below are the callbacks we provide to blinkcore

// This is called by timer ISR about every 512us with interrupts on.

void timer_256us_callback_sei(void) {

    // Decrementing slightly more efficient because we save a compare.
    static unsigned step_us=0;

    step_us += 256;                     // 256us between calls

    if ( step_us>= 1000 ) {             // 1000us in a 1ms
        timer_1000us_callback_sei();
        step_us-=1000;
    }
}

// This is called by timer ISR about every 256us with interrupts on.

void timer_128us_callback_sei(void) {
    IrDataPeriodicUpdateComs();
}

// This is the entry point where the blinkcore platform will pass control to
// us after initial power-up is complete

// We make this weak so that a game can override and take over before we initialize all the hier level stuff

void run(void) {
    
    ir_enable();

    pixel_enable();

    button_enable_pu();
    
    // Call user setup code
           
    setupEntry();

    while (1) {

        updateMillisSnapshot();             // Use the snapshot do we don't have to turn off interrupts
                                            // every time we want to check this multibyte value


        loopEntry( &loopstate_in , &loopstate_out );

        if (sleepTimer.isExpired()) {
            sleep();

        }

        pixel_displayBufferedPixels();      // show all display updates that happened in last loop()
                                            // Also currently blocks until new frame actually starts


        // TODO: Possible
    }

}
