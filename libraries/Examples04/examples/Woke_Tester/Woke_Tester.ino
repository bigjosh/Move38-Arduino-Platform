/* 
 *  
 *  Woke_tester
 *  
 *  Likely the most boring demo ever. 
 *  Blinks the LEDs red while waiting to fall sleep. 
 *  Upon waking, the LEDs do a green dance around the face, 
 *  then back to red flashing while we wait to fall sleep again. 
 *  
 */

#include "blinklib.h"

void setup() {
}

int danceStep;
bool dopaFlag = false;

uint32_t nextEvent=0; 

void loop() {

  if ( hasWoken() ) {   

    // I'm not asleep
    
    dopaFlag = 1;
    danceStep =0;

  }

  uint32_t now = millis(); 

  if ( now >= nextEvent ) {

    if (dopaFlag) {   
  
      setFaceColor( danceStep , GREEN );
  
      danceStep++;
  
      if (danceStep == FACE_COUNT ) {
  
        dopaFlag=0;
        
      }

    } else {    // Waiting for the inevitable 

      setColor( RED); 
      
    }

    nextEvent = now + MILLIS_PER_SECOND;

  } else {

    setColor( OFF );      // Clear the display on the next loop() after it is set. This makes the above effects into brief flashes that last 1 pixel refresh cycle
    
  }
  
}
