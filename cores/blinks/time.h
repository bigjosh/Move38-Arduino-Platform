/*
 * time.h
 *
 * Time related functions
 *
 */ 

#ifndef TIME_H_
#define TIME_H_

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


// Delay `millis` milliseconds. 

void delay_ms( unsigned long millis);
    
#endif /* TIME_H_ */