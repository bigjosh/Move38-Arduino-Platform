/*
 * Time related functions
 * 
 */ 


#include "hardware.h"

#include <util/delay.h>


#include "debug.h"

#include "power.h"


#include "time.h"


// Delay `millis` milliseconds. 
// TODO: sleep to save power?
// TODO: User timer?

// Of course this will run slighhtly long, but betyter to go over than under
// and no reasonable expeaction that the delay will be *exact* since there are
// function call overheads and interrutps to consider. 

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

