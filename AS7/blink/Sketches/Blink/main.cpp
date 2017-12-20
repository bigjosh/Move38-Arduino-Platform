/*
 * Blinks - Color Contagion
 * 
 * If alone, glow white
 * Otherwise transition to neighboring color
 * Holdoff after transitioning to neighboring color for length of transition
 * 
 */

#include "blinklib.h"

#include "utils.h"
#include "Serial.h"

Color colors[] = { RED , CYAN , MAGENTA, YELLOW, BLUE, GREEN };
uint32_t timeLastChanged_ms = 0;
uint16_t changeHoldoff_ms = 1000;

//ServicePortSerial sp;

void setup() {
    setColor( BLUE );
    
}

uint32_t nextSend=0;

uint8_t nextData =0; 

void loop() {
    
    uint32_t now  = millis();
    
    if ( now > nextSend ) {
        
        irSendDataX(  nextData++ , 0b00111111 );
        
        nextSend = now + 1000;
        
        setColor( RED );
                       
    }        else {
        
        setColor( OFF );        
    }        
    

}