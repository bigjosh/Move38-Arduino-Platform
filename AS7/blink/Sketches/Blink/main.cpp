/*
 * Blinks - Color Contagion
 * 
 * If alone, glow white
 * Otherwise transition to neighboring color
 * Holdoff after transitioning to neighboring color for length of transition
 * 
 */

#include "blinklib.h"
#include "blinkstate.h"

#include "utils.h"
#include "Serial.h"

Color colors[] = { RED , CYAN , MAGENTA, YELLOW, BLUE, GREEN };
uint32_t timeLastChanged_ms = 0;
uint16_t changeHoldoff_ms = 1000;

ServicePortSerial sp;

void setup() {
  setState(1);
  setColor(colors[0]);
  
  sp.begin();
  
  sp.print("For a good time, call:");
  
  const utils_serialno_t *sn = utils_serialno();
  
  // Embellished to print with dashes between bytes, trailing LF

  for( int i=0 ; i<SERIAL_NUMBER_LEN-1; i++ ){
    sp.print(  sn->bytes[i] , HEX );
    sp.print('-');    
  }      
    
  sp.println(sn->bytes[i] , HEX );
}

void loop() {
    
  int numNeighbors = 0;
  uint32_t curTime_ms = millis();

  if( buttonSingleClicked() ) {
        setState( (getState() % 6) + 1 );
        setColor( colors[getState()-1] );
        timeLastChanged_ms = curTime_ms;
  }

  for( int i = 0; i < 6; i++ ) {
    
    byte neighborState = getNeighborState(i);
    
    if(neighborState != 0) {

      numNeighbors++;
      
      if(neighborState != getState() && curTime_ms - timeLastChanged_ms > changeHoldoff_ms){
        setState( neighborState );
        setColor( colors[getState()-1] );
        timeLastChanged_ms = curTime_ms;
      }
    }
  }
  
  if(numNeighbors == 0) {
    // alone, so glow white
    setColor(WHITE);
  }
  else {
    // together, show state
    setColor( colors[getState()-1] );
  }

}