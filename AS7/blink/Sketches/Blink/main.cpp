#include "Arduino.h"

#include "blinklib.h"

#include "ir.h"

#include "utils.h"

#include "util/delay.h"

#include "debug.h"

void setup() {

    // No setup needed for this simple example!

    for( int b=0; b<31; b++ ) {
        setColor( dim( GREEN , b ) );
        delay(100);
    }

    setColor(OFF);
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
            _delay_ms(20);
                
            break;

        case SYNC:

            setColor( YELLOW );
            
            FOREACH_FACE(x) {
                
                // Sync to the first face we see data on
                
                if (irIsReadyOnFace(x)) {
                    
                    count = irGetData(x);             
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
                    
                    mode = ERROR; 
                    
                }                    
                
              

                //count = irGetData(rxFace);
                //setFaceColor( count % FACE_COUNT , GREEN );       // Turn off previous pixel
                
                


            }
        
            break;

        case ERROR:

            setColor(RED);

            // Do nothing in error mode - just show solid red
            break;

    }
    
}

