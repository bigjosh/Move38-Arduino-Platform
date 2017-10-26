/*
 * Timer related functions
 * 
 */ 

// We will use 8 bit Timer2 for the following...
// 1. Timekeeping. 
// 2. Button reading. We periodically check the button position and debounce state and decode clicks
// 3. IR reading. We periodically check the IR LED states and decode rx data. 

// These are all handled by callbacks from here.

// Note that we do not handle the pixel timing functions here since the pixels use hardware PWM on a dedicated timer,
// so best to have that timer generate the overflows for pixel control. 

// We want to run this timer at the overflow period the pixel timer, but we will run them out of phase
// so as long as both ISRs take less than 1/2 of the period, then they should never interrupt or delay each other.
// TODO: Confirm that timers are out of phase, and then niether uses up longer than 1/2 of its slot

// We overlfow (and interrupt) every 512us

#include <assert.h>         // For static_assert

#include "hardware.h"
#include "blinkcore.h"

#include <util/delay.h>

#include "debug.h"

#include "power.h"

#include "pixel.h"          // We depend on the pixel ISR to call us for timekeeping

#include "callbacks.h"

#include "timer.h"


/** Timing ***/

// Delay `millis` milliseconds. 
// TODO: sleep to save power?
// TODO: User timer?

// Of course this will run slightly long, but better to go over than under
// and no reasonable expeaction that the delay will be *exact* since there are
// function call overheads and interrupts to consider. 

// TODO: Build this on top of our own mills command
// or use a timer callback


void delay_ms( unsigned long millis) { 
    
    while (millis >= 1000 ) {
        _delay_ms( 1000 );        
        millis -= 1000;
    }        
    
    while (millis >= 100 ) {
        _delay_ms( 100 );
        millis -= 100;
        
    }

    while (millis >= 10 ) {
        _delay_ms( 10 );
        millis -= 10;
    }
    
    while (millis >= 1 ) {
        _delay_ms( 1 );
        millis -= 1;
}
    
  
}    

