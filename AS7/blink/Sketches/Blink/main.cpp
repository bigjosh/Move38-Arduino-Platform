
#include "Serial.h"

// Our currently displayed colors and also the colors we send/receive in a packet to share

static Color colors[FACE_COUNT];


byte randomByte() {

    return random(255);

}

// Generate a random color

Color randomColor() {

    return makeColorHSB( randomByte() , 255 , 255 );

}


// Assign a random color to each face

void splat() {

    FOREACH_FACE(f){
        colors[f] = randomColor();
    }

}

// Display the currently programmed colors

void updateDisplayColors() {

    FOREACH_FACE(f) {
        setColorOnFace( colors[ f ] , f );
    }

}


void setup() {
    //splat();
    //updateDisplayColors();  
}

// Do we need to send?
boolean sendFlag;

void loop() {

  // Single button press resplats the colors

  if ( buttonDown() ) {

      splat();
      updateDisplayColors();
      
      sendFlag = true;
      
  } else {
      
      // We wait until the button is back up again to send
      // We use the flag to only send one time after the button comes up
      
      if (sendFlag) {      

          FOREACH_FACE(f) {

              if (!isValueReceivedOnFaceExpired(f)) {    // Do we have a neighbor here?

                  sendPacketOnFace( f , (byte *) colors , 2 * FACE_COUNT );

              }

          }
          
          sendFlag = false;
          
      }          

  }


  // Next check for any received packets

  FOREACH_FACE(f) {

      if ( isPacketReadyOnFace( f ) && getPacketLengthOnFace(f) == 2 * FACE_COUNT ) {

          Color *recievedColors = (Color *) getPacketDataOnFace( f ) ;
          
          FOREACH_FACE( f ) {
              
              colors[f] = recievedColors[f];
              
          }                        

          updateDisplayColors();

      }

  }

}
