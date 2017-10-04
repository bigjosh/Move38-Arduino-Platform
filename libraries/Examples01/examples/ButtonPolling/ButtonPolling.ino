#include "blink.h"

// Here we are consdtantly checking the button to see if it is down right now.
// If it is down, we set our color to red, otherwise we turn of the LEDs. 

void setup() {

  // No setup needed for this simple example!  

}

void loop() {

  if (buttonDown()) {
  
    setColor( RED );

  } else {
   
    setColor( OFF );

  }
  
}
