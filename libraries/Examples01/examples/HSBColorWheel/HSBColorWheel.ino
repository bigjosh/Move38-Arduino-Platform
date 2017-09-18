#include "blink.h"

// Spin around in goovy HSB color space
// HSB stands for Hue, Saturation, and Brightness

void setup() {

  // No setup needed for this simple example!  

}

void loop() {

  byte hue=0;

  while (1) {
  
    // Spin the hue while keeping color saturation and brightness at max  
    setColor( makeColorHSB( hue , 255 , 255 ) );
    delay(20);  

    // Becuase we are using an 8-bit byte for the `hue` variable, 
    // this will automatically roll over from 255 back down to 0  
    // (255 is 11111111 in binary, and 11111111 + 00000001 = 00000000)
    hue++;      
  }

}


