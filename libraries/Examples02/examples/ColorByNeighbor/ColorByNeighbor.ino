/*
 * Color By Number/Neigbor
 * 
 * An example showing how to use the blinkstate library to count the number of neighboors. 
 * 
 * Each Blink broadcasts 1 to its neighbors, letting its neigbors know it's present.
 * Each Blink displays a color based on the number of neighbors around it.
 * 
 * Shows a little spin on startup so you know it is running. 
 * 
 */

#include "blinklib.h"
#include "blinkstate.h"

// color by number of neighbors
Color colors[] = {  
  dim(WHITE,5),   // 0 neighbors
  RED,            // 1 neighbors
  YELLOW,         // 2 neighbors
  GREEN,          // 3 neighbors
  CYAN,           // 4 neighbors
  BLUE,           // 5 neighbors
  MAGENTA,        // 6 neighbors
}; 

void setup() {

  FOREACH_FACE(i) {
      // simple startup sequence with a magenta spin
      setFaceColor( i , MAGENTA );
      delay(100);
      setFaceColor( i , OFF );
  }

  blinkStateBegin();  
  
  setState(1);
}


void loop() {

  // count neighbors
  int numNeighbors = 0;

  // We are being slightly tricky here - since each neighboor sends 
  // the value "1" and a face that has no neighboor returns a value of "0",
  // we can count the total number of neighboors by simply suming all the 
  // data values recieved. 

  FOREACH_FACE(i) {
    numNeighbors+=getNeighborState(i);
  }

  // look up the color to show based on number of neighbors
  setColor( colors[ numNeighbors ] );
}
