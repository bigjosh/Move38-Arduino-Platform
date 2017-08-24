// HelloBlinks
// Light all the LEDs up red if button is down, green if button is up

void setup() {
  // put your setup code here, to run once:
}

void loop() {

  if (buttonDown()) {
    setRGB( 200 , 0 , 0 );
  } else {
    setRGB( 0 , 200 , 0 );
  }
}
