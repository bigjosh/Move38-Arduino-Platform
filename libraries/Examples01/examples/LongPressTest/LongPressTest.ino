#include "blinklib.h"

// ClickTest
// Blinks RED   for a single click
// Blinks GREEN for a long press

void setup() {

  // No setup needed for this simple example!  

}

void loop() {

  if (buttonSingleClicked()) {
    setColor( RED );    // Blink red for 1/20th of a second    
  } else if (buttonLongPressed() ) {
    setColor( GREEN );    // Blink red for 1/20th of a second
  } else {
    return;              // No clicks detected. 
  }

  delay(100);       // Show the color long enough to see
  setColor(OFF);
      
}

