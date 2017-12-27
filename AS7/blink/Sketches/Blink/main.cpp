/*
 * Blinks - One way IR Tester
 * 
 * TLDR;
 * 1. Load on two tiles
 * 2. Put them next to each other
 * 3. Push the button on one to switch it to receive mode
 * 4. Green spinning means good received data. Red blink mead missed message. 
 *
 *
 * Starts in TX mode where it sends number 0-5 repeatedly. Shows number in BLUE on pixels. 
 * Button press switches to SYNC mode where listens for next number. Show YELLOW on pixels while waiting. 
 * On receiving a number in SYNC mode, switches to TX mode where it shows GREEN on pixel for match, or RED on tile for mismatch.
 *
 * Also prints some helpful statistics out the serial port in RX mode. 
 *
 */

#include "blinklib.h"

#include "Serial.h"

#define SENDSPEED_STEPS_PER_S   30

ServicePortSerial sp;

void setup() {

    setColor( BLUE );
    sp.begin();
    sp.println("IR Tester starting. Push button to enter RX mode."); 
       
}

uint32_t nextSendTime=0;

uint32_t rxModeStartTime;       // Used for printing time deltas to the serial port in RX mode
uint32_t rxModeNextStatusTime;      
uint32_t rxModeGoodCount;
uint32_t rxModeErrorCount;


#define RX_MODE_STATUS_INTERVAL_S  10      // Print a status line this often 

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
            
            // Start sending now
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
            
            nextSendTime = now +  ( MILLIS_PER_SECOND / SENDSPEED_STEPS_PER_S );
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
                    
                    sp.println( "Starting RX mode...");
                    
                    mode = RX;          // We are now SYCN'ed!
                    rxModeStartTime     =now;
                    rxModeErrorCount    =0;
                    rxModeGoodCount     =0;
                    rxModeNextStatusTime=0;
                        
                }                    
                    
                
                if (data == newData ) {
                    
                    /*
                    sp.print( " Match: ");
                    sp.print( newData );
                    sp.println();
                    */
                    setFaceColor( data  , GREEN );   // Match
                    
                    rxModeGoodCount++;
                    
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
                    
                    rxModeErrorCount++;
                    
                }                                        
                
                
                // Advance to next expected value 
                                    
                data++;                    
                data = data %  FACE_COUNT ;
                                    
                setFaceColor( data % FACE_COUNT , OFF);   // Turn off (otherwise we could not see anything when it was working well)
                
                if (rxModeNextStatusTime <= now ) {
                    
                    // Print current status out serial port
                    sp.print(" Secs:");
                    sp.print( (now-rxModeStartTime)/MILLIS_PER_SECOND );
                    sp.print(" Good:");
                    sp.print( rxModeGoodCount );
                    sp.print(" Errors:");
                    sp.print( rxModeErrorCount );
                    sp.println();
                    
                    rxModeNextStatusTime = now + (RX_MODE_STATUS_INTERVAL_S * MILLIS_PER_SECOND );
                    
                }                    
                    
            }     // IsDataReady(f);
            
        }    // FOREACH_FACE(f)
        
    }  // mode == RX
        

}
