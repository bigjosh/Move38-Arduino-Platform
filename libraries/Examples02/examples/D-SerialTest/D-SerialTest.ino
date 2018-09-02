/*
 * Example showing how to use the serial port.
 * 
 * To use this you'll need to connect a serial terminal to the Blinks service port. 
 * More info on how to do that here...
 * https://github.com/bigjosh/Move38-Arduino-Platform/blob/master/Service%20Port.MD
 */

#include "Serial.h"

ServicePortSerial Serial;

void setup() {

  Serial.begin();
  Serial.println("Send the letter R,G, or B to change color...");

  setColor( WHITE ) ; 
  
}


void loop() {

  if (Serial.available()) {

    int c = Serial.readWait();

    switch (c) {

      case 'R': 
      case 'r':      
        setColor(RED);
        Serial.println("Color set to RED.");        
        break;
        
      case 'G':
      case 'g': 
        setColor(GREEN);
        Serial.println("Color set to GREEN.");                
        break;
        
      case 'B':
      case 'b': 
        setColor(BLUE);
        Serial.println("Color set to BLUE.");                
        break;

      case '\r':
      case '\n':
        break;      // Ignore carridge return/enter key
       
      default: 
        Serial.println("I didn't understand.");
        break;

    }

  }
}


