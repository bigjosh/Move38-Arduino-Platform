#include "blinklib.h"

/// Show some soothing color ramps.

void setup() {

  // No setup needed for this simple example!  

}

Color colors[] = { BLUE ,   RED , GREEN   };

byte color = 0; 

int brightness = 1; 

int direction = 1;

void loop() {

   setColor( dim( colors[color] ,  brightness  ) );


   if (brightness== MAX_BRIGHTNESS) {     // Got to the top, turn around
    
      direction = -1; 
      
   } else if (brightness==0) {              // Got to the bottom, turn around and step to next color

      direction = 1;

      color++;

      if (color==COUNT_OF( colors )) {
        color=0;
      }
   }

   brightness += direction; 
   
}
