#include "blink.h"

// ClickTest
// Blinks RED   for a single click
// Blinks GREEN for a double click
// Blinks BLUE  for a multi  click (3 ro more)

// Remember that holding the button down will abort any click in progress

void setup() {

  // No setup needed for this simple example!  

}

void loop() {

  if (buttonSingleClicked()) {
    setColor( RED );    // Blink red for 1/20th of a second    
  } else if (buttonDoubleClicked()) {
    setColor( GREEN );    // Blink red for 1/20th of a second
  } else if (buttonMultiClicked()) {
    setColor( BLUE );    // Blink red for 1/20th of a second
  } else {
    return;              // No clicks detected. 
  }

  delay(100);       // Show the color long enough to see
  setColor(OFF);
      
}
