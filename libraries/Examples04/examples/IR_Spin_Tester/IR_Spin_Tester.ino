/*

    IR teser
    
    Test for errors on IR link by sending a pattern and then checking for missed steps on the receiver.
    
    On startup, we are in SEND MODE. We broadcast the pattern on all faces. Each step of the blue led
    indicates one step of the pattern. 
    
    Single click switches us to SYNC MODE. In sync mode we look for data on all faces. When we receive something,
    we then switch to RECEIVE MODE and start looking for the pattern on that face starting at the initial step we just received.
    All LEDs are yellow while we search.
    
    In RECEIVE MODE, we continue reading data off the face that we found in SYNC MODE and checking to make sure no
    steps in the pattern are skipped. As long as sequence is continuous, we indicate the pattern steps with a spinning green LED.
    If we get data that does not match the next expected sequence in the pattern, then we go in to ERROR mode. 
    
    In ERROR MODE, we display red leds to indicate what errro condicitons were detected...
    
        FACE   CONDITION
        ====   ============================================
          0    Missed step in sequence (always lit)
          1    Droppout (too long an idle  gap while reading in a frame - low signal strength or blocked path) 
          2    Noise (too many flashes detected or flashes in wrong places - strong ambient light)
          3    Overflow (too slow reading new data - a new frame came in before we read out the last one)
          4    Parity (received a frame that was valid in form, but the error checking bit was wrong)
          
    Single click exits ERROR MODE and goes back to SEND MODE.           


*/

#include "Arduino.h"

#include "blinklib.h"

//#include "ir.h"

//#include "utils.h"

//#include "util/delay.h"

//#include "debug.h"

void setup() {
    // This page intionally left blank
}

typedef enum { TX , RX, SYNC , ERROR } mode_t;

mode_t mode=TX;      // Are we transmitting or receiving?

uint8_t count=0;

uint8_t rxFace=0;       // Which face are we recieving from?

// Find the highest number we can got to that is 
// even multiple of 6 so we can MOD it to make a nice spin around
// the pixels for visual feedback. Just using numbers 0-5
// Would not be great for error check since then if you missed 5
// packets you would not notice it. 
// If we didn't use an even multiple of 6, then there would be 
// and ugly jump when we overflowed. 

const uint8_t topCount = ( (1<<7) / FACE_COUNT ) * 6;       
                                                
bool irLastValue( uint8_t led );

void loop() {
                       
    if( buttonSingleClicked() ) {
        
        // Click in TX goes to SYNC.
        // Click any other mode goes back to TX
        
        if (mode==TX) {
            mode=SYNC;
            
            // Clear out any left over receives and start looking fresh now
            FOREACH_FACE(x) {
                if (irIsReadyOnFace(x)) {
                    irGetData(x);
                }                                    
            }                
            setColor(OFF);  // Clear the last RED pixel. Neatness counts. 
        } else {
            mode=TX;
            setColor(OFF);  // Clear the last GREEN and maybe BLUE pixel. Neatness counts.            
        }
                
    }
    

    switch (mode) {

        case TX:
        
            setFaceColor( count % FACE_COUNT , OFF );       // Turn off previous pixel
        
            count++;                       
            
            if (count==topCount) {       // Roll over when we hit top (remember that topCount always even multiple of 6)
                count=0;
            }
            
            setFaceColor( count % FACE_COUNT , BLUE );        // Turn on next one
            
            // TODO: send all faces at once
            
            irBroadcastData( count );
            delay(20);                  // Slow down a bit so the spin is not just a blur
                
            break;

        case SYNC:

            setColor( YELLOW );
            
            FOREACH_FACE(x) {
                
                // Sync to the first face we see data on
                
                if (irIsReadyOnFace(x)) {
                    
                    count = irGetData(x);   
                    irGetErrorBits(x);     // Clear out any left over errors (only care about errros from when we synced)
                              
                    rxFace = x;
                    mode = RX;
                    setColor( OFF );        // Clean off yellow before we start to spin. 
                    break;                                   
                
                } 

                
            }                
            
            
            break;
        

        case RX:
                
            if (irIsReadyOnFace(rxFace)) {          // Wait for 1st message to come in
                
                setFaceColor( count % FACE_COUNT , OFF );       // Turn off previous pixel                
                
                count++;        // next number expected 
                
                if (count==topCount) {       // Roll over when we hit top (IR only transmits 7 bit)
                    count=0;
                }                    
                            
                if (count == irGetData(rxFace) ) {          // Match?
                    
                    uint8_t count_pixel= count % FACE_COUNT;
                               
                    setFaceColor( count_pixel , GREEN );       // Turn on next pixel
                    
                    if ( count_pixel != rxFace ) {
                        setFaceColor( rxFace , BLUE );          // Indicate which face we are synced to
                    }                        
                    
                } else { // No match - error
                    
                    setColor( OFF );        // Clear all pixels
                    
                    setFaceColor( 0 , RED );    // Show error with at least 1 red pixel
                    
                    uint8_t errorBits = irGetErrorBits(rxFace);
                    
                    if ( bitRead(errorBits , ERRORBIT_DROPOUT) ){
                        setFaceColor(1 , RED) ;
                    }                        

                    if ( bitRead(errorBits , ERRORBIT_NOISE  ) ){
                        setFaceColor(2 , RED) ;
                    }

                    if ( bitRead(errorBits , ERRORBIT_OVERFLOW) ) {
                        setFaceColor(3 , RED) ;
                    }

                    if ( bitRead(errorBits , ERRORBIT_PARITY  ) ) {
                        setFaceColor(4 , RED) ;
                    }
                    
                    mode = ERROR; 
                    
                }                    
                
              

                //count = irGetData(rxFace);
                //setFaceColor( count % FACE_COUNT , GREEN );       // Turn off previous pixel
                
                


            }
        
            break;

        case ERROR:

            // Do nothing in error mode - just keep showing the red error code
            break;

    }
    
}

