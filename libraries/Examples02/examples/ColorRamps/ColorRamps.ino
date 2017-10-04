#include "blink.h"

/// Show some soothing color ramps.

void setup() {

  // No setup needed for this simple example!  

}

void loop() {

  Color colors[] = { RED , YELLOW , GREEN , CYAN , BLUE , MAGENTA };

  for( byte i=0; i< COUNT_OF(colors); i++) {

    Color c = colors[i];

    byte brightness=0;
      
    // Ramp up...
    while (brightness<BRIGHTNESS_LEVELS) {

      setColor( dim( c , brightness ) );
      brightness++;
      
      delay(10);      
    }

    // Ramp down...
    while (brightness>0) {
  
      brightness--;
      setColor( dim( c , brightness ) );
      
      delay(10);       
    }

  }

}
