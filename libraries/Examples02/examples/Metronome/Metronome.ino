#include "blinklib.h"

// Make a red blink once per second using the millis() function. 

void setup() {

  // No setup needed for this simple example!  

}

unsigned long nexttick = 0;

void loop() {

  // Note that here we check if millis() is _greater_than_ lasttick rather than equal to it becuase
  // it is possible to miss a tick so we might never see when moment when they are exactly equal.
  // This means that our metronome might be off by a millisecond or two, but that's ok. 

  if (millis() >= nexttick) {

    setColor( RED );    // Blink red for 1/20th of a second
    delay(50); 
    setColor( OFF );

    // Note here we increment the nexttick rather than just setting it to the current time
    // plus 100 milliseconds becuase that would make there be at least 1,050 milliseconds between 
    // ticks. This way we will always average 1,000 millis between ticks and errors will not
    // accumulate. See why?
    
    nexttick+=1000;     // tick again 1 second (1000 milliseconds) later than when we last ticked 
    
  }

  
}
