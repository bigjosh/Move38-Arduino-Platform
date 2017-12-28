/*
 * Blinks - One way, point to point IR Tester
 * 
 * TLDR;
 * 1. Load on two tiles
 * 2. Put them next to each other
 * 3. Push the button on one tile to switch it to receive mode
 *
 * Blue spinning pixel - sending
 * Green spinning pixel - reading valid messages in order
 * Yellow flash - Got a valid message, but out of expected order due to missed message(s)
 * Red flash - got an invalid message.
 *
 * Also prints some helpful statistics out the serial port in receive mode. 
 *
 * Note that since the sequence number only runs form 0-5, if you miss an even multipule
 * of 6 messages in a row then this will not be detected.
 *
 * Also note that since the error detection depends on comparing upper bits with the 
 * inverse of the lower bit, an mulit-bit error that the same bit in each zone would not 
 * be detected, but IO think this is very very unlikely IRL. 
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

/*
* We send the numbers 0-5 in order repeatedly. The numbers are encoded so that
* bits 0-2 are the actual number and bits 3-5 are the inverse. This helps us tell
* the difference between an invalid message, or if we just missed a message in the sequence.
*/

uint8_t encode( uint8_t data ) {
    return data | ( (data << 3) ^ 0b00111000 ) ;
}    

uint8_t decode( uint8_t data ) {
    return data & 0b0000111;
}

uint8_t isValid( uint8_t data ) {
    return decode( data ) ==  ( ( data ^ 0b00111000) >> 3 )  ;
}        

uint32_t nextSendTime=0;

uint32_t rxModeStartTime;       // Used for printing time deltas to the serial port in RX mode
uint32_t rxModeNextStatusTime;      
uint32_t rxModeGoodCount;
uint32_t rxModeMissedCount;
uint32_t rxModeErrorCount;


#define RX_MODE_STATUS_INTERVAL_S  10      // Print a status line this often 

uint8_t seq =0;                         // Counts 0-5 and then resets back to 0
    
enum { TX , SYNC , RX } mode = TX;
   
void loop() {
    
    uint32_t now  = millis();
    
    if (buttonPressed()) {
        
        if (mode==TX) {
            
            setColor( YELLOW );     
            mode = SYNC;                    // Start looking for initial seq

            // Clear counters            
            rxModeStartTime     =now;
            rxModeErrorCount    =0;
            rxModeGoodCount     =0;
            rxModeMissedCount   =0;
            rxModeNextStatusTime=0;
            
            
        } else {
            
            mode=TX;
            
            setColor( OFF );
            
            // Start sending now
            nextSendTime = now;            
            seq=0;
            
        }                        
    }            
                
    if (mode==TX) {
                       
        if ( now > nextSendTime ) {
                
            setFaceColor(  seq , OFF ) ;       // Turn off previous pixel               
        
            seq++;
                
            seq = seq % FACE_COUNT;  
                
            irSendData(  encode(seq) , 0b00111111 );
        
            setFaceColor( seq , BLUE );
            
            nextSendTime = now +  ( MILLIS_PER_SECOND / SENDSPEED_STEPS_PER_S );
        }        
                        
            
    } else {        // mode == RX
                    
        
        FOREACH_FACE(f) {
            
            if (irIsReadyOnFace(f)) {
                
                uint8_t newData = irGetData(f);
                
                if ( isValid( newData) ) {
                    
                    uint8_t recSeq = decode( newData ); 
                    
                    rxModeGoodCount++;     // Got a valid message
                                                        
                    if (mode==SYNC) {
                                                            
                        seq= recSeq;
                        mode = RX;          // We are now SYCN'ed!                       
                                                                       
                    } else {
                        
                        if (seq == recSeq ) {      // Expected SEQ?
                            
                            setFaceColor( seq  , GREEN );   // Match
                            
                        } else {                   // Unexpected SEQ
                            
                            rxModeMissedCount++;
                        
                            seq= recSeq;
                        
                            setColor( YELLOW );    // Missed 
                        }                                                    
                    }                            
                                                                                            
                    // Advance to next expected value
                   
                    seq++;
                    seq = seq %  FACE_COUNT ;
                   
                    setFaceColor( seq % FACE_COUNT , OFF);   // Turn off (otherwise we could not see anything when it was working well)
                                                                     
                } else {        // invalid data 
                    
                    rxModeErrorCount++;
                    setColor( RED );    // Error
                    
                    // Resync
                    mode=SYNC;
                                        
                }                                    
                
            }   // irDataReady(f)             
            
        }       // FOREACH_FACE(f)     
                
                
       if (rxModeNextStatusTime <= now ) {
                    
                    // Print current status out serial port
                    sp.print(" Secs:");
                    sp.print( (now-rxModeStartTime)/MILLIS_PER_SECOND );
                    sp.print(" Good:");
                    sp.print( rxModeGoodCount );
                    sp.print(" Missed:");
                    sp.print( rxModeMissedCount );                    
                    sp.print(" Errors:");
                    sp.print( rxModeErrorCount );
                    sp.println();
                    
                    rxModeNextStatusTime = now + (RX_MODE_STATUS_INTERVAL_S * MILLIS_PER_SECOND );
                    
        }                    
                            
    }  // mode == RX        
}
