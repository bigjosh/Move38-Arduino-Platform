/*
  
  NOTE: This sketch is only interesting if you have a Blinks Dev Candy adapter connecting
  the blink to your serial port!
  
  Every blink tile comes into this world with its own untique serial number.
  
  This sketch prints the current tile's serial number out to the serial port.
  
  The serial number is 9 bytes long, and is printed with hex numbers like this...
  
  "For a good time, call:39-11-1-50-4E-38-33-33-39"
  
    ...or...
    
  "For a good time, call:30-20-3-50-4E-38-33-33-39"

  The first bytes are more likely to be different than the last bytes, in case
  you can't remember the whole thing (it is long). 

*/

      

#include "Serial.h"

ServicePortSerial sp;

void setup() {
  
  sp.begin();
  
}

void loop() {
    
  sp.print("For a good time, call:");
  
  // First load the serial number buytes into a buffer
  
  byte snBuffer[SERIAL_NUMBER_LEN];   
  
  for( byte i=0; i<SERIAL_NUMBER_LEN ; i++) {
    
    snBuffer[i] = getSerialNumberByte(i); 
    
  }
  
  // Next print them out embellished with dashes between bytes, trailing LF
  

  for( byte i=0;  i<SERIAL_NUMBER_LEN-1 ; i++ ) { // Stop printing one short of the end
    sp.print(  snBuffer[i] , HEX );
    sp.print('-');
  }
  
  // Print the final byte with no dash after it
  
  sp.println(snBuffer[SERIAL_NUMBER_LEN-1] , HEX ); 
  
  sp.println("Bye!");
  
  while (1); 
  
}

