#include "blinklib.h"

// Use the buttonPressed() function to move a blue pixel one step around the face 
// each time the button is pressed down.
// Note that there is no latnecy on the button going down as long as it happens a significant ammount of
// time after the most recent up. Debouncing happens *after* each button state change.


byte step=0;

void setup() {

  // Show the first step so they know wee are running
  setFaceColor( step , BLUE );

}

void loop() {


  if (buttonPressed()) {

    // Turn off previous pixel
    setFaceColor( step , OFF );

    // Step to next pixel 
    step++;

    // Reset back to 0 after every 6. 
    
    if (step==6) step=0;

    // Turn on new pixel
    setFaceColor( step , BLUE );

  }

}


