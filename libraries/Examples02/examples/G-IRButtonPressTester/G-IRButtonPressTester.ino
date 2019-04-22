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

#define VALUE_BUTTON_UP   'U'
#define VALUE_BUTTON_DOWN 'D'

// Did we get an error onthis face recently?
Timer errorOnFaceTimer[ FACE_COUNT ];

const int showErrTime_ms = 500;    // Show the errror for 0.5 second so people can see it 

void loop() {

  FOREACH_FACE(f) {

    // Set the color on this face based on what we see...

    if ( !isValueReceivedOnFaceExpired( f ) ) {      // Have we seen an neighbor on this face recently?

      switch ( getLastValueReceivedOnFace( f ) ) {
        
        case VALUE_BUTTON_UP : 

          // A zero is idle - we have a neighbor and his button is not down
          setColorOnFace( dim( BLUE , 100 ) , f );
          break;
           
        case VALUE_BUTTON_DOWN: 
          // A 1 means other side button down, so show green
          setColorOnFace( GREEN , f );
          break;

        default: 
          // anything else is an error
          // We set a timer so this face will show red for a little while to make sure it is seen
          errorOnFaceTimer[f].set( showErrTime_ms  );           
          break;
            
        }
        
      } else {

        // No neighbor on this face right now

        setColorOnFace( dim( YELLOW , 40 ) , f );
        
      }

      // The red (error) will cover any other color on the face unil the timer times out      

      if ( !errorOnFaceTimer[f].isExpired() ) {

        setColorOnFace( RED , f );
        
      } 
            

  }

  // Finally update the value we send to others to match our current button state


  if ( buttonDown() ) {

    setValueSentOnAllFaces( VALUE_BUTTON_DOWN ) ;

  } else {

    setValueSentOnAllFaces( VALUE_BUTTON_UP ) ;
    
  }
        
}
