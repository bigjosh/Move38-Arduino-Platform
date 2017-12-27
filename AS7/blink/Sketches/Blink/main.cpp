/*
 * Blinks - IR Tester
 * 
 * Starts in TX mode where it sends number 0-5 repeatedly. Shows number in BLUE on pixels. 
 * Button press switches to SYNC mode where listens for next number. Show YELLOW on pixels while waiting. 
 * On receiving a number in SYNC mode, switches to TX mode where it shows GREEN on pixel for match, or RED on tile for mismatch.
 *
 */

#include "blinklib.h"

#include "utils.h"
#include "Serial.h"

#include <stdint.h>         // UINT8_MAX

ServicePortSerial sp;

void setup() {

    setColor( BLUE );
    sp.begin();
       
}

uint32_t nextSendTime=0;

uint8_t data =0; 
    
enum { TX , SYNC , RX } mode = TX;
   
void loop() {
    
    uint32_t now  = millis();
    
    if (buttonPressed()) {
        
        if (mode==TX) {
            
            setColor( YELLOW );     
            mode = SYNC;
            
        } else {
            
            mode=TX;
            setColor( OFF );
            
            nextSendTime = now;            
            
        }                        
    }            
            
        
    
    if (mode==TX) {
                       
        if ( now > nextSendTime ) {
                
            setFaceColor(  data , OFF ) ;       // Turn off previous pixel               
        
            data++;
                
            data = data % FACE_COUNT;  
                
            irSendData(  data , 0b00111111 );
        
            setFaceColor( data , BLUE );
            
            nextSendTime = now + 100;
        }        
                        
            
    } else {        // mode == RX
                    
        
        FOREACH_FACE(f) {
            
            if (irIsReadyOnFace(f)) {
                
                uint8_t newData = irGetData(f);
                
                if (mode==SYNC) {
                    
                    
                    /*
                    sp.print( " Sync'ed: ");
                    sp.print( newData );
                    sp.println();                    
                    */
                    
                    data=newData;
                    
                    mode = RX;          // We are now SYCN'ed!
                        
                }                    
                    
                
                if (data == newData ) {
                    
                    /*
                    sp.print( " Match: ");
                    sp.print( newData );
                    sp.println();
                    */
                    setFaceColor( data  , GREEN );   // Match
                    
                } else {
                    
                    /*
                    sp.print( "Expected: ");
                    sp.print( data );
                    sp.print( " Got: ");
                    sp.print( newData );
                    sp.println();
                    */
                    
                    sp.print( (char) data);
                    sp.print( (char) newData);
                    
                    setColor( RED );    // No match
                    data = newData;     // Reset 
                    
                }                                        
                
                
                // Advance to next expected value 
                                    
                data++;                    
                data = data %  FACE_COUNT ;
                                    
                setFaceColor( data % FACE_COUNT , OFF);   // Turn off (otherwise we could not see anything when it was working well)
                    
            }     // IsDataReady(f);
            
        }    // FOREACH_FACE(f)
        
    }  // mode == RX
        

}
