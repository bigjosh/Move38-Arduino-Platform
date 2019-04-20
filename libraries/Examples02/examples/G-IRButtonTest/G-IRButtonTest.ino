/*
 * IR Button Press
 * 
 * Great for testing if blinks work and can communicate over IR
 * 
 * Each pixel shows what is happening on the corresponding IR face
 * 
 * YELLOW = No neighboor seen over IR
 * BLUE   = Neighboor's button is up
 * GREEN  = Neighboor's button is down
 * RED    = Error data recieved (clears after 1 second)
 *  
 */


void setup() {
  // Blank
}


// Did we get an error onthis face recently?
Timer errorOnFaceTimer[ FACE_COUNT ];

const int showErrTime_ms = 1000;    // Show the errror for 1 second so people can see it 

void loop() {

  FOREACH_FACE(f) {

    // Set the color on this face based on what we see...

    if ( !isValueReceivedOnFaceExpired( f ) ) {      // Have we seen an neighbor on this face recently?

      switch ( getLastValueReceivedOnFace( f ) ) {
        
        case 0: 

           // A zero is idle - we have a neighbor and his button is not down
           setColorOnFace( dim( BLUE , 100 ) , f );
           break;
           
        case 1: {
            // A 1 means other side button down, so show green
           setColorOnFace( GREEN , f );
            
          }
          break;

        case 2: {
            // anything else is an error
            // We set a timer so this face will show red for a little while to make sure it is seen
            errorOnFaceTimer[f].set( showErrTime_ms  );           
          }
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

    setValueSentOnAllFaces( 1 ) ;

  } else {

    setValueSentOnAllFaces( 0 ) ;

    
  }
        
}
