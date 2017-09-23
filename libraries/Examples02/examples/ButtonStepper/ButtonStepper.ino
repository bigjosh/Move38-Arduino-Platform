#include "blink.h"

// Use the buttonPressedCount() function to move a blue pixel one step around the face 
// each time the button is pressed down.
// Note that there is no latnecy on the button going down as long as it happens a significant ammount of
// time after the most recent up. Debouncing happens after each button state change.

void setup() {

  // No setup needed for this simple example!  

}

void loop() {

  byte step=0;

  // Show the first step so they know wee are running
  setFaceColor( step , BLUE );
  
  while (1) {

    byte pressedCount = buttonPressedCount();

    // We could get rid of this test, but then we'd turn off the pixel on every cycle
    // even when its positioin didn't move and that would look flashy.

    if (pressedCount) {

      setFaceColor( step , OFF );

      // Remember that the button could have been pressed more than once since we last checked
      // (very unlikely in this simple demo, but possible in sketches where the loop() takes a long
      // time)
      
      step+= pressedCount;

      // Reset back to 0 after every 6. 
      // Could have also used a mod here, but this way makes it easy to 
      // do something fun on every rotation. Can you make it so that we step to
      // a new color each time we go around?
      
      while (step>=6) step-=6;
  
      setFaceColor( step , BLUE );

    }

  }

}


