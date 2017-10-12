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

#include <util/delay.h>

#include "debug.h"

#include "power.h"

#include "pixel.h"          // We depend on the pixel ISR to call us for timekeeping

#include "callbacks.h"

#include "timer.h"

#include "ir_callback.h"    // Just connects the IR periodic callback to the timer ticks

#if F_CPU != 4000000
    #error This code assumes 4Mhz clock
#endif

// TODO: Run the timer top so one overflow is exactly 500ms to make millis calcs easier. 

// We will run in normal mode where we always count up to 255 and then overflow
// We will use the overflow interrupt to drive most of our tasks

void timer_init(void) {
    
    
    SBI( TIMSK2 , TOIE2 );      // Enable interrupt on overflow
    
}

// Start the timer

void timer_enable(void) {
    
    // This is pretty simple since we are not using any of the outputs and 
    // we are running in the default "normal" mode
    
    /*
        23.7.1 Normal Mode
        The simplest mode of operation is the Normal mode (WGM2[2:0] = 0). In this mode the counting direction
        is always up (incrementing), and no counter clear is performed. The counter simply overruns when it
        passes its maximum 8-bit value (TOP = 0xFF) and then restarts from the bottom (0x00).        
    */

    #if TIMER_RANGE != 0x100
        #error Must change the definition of TIMER_RANGE to match the actual values in the timer2
    #endif

    TCNT2 = 128;                        // Start completely out of phase with the other timer. See holdTimers()
    
    TCCR2B = _BV( CS21 );               // Start the timer with Prescaller /8    
    
    #if TIMER_PRESCALER != 8
        #error Must change the definition of TIMER_PRESCALLER to match the actual values in the timer2
    #endif
    
    
        
}        

void timer_disable(void) {    
    TCCR2B = 0;                         // Stop timer
}    


struct ISR_CALLBACK_TIMER : CALLBACK_BASE< ISR_CALLBACK_TIMER> {
    
    static const uint8_t running_bit = CALLBACK_TIMER_RUNNING_BIT;
    static const uint8_t pending_bit = CALLBACK_TIMER_PENDING_BIT;
    
    static inline void callback(void) {
        
        timer_callback();
        
    }
    
};

// This is executed every 512us

ISR( TIMER2_OVF_vect ) {
    
    ir_tick_isr_callback();             // Call the IR callback with interrupts off

    
    ISR_CALLBACK_TIMER::invokeCallback();
    
}    

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

