/*
    Happy New Years - 2018

    Firecracker

    Rules:
    If button pressed...
    explode with firecracker like flashing of lights
    when done, pick one of my neighbors to send the spark to
    if receive spark, explode like firecracker
    remember who I received it from
    when done, pick one of my other neighbors to send the spark to
    if no other neighbors available, firecracker is extinguished

*/

#include "blinklib.h"
#include "blinkstate

// Our local state. We also reuse these values as messages to send to neighbors

enum state_t { 
    READY ,         // We will explode if we see an INFECT message on any face. 
    EXPLODING ,     // Running the explosion pattern. soruceFace is the face we got INFECTED from.
    COOLDOWN,       // Just a delay for visual effect. soruceFace is still the face we got INFECTED from.
    INFECT          // Trying to send an INFECT message to any neighbor besides sourceFace. 
};

static state_t state = READY; 

#define NO_FACE FACE_COUNT     // Use FACE_COUNT as a match-none special value for sourceFace
                               // We use this when the explosion was manually started so no
                               // face is the source. 


static int sourceFace=NO_FACE; // The face we got the spark from.
                               // Only valid in EXPLODING, COOLDOWN, and INFECT
                               // NO_FACE if we did not get the spark from anywhere (was cause by button press)

static int targetFace=NO_FACE;        // The face we are sending the spark to.
                               // Only valid in INFECT

static uint32_t nextStateTime;        // Time we switch to next state. Valid in EXPLODING, COOLDOWN, and INFECT.

// How long we show the effect once we get a spark    
static const uint16_t explosionDurration_ms = 400;    

// How long after the effect ends before we send the spark out to a neighbor
static const uint16_t cooldownDurration_ms = 100;

// How long we wait for a neighbor to ack our spark before giving up
static const uint16_t infectTimeout_ms = 200;

// The color we show on an occupied face in READY state 
static Color readyFaceColor;

#include "Serial.h"

void setup() {
  // put your setup code here, to run once:
  readyFaceColor = makeColorHSB( 25, 255, 128);     // A nice smoldering orange  
}

// Returns a face that (1) has not yet expired, and (2) is not `exclude`
// Returns NO_FACE is no faces meet the criteria

static byte pickSparkTarget( uint32_t now, byte exclude ) {
    
    // First tally up all the potential target faces
                
    byte potentialTargetList[FACE_COUNT];                 
    byte potentialTargetCount=0;
                
    FOREACH_FACE(f) {
        
        if ( () && (f!=exclude) ) {


            potentialTargetList[ potentialTargetCount ] = f;
            potentialTargetCount++;
        }
        
    }
    
    if ( potentialTargetCount==0) {
        // No place to send
        return NO_FACE;
    }        
        
    // Pick which of the potential nodes is the winner...
                    
    byte targetIndex = rand( potentialTargetCount-1 );
    
    return potentialTargetList[ targetIndex ];
    
}    
    

void loop() {
        
    // put your main code here, to run repeatedly:
    uint32_t now = millis();
  
    bool detonateFlag = false;        // Set this to true to cause detonation
  
    // First get some situational awareness
  
    FOREACH_FACE(f) {
        
        if (irIsReadyOnFace(f)) {
        
            // Got something, so we know there is someone out there
            neighboorExpireTime[f] = now + expireDurration_ms;
        
            // Clear to send on this face immediately to ping-pong messages at max speed without collisions
            neighboorSendTime[f] = 0;
                
            byte receivedMessage = irGetData(f);    
                   
            if (receivedMessage==INFECT) {      // We just got a spark!
                
                detonateFlag=true;
                sourceFace=f;
                
            } else if (receivedMessage == EXPLODING && state==INFECT && f==targetFace) {
                
                // They got our INFECT message!
                state=READY;            
                
            }                        
        }                          
    }      
        
    // if button pressed, firecracker
    if (buttonSingleClicked()) {
        detonateFlag=true;
        sourceFace = NO_FACE;           // Manually initiated, so no source                
    }
              
  
    if (detonateFlag) {
        state=EXPLODING;
        nextStateTime=now+explosionDurration_ms;
    }      
  
  
    // In all states, we show orange on any face that has a neighbor
    // (may be covered by effect if we are EXPLODING) 
    
    FOREACH_FACE(f) {
          
        if (neighboorExpireTime[f]>now) {
            setFaceColor(f, readyFaceColor );
        } else {
            setFaceColor(f,OFF);
        }
          
    }
       
    // Note that we do not explicitly check for READY since in this state
    // we don't really do anything (the original background that we already drew shows)
    
    if (state == EXPLODING ) {         
        
        // While exploding we show the fireworks!            
          
        if (now < nextStateTime ) {        // Still exploding
            
            // Blink a random face white
            setFaceColor( rand( FACE_COUNT -1 ) , WHITE );
            
        } else {
            
            // Show's over folks!
            state=COOLDOWN;
            nextStateTime=now+cooldownDurration_ms;            
            
        }                              
    }        
    
    // Note that we concatenate our if's here so that we do the right thing 
    // after a state change.  Assumes state if's are listed in chronological order
    
    if (state == COOLDOWN ) { 
        
        // We don't show anything in cool down
            
        if (now >= nextStateTime ) {        // Done cooling down, so time to infect!
        
            targetFace = pickSparkTarget( now , sourceFace );
        
            if ( targetFace != NO_FACE ) { 
                // We found a target                                    
                state=INFECT;
                nextStateTime=now+cooldownDurration_ms;                
            } else {
                // No suitable targets, fizzle out
                state=READY;
            }                        
        
        }
    }        
    
    if (state==INFECT) {
               
        if (now < nextStateTime ) {
            
            // Show the spark flying away
            setFaceColor( targetFace , BLUE );            
            
        } else {
            
            // Give up trying to infect. The target is not acking us
            state = READY;         
            
        }            
        
    }        
    
    // Send out messages to our neighbors

    FOREACH_FACE(f) {
        
        if ( neighboorSendTime[f] <= now ) {        // Time to send on this face?
                    
            if (state==INFECT && f != targetFace ) {
                
                // This is a bit of a hack. We can't send our current state because then
                // we would infect everyone. So we send READY because it is benign.
                irSendData( f , READY );
                                    
            } else {        // In any state but INFECT
                
                irSendData( f , state );
                                
            }
            
            // Here we set a timeout to keep periodically probing on this face, but
            // if there is a neighbor, they will send back to us as soon as they get what we
            // just transmitted, which will make us immediately send again. So the only case 
            // when this probe timeout will happen is if there is no neighbor there. 
            
            neighboorSendTime[f] = now + sendprobeDurration_ms;
                        
        }                            
    }            
        
}


