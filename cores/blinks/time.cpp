/*
 * Time related functions
 * 
 */ 


#include "hardware.h"

#include <util/delay.h>

#include "debug.h"

#include "power.h"

#include "pixel.h"          // We depennd on the pixel ISR to call use for timekeeping

#include "time.h"

/** Timing ***/

// Milliseconds elapsed since last call to pixel_enable()
// Resolution is limited by the pixel clock, which only updates about
// once per 2.5ms with 4Mhz system clock and /8 prescaller.
// Resets back to 0 on pixel_disable()
// Assumes interrupts are enabled when called.

uint32_t pixel_mills(void);

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

