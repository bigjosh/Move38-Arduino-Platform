/*
 * Blinkani tester
 *
 * Demonstrates the blinkani animation library.
 *
 * Press the button to make 3 red blinks with 200ms on/off time. 
 */

#include "blinklib.h"
#include "blinkani.h"

#include "Serial.h"

ServicePortSerial sp;

void setup() {
    
    setColor( BLUE );        
    blinkAniBegin();
    
    sp.begin();
    sp.println("setup");
           
}

bool newEventFlag=false;

void loop() {
    
    if ( buttonSingleClicked() ) {
        sp.println("pressed");
        
        //blink(1000 , RED, 1000 , GREEN );
        
        blink( MAGENTA , 500  );        
        newEventFlag=true;
        
    }   
    
    if (buttonDoubleClicked() ) {
        
        strobe( 5 , GREEN , 200 );
    }        
    
    
    
}    