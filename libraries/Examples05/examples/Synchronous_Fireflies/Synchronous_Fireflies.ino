#include "blinklib.h"

/*
 * Synchronous fireflies demo
 * 
 * Turns a set of tiles into fireflies that will eventually synchonize thier blinks when they are in contact in 
 * any configuration. Put the fireflies touching each other in any pattern or shape and wait for them to sync!
 * Pressing the button on a tile make it flash immedeately which is nice for unsyncing so you can see them sync back again.
 * 
 * 
 * Really cool stuff. Do read more about it...
 * 
 * https://www.google.com/search?rlz=1C1CYCW_enUS687US687&q=Mirollo+and+Strogatz+fireflies
 * 
 * The current parameters are...
 * 
 * 1) Each pass though loop() takes about 1ms due to the delay(1) at the bottom
 * 2) Our urge to blink increases by 1 per pass
 * 3) We blink when get to an urge of 1,0000 - which means we blink about once per second in isolation
 * 3) Seeing someone else blink on one of our faces increases our urge to blink by 10% of current urge. 
 * 4) Our maximum brightness is 200 and decays by 1 for each pass. This make our flash fade over about 0.1s, just because it looks nice.
 * 
 */

void setup() {
  // Empty
}

unsigned urge = 0;    // The urge to flash. Increases until we can't take it anymore!

unsigned brightness = 0;    // Current brightness as 0-200. Decays over time, one step per ms. 200 steps = ~1/5th second to fade away. 

void loop() {

        // The urge is building...

        urge++;          


        if (buttonPressed()) {      // Tickle my tummy to make be blink immediately (good for forcing tiles out of sync)

          urge = 1000;                
        
        }

        // Until we can't take it any more!...
        
        if ( urge >= 1000 ) {          

          irBroadcastData( 1 );          // Flash to neighbors! (actual value sent does not matter)

          brightness = 200;               // Max brightness

          urge = 0;

        } 


        // Let our colors shine though 
        
        if (brightness > 0 ) {

          brightness--;   // Decay brightness because it looks mucho mas suavecito 

          setColor( dim( GREEN , brightness );   // Our brightness is 0-200 and dim() range is 0-255 so we will not be as bright was we can be, but we are only a firefly. 
          
        }

        // See what our neighbors are up to....

        FOREACH_FACE(face) {

          if ( irIsReadyOnFace( face ) ) {     // Anyone near us flash?

            irGetData( face );              // Ignore the Actual data

            ///***** THE MAGIC HAPPENS!!!!! *****////
              
            urge += (urge/10);                       
                
        }
    }

    delay(1);      // Slow down a bit

}
