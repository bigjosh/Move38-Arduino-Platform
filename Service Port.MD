
> *"The most effective debugging tool is still careful thought, coupled with judiciously placed print statements."*
 
-Brian Kernighan, "Unix for Beginners" (1979)


#### Service Port

Each Blink tile has a service port to make developing and debugging easier. For example:

* See print statements
* Accept type in values and commands 
* Output precise timing signals to an oscilloscope
 

### Seeing output

When writing normal game code, you can use the the service port as a serial connection to the host computer.

#### Connection 

Get yourself a Blinks Dev Candy board, which converts the service port connector into a 6-pin header that connects to a USB serial converter like this...

http://amzn.to/2ptPUv5

#### Use

* Connect the USB serial port adapter to the computer
* Open Arduino IDE
* Pick the correct serial port from `Tools->Port`
* Open the monitor with "Tools->Serial Port Monitor" 
* Pick `1000000 baud`. (Yes, it is that fast! :) )

You will now see anything that the tile prints out. 

You can also use any other serial monitor program with the same port and baud settings.

#### Instrumentation

The easiest way to get prints out the serial port is to use the `ServicePortSerial` class like this...

~~~
/*
 * Prints a welcoming message to the servie port.
 * 
 * To see the message, connect a serial terminal to the Blinks service port. 
 * More info on how to do that here...
 * https://github.com/bigjosh/Move38-Arduino-Platform/blob/master/Service%20Port.MD
 */

#include "Serial.h"

ServicePortSerial Serial;

void setup() {

  Serial.begin(); 

}


void loop() {

  Serial.println("Hello Serial port!");
    
}; 
~~~ 

#### Input

The Serial class also has several read functions that let you input data from the serial terminal. Check out the Tile World example to see one way to do this.  
    
Note that that by default Arduino serial monitor does not send what you type until you press enter.  