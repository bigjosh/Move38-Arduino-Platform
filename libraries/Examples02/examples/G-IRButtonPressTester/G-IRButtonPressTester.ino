/*
 * IR Button Press
 * 
 * Great for testing if blinks work and can communicate over IR
 * 
 * Each face shows:
 *   YELLOW - No neighbor on this face
 *   BLUE   - Neighbor button UP
 *   GREEN  - Neighbor button DOWN
 *   RED    - Error. Saw unexpected datum on this face. Resets after 0.5 second. 
 * 
 */


void setup() {
  // Blank
}

// Pick some unlikely values so we are more likely to show an error if a different 
// game is runnning on the oposing blink

#define VALUE_BUTTON_UP   12
#define VALUE_BUTTON_DOWN 21

// Did we get an error onthis face recently?
Timer errorOnFaceTimer[ FACE_COUNT ]; 

const int showErrTime_ms = 500;    // Show the errror for 0.5 second so people can see it 

void loop() {

  // Make a little pulsation so we know it is working

  uint8_t brightness = sin8_C( (millis()/4) & 0xff );

  FOREACH_FACE(f) {

    Color color;     

    // Set the color on this face based on what we see...

    if ( !isValueReceivedOnFaceExpired( f ) ) {      // Have we seen an neighbor on this face recently?

      switch ( getLastValueReceivedOnFace( f ) ) {
        
        case VALUE_BUTTON_UP : 
          color = BLUE;
          break;
           
        case VALUE_BUTTON_DOWN: 
          color=GREEN;
          break;

        default: 
          // anything else is an error
          // We set a timer so this face will show red for a little while to make sure it is seen
          errorOnFaceTimer[f].set( showErrTime_ms  );           
          break;
            
        }
        
      } else {

        // No neighbor on this face right now

        color=YELLOW;
        
      }

      // The red (error) will cover any other color on the face unil the timer times out      

      if ( !errorOnFaceTimer[f].isExpired() ) {

        color = RED; 
        
      } 
      
      setColorOnFace( dim( color , brightness ) , f );           

  }

  // Finally update the value we send to others to match our current button state


  if ( buttonDown() ) {

    setValueSentOnAllFaces( VALUE_BUTTON_DOWN ) ;

  } else {

    setValueSentOnAllFaces( VALUE_BUTTON_UP ) ;
    
  }
        
}
