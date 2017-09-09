/*
 * BlinksFirmware.c
 *
 * Created: 3/26/2017 8:39:50 PM
 * Author : josh.com
 */ 

#include "blinks.h"
#include "Arduino.h"

#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "debug.h"
#include "utils.h"
#include "ir.h"
#include "pixel.h"
#include "button.h"
#include "adc.h"
#include "power.h"

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


static void init(void) {

    mhz_init();				// switch to 4Mhz. TODO: Some day it would be nice to go back to 1Mhz for FCC, but lets just get things working now.
    
    DEBUG_INIT();			// Handy debug outputs on unused pins
    
    power_init();
    
    adc_init();			// Init ADC to start measuring battery voltage
    
    ir_init();
    ir_enable(); 
        
    pixel_init();
    pixel_enable();
    
    button_init();
    
    sei();					// Let interrupts happen. For now, this is the timer overflow that updates to next pixel.

}    
    
int main(void)
{
	init();
	
	setup();
	
	for (;;) {
        
		loop();
	}
	
	return 0;
}
