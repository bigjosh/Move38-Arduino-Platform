// Simple sleep demo
// First shows a red ramp fade at power up
// Then Blink goes to sleep with an 8 second timeout
// If anything wakes it (currently only a button change), then blinks blue
// If the sleep trimes out after 8 seconds, then blinks green
// Repeats

// You can measure power usage durring the sleep period

void setup() {
    // put your setup code here, to run once:

    // Show a nice 2 second red power up fade - makes it so you can differentiate a reset
    
    for( uint8_t i=0; i<200;i++) {
       
        setAllRGB(  i , 0 , 0 );   
        delay(10);
        
    }   
         
    setAllRGB(  0 , 0 , 0 );

}

void loop() {
 
 while (1) {

     disablePixels();     // Turn of pixels before sleeping
     
     uint8_t timeoutflag= powerdownWithTimeout( TIMEOUT_8S );   // Sleep with a 2 second timeout
     enablePixels();
     
     if (timeoutflag) {
        setAllRGB(  0 , 200 , 0 );   // Blink green on timeout
        
     } else {
         
        setAllRGB(  0 , 0 , 200 );   // Blink blue on not timeout (for now only button)
         
     }                          
     
     delay(100);              // Short blink
     setAllRGB(  0 , 0 , 0 );
          
 }
}
