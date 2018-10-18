/*
 * bareblinkOS.cpp
 *
 * An example that runs directly on the blinkOS without blinklib
 *
 */ 


#include "blinkos.h"


void setupEntry() {
    
    asm("nop");
    
};  

uint8_t step;
millis_t nextstep;

void loopEntry( loopstate_in_t const *loopstate_in , loopstate_out_t *loopstate_out) {
    
    
    if ( loopstate_in->buttonstate.bitflags & BUTTON_BITFLAG_DOUBECLICKED ) {
                
        loopstate_out->colors[step] =  pixelColor_t( 0 , 0 , 0 , 1 );
                
        step++;
        
        if (step>= PIXEL_FACE_COUNT ) {
            step=0;
        }            

        loopstate_out->colors[step] = pixelColor_t( 23 , 10, 0 , 1 );

        nextstep = loopstate_in->millis + 200;
    }        
    
    
}    