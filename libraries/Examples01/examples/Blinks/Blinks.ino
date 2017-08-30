/*

  Blinks

  Turns on all the LED red then green then blue, each for one second, then off for one second, repeatedly.

*/

void setup() {
  // put your setup code here, to run once:
}

void loop() {

  setAllRGB( 200 , 0 , 0 );          // Set all LEDs red
  delay(1000);                       // wait for a second  

  setAllRGB( 0 , 200 , 0 );          // Set all LEDs green
  delay(1000);                       // wait for a second  

  setAllRGB( 0 , 0 , 200 );          // Set all LEDs blue
  delay(1000);                       // wait for a second  

  setAllRGB( 0 , 0 ,   0 );          // Set all LEDs off
  delay(1000);                       // wait for a second  


}
