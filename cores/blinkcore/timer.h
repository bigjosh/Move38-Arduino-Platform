/*
 * timer.h
 *
 * Timer related functions
 *
 */ 

#ifndef TIMER_H_
#define TIMER_H_

// Class for a timer-based callback
// `when` is the time in millis when you want to get called. 
// Calling start on an already running timer resets its `when` time.

#include "blinkcore.h"

class Timer_Callback {
    
    uint32_t when;
    
    void (*callback)(void);
    
    Timer_Callback *next;
    
    public:
    
    void Start( uint32_t when , void (*callback)(void) ); 
    void Stop(void); 
        
};    

void timer_init(void);
 
// Start the timer

void timer_enable(void);

void timer_disable(void);

// User supplied callback. Called every 512us with interrupts on. Should complete work in <<256us.

void timer_callback(void);

// These values are based on how we actually program the timer registers in timer_enable()
// There are checked with assertion there, so don't chnage these without chaning the actual registers first

#define TIMER_PRESCALER     8
#define TIMER_RANGE     0x100

#define TIMER_CYCLES_PER_TICK (TIMER_PRESCALER*TIMER_RANGE)

#define F_TIMER ( F_CPU / TIMER_CYCLES_PER_TICK )       // Timer2 overflow frequency. Units of TICKS/SEC. Comes out to 1953.125 = 1.953125 kilohertz, 512us per tick

#define MILLIS_PER_SECOND 1000

#define CYCLES_PER_MS (F_CPU/MILLIS_PER_SECOND)

#define TIMER_MS_TO_TICKS(ms)  ( ( CYCLES_PER_MS * ms)   / TIMER_CYCLES_PER_TICK )  // Slightly contorted to preserve precision

#define US_PER_SECOND 1000000

#define CYCLES_PER_US (F_CPU/US_PER_SECOND)

#define US_TO_CYCLES(us) (us * CYCLES_PER_US )
   
// Delay `millis` milliseconds.

void delay_ms( unsigned long millis);    
    
#endif /* TIMER_H_ */