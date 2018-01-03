/*
 * Blinkani tester
 *
 * Demonstrates the blinkani animation library.
 *
 * Press the button to make 3 red blinks with 200ms on/off time. 
 */

#include "blinklib.h"
#include "blinkani.h"

void setup() {
    
    setColor( BLUE );        
    blinkAniBegin();
           
}

void loop() {
    
    if (buttonPressed()) {
        blink( 200 , 3 , RED );        
    }   
    
}    