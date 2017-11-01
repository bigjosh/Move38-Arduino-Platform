/*
 * blinkstate tester
 * 
 * An example showing how to use the blinkstate library. 
 * 
 * Each tile has a current locate state. That state maps to a color. 
 * 
 * The tile's state is continously broadcast on all faces. 
 * 
 * Press the button to change the tile's state. The state is 
 * show as a color while the button is held down. 
 * 
 * The "white" state is special. In this state the tile does not broadcast. 
 * 
 * The tile is also always listing on all faces. When it recieves a state from a
 * neighboring tile, it displays the color on the associated face (unless the
 * local button is down, in which case it shows the local state color).
 * 
 */

#include "blinklib.h"
#include "blinkstate.h"

void setup() {
  // put your setup code here, to run once:

  FOREACH_FACE(f) {

      setFaceColor( f , BLUE );
      delay(100);
      setFaceColor( f , OFF );
      blinkStateBegin();

  }
}

Color colors[] = { dim(WHITE,5) , RED, GREEN, BLUE };    // States 0,1,2,3
                                                // This will make us blink WHITE when we switch to state 0
byte state=0;

void loop() {

  if (buttonPressed()) {            // Rememeber - we get one buttonPressed event each time the button is pressed down

    state++;

    if (state== COUNT_OF( colors )) {
      state=0;      
    }

    setColor( colors[ state ] );        // Feedback to user 

    setState( state );                // Tell the world 


  } else if (!buttonDown()) {             // As long as the button stays down, we will keep showing the new color for user feedback. 

    
    FOREACH_FACE(f) {

      setFaceColor( f , colors[ getNeighborState(f) ] );


    }

  }
    
}
