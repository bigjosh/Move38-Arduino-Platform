#include "blink.h"

void setup() {

  // No setup needed for this simple example!  

}

void loop() {

  // a millisecond is 1/1,000th of a second, so
  // delay(500) will wait 1/2 a second, so
  // this code will blink the tile on and off
  // once per second!

  setColor( RED );
  delay(500);    

  setColor( OFF );
  delay(500);
  

}
