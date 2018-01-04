/*
 * Blinkani tester
 *
 * Demonstrates the blinkani animation library.
 *
 * Single click for a 500ms magenta flash
 * Double click for a strobe of 5 green blinks each 200ms period. 
 */

#include "blinklib.h"
#include "blinkani.h"

void setup() {
    
    setColor( BLUE );        
    blinkAniBegin();
           
}

bool newEventFlag=false;

void loop() {
    
    if ( buttonSingleClicked() ) {
        
       rotate( makeColorHSB(0,0,64) , 200 );
        
    }   
    
    if (buttonDoubleClicked() ) {
        
        strobe( 5 , GREEN , 200 );
    }        
       
    
}    