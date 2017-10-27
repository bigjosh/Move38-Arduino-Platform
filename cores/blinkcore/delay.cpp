/*
 * Timer related functions
 * 
 */ 


#include "hardware.h"
#include "blinkcore.h"

#include <util/delay.h>

#include "debug.h"

#include "timer.h"
#include "delay.h"

// Uses timer1 to provide accurate delay
// Assumes timer1 stopped on entry, leaves timer1 stopped on exit
// Assumes timer1 overflow flag cleared on entry, leaves clear on exit

// TODO: After all this work, seems like probably better handled in blinklib by just polling on millis()?
// Maybe this is good for sleeping while in delay? Nah. Oh well. 
    
void delay_msx( uint16_t ms ) {
        
    
    TCNT1 = 0;          // Start timer at 0. Assumes timer is normally stopped. 
             

    #if F_CPU != 4000000
        #error delay_ms is hardcoded to 4Mhz system clock. You may have to update timer2 prscaller here
    #endif
                
    #if DELAY_PRESCALER != 1024
        #error DELAY_PRSCALER must match actual prescaller give to hardware timer1
    #endif 
        
    // This calculation order preserves precision in integer math
    
    const unsigned timer_ticks=  (CYCLES_PER_MS * (unsigned long) ms ) / DELAY_PRESCALER ;    

    SBI( TIFR1 , ICF1 );                // Clear TOP match flag in case it is set.
                                        // "ICF can be cleared by writing a logic one to its bit location."
    
    ICR1 = timer_ticks;                 // We will set ICF1 bit we hit this  

    TCCR1B =
    
        _BV( WGM12) | _BV( WGM13) |         // Mode 12 - CTC TOP=ICR1 Immediate MAX
    
        _BV( CS12) | _BV(CS10) ;            // PRESCALER= clk/1024, starts timer

            
    while ( !TBI( TIFR1 , ICF1 ));       // Wait for OVR flag to indicate we are done
                                         // When the Input Capture Register (ICR1) is
                                         // set by the WGM1[3:0] to be used as the TOP value, the ICF Flag is set when the counter reaches the
                                         // TOP value.
            
    TCCR1B =0;          // Stop timer

    
}
      