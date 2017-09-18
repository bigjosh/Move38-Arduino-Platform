/*

    This library presents an API for game developers on the Move38 Blinks platform.
    More info at http://Move38.com
    
    This API sits on top of the hardware abstraction layer and presents a simplified representation
    of the current state of the tile and its adjacent neighbors. 
    
    It handles all the low level protocol to communicate state with neighbors, and
    offers high level functions for setting the color and detecting button interactions. 

*/

#include "Arduino.h"

#include "blink.h"



#include "pixel.h"
#include "time.h"
#include "button.h"

void setColor( Color newColor ) {
    
    pixel_SetAllRGB( (( newColor >> 8 ) & 0b00001111 ) << 4 , (( newColor >> 4 ) & 0b00001111 ) << 4 , (( newColor  ) & 0b00001111  ) << 4 );
    
}    
    
// Note that we do not expose _delay_ms() to the user since then they would need
// access to F_CPU and it would also limit them to only static delay times. 
// By abstracting to a function, we can dynamically adjust F_CPU and 
// also potentially sleep during the delay to save power rather than just burning cycles.     
    
void delay( unsigned long millis ) {
    delay_ms( millis );
}    


bool buttonDown(void) {
    
    return button_down();
    
}    

// This is the entry point where the platform will pass control to 
// us after initial power-up is complete

void run(void) {
    
    setup();
    
    while (1) {
     
        loop();
           
    }        
    
}    
