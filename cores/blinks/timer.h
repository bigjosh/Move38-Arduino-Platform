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

void timer_callback(void) __attribute__((weak));

#define F_TIMER ( F_CPU /8 /256 )       // Timer2 frequency after /8 prescaller and 0xFF TOP. Comes out to 1.953125 kilohertz, 512us
   
// Delay `millis` milliseconds.

void delay_ms( unsigned long millis);    
    
#endif /* TIMER_H_ */