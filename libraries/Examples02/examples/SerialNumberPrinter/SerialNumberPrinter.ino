/*
  
  Every blink tile comes into this world with its own untique serial number.
  
  This sketch prints the current tile's serial number out to the serive port.
  
  The serial number is 9 bytes long, and is printed with hex numbers like this...
  
  "For a good time, call:39-11-1-50-4E-38-33-33-39"
  
    ...or...
    
  "For a good time, call:30-20-3-50-4E-38-33-33-39"

  The first bytes are more likely to be different than the last bytes, in case
  you can't remeber the whole thing (it is long). 

*/

#include "utils.h"
#include "Serial.h"

ServicePortSerial sp;

void setup() {
  
  sp.begin();
  
  sp.print("For a good time, call:");
  
  const utils_serialno_t *sn = utils_serialno();
  
  // Embellished to print with dashes between bytes, trailing LF

  int i=0;

  while ( i<SERIAL_NUMBER_LEN-1){
    sp.print(  sn->bytes[i++] , HEX );
    sp.print('-');        
  }      
    
  sp.println(sn->bytes[i] , HEX );
}

void loop() {
  // This page intentional left blank
}

