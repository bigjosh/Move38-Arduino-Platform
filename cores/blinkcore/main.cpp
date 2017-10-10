/*
 * BlinksFirmware.c
 *
 * Created: 3/26/2017 8:39:50 PM
 * Author : josh.com
 */ 

//#include "Arduino.h"
#include "hardware.h"

#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "debug.h"
#include "utils.h"
#include "ir.h"
#include "pixel.h"
#include "timer.h"
#include "button.h"
#include "adc.h"
#include "power.h"
#include "callbacks.h"
 
// Change clock prescaller to run at 4Mhz. 
// By default the CLKDIV fuse boots us at 8Mhz osc /8 so 1Mhz clock
// Change the prescaller to get some more speed but still run at low battery voltage

/*

    To avoid unintentional changes of clock frequency, a special write procedure must be followed to change the
    CLKPS bits:
    1. Write the Clock Prescaler Change Enable (CLKPCE) bit to one and all other bits in CLKPR to zero.
    2. Within four cycles, write the desired value to CLKPS while writing a zero to CLKPCE.

*/

static void mhz_init(void) {
    CLKPR = _BV( CLKPCE );                  // Enable changes
    CLKPR = _BV( CLKPS0 );                  // DIV 2 (4Mhz clock with 8Mhz RC osc)
    
    #if (F_CPU != 4000000 ) 
        #error F_CPU must match the clock prescaller bits set in mhz_init()
    #endif    
}    


// This will put all timers into sync mode, where they will stop dead
// We can then run the enable() fucntions as we please to get them all set up
// and then release them all at the same exact time
// We do this to get timer0/timer1 and timer2 to be exactly out of phase
// with each other so they can run without stepping on each other
// This assumes that one of the timers will start with its coutner 1/2 way finished
//..which timer2 does. 

void holdTimers(void) {
    SBI(GTCCR,TSM);         // Activate sync mode
    SBI(GTCCR,PSRASY);      // Stop timer0 and timer1
    SBI(GTCCR,PSRSYNC);     // Stop timer2
}     


void releaseTimers(void) {
    CBI(GTCCR,TSM);            // Release all timers at the same moment
}    


static void init(void) {

    mhz_init();				// switch to 4Mhz. TODO: Some day it would be nice to go back to 1Mhz for FCC, but lets just get things working now.
    
    DEBUG_INIT();			// Handy debug outputs on unused pins
    
    power_init();
    timer_init();
    button_init();
    
    adc_init();			    // Init ADC to start measuring battery voltage
    pixel_init();    
    ir_init();
    
    ir_enable(); 
        
    holdTimers();        
    pixel_enable();    
    timer_enable();
    releaseTimers();
    
    button_enable();
    
    sei();					// Let interrupts happen. For now, this is the timer overflow that updates to next pixel.

}    



// This empty run() lets us at least compile when no higher API is present.
/*
void __attribute__((weak)) run(void) {    
    pixel_SetAllRGB( 0,  255 ,  0  );
}
*/     
    
int main(void)
{
	init();
	
    while (1) {
	    run();
        // TODO: Sleep here and only wake on new event
    }        
		
	return 0;
}
