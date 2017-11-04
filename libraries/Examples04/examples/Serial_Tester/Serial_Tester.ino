/*
 * An example showing how to use the service port serial connection. The service port give you a handy way to 
 * type text into your programm running on a tile, and have it print back messages to you. This can be really
 * handy when debugging!
 * 
 * "The most effective debugging tool is still careful thought, coupled with judiciously placed print statements."
 *   -Brian Kernighan, "Unix for Beginners" (1979)
 * 
 * 
 * To use this, you must have...
 *    (1) a blink tile with a serive port installed (it is a little 4 pin plug)
 *    (2) a cable adapter that plugs into the service port and connects it to a USB on your computer
 *    
 * If you've got those, then you need to got the menu above and....
 * 
 *    (1) Goto Tools->Port and pick the com port the USB adapter landed on (if you give up, you can try them all)
 *    (2) Goto Tools->Serial Monitor to open the Arduino serial monitor window
 *    (3) Pick "500000 baud" in the serial monitor window
 *    (4) Pick "No line ending" in the serial port monitor window
 *    
 * Now download this sketch to your blink and you should see an nice welcome message pop up   
 * in the serial monitor window telling you want to do next!
 * 
 * (Hint: Type the letter "b" in the bar at the top of the serial monitor window and press the "Send" button)
 * 
 */

#include "blinklib.h"

#include "Serial.h"

#include <ctype.h>    // toupper()

ServicePortSerial Serial;

void setup() {

  Serial.begin(); 

  Serial.println("What is your color wish, master?");  

}


void loop() {

  if (Serial.available()) {

    int i = Serial.read();

    if (isprint(i)) {
      Serial.print("You pressed the '");
      Serial.print( (char) i  );          // Casting to a `char` invokes the print that displays the acii character
      Serial.print("' key. ");
    } else {
      Serial.print("The code of your key was ");
      Serial.print(i);
      Serial.print("." );      
    }    
    
    switch ( toupper( (unsigned char) i) )  {

        case 'R': setColor( RED ) ;
          Serial.println("Red");
          break;
         
        case 'G': setColor( GREEN ) ; 
          Serial.println("Green");
          break;
          
        case 'B': setColor( BLUE ) ; 
          Serial.println("Blue");
          break;

        default:
          Serial.println("Unknown color, try keys `R`, `G`, or `B`.");
          break;
              
    }

    
  }
  
}
