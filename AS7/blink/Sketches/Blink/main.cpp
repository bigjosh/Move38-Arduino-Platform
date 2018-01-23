/*
    Happy New Years - 2018

    Firecracker

    Rules:
    If button pressed...
    explode with firecracker like flashing of lights that spreads to neighboring tiles
        
    if receive spark, explode like firecracker and  remember who I received spark from
    
    after an initial ignition time, pick one of my other neighbors to send the spark to.
    we will not send a spark back to the same place we got it from. 
    if no suitable neighbors available, firecracker is extinguished.

*/

#include "blinklib.h"
#include "blinkstate.h"

// Our local state. We also reuse these values as messages to send to neighbors

enum state_t { 
    READY ,         // We will explode if we see an INFECT message on any face. 
    IGNITE ,        // Running the explosion pattern. soruceFace is the face we got SPARK from.
    BURN,           // Spreading to any neighbor besides sourceFace.    
};

static state_t state = READY; 

#define NO_FACE FACE_COUNT     // Use FACE_COUNT as a match-none special value for sourceFace
                               // We use this when the explosion was manually started so no
                               // face is the source. 


static int sourceFace=NO_FACE; // The face we got the spark from.
                               // Only valid in EXPLODING, COOLDOWN, and INFECT
                               // NO_FACE if we did not get the spark from anywhere (was cause by button press)

static int targetFace=NO_FACE; // The face we are sending the spark to.
                               // Only matters in BURN state. NO_FACE if we are not spreading

static uint32_t nextStateTime;        // Time we switch to next state. Valid in EXPLODING, COOLDOWN, and INFECT.

// How long between when we first get a spark and start sending a new spark
static const uint16_t igniteDurration_ms = 200;

// Time we will continue to burn after we start sparking
static const uint16_t burnDuration_ms = 400;


// Show on an occupied face in READY state 
static Color readyFaceColor = makeColorRGB( 10*8,  8*8 , 0 );

// Show on the face that we are sending the spark to 
static Color sparkFaceColor = makeColorRGB( 20*8 , 0 , 0 );

// Show on random faces as we ignite and burn
static Color sparkleColor   = makeColorRGB( 20*8 , 20*8 , 20*8 );

void setup() {
  blinkStateBegin();
}

// Returns a face that (1) has not yet expired, and (2) is not `exclude`
// Returns NO_FACE is no faces meet the criteria

static byte pickSparkTarget( byte exclude ) {
    
    // First tally up all the potential target faces
                
    byte potentialTargetList[FACE_COUNT];                 
    byte potentialTargetCount=0;
                
    FOREACH_FACE(f) {
        
        if ( (!isNeighborExpired(f)) && (f!=exclude) ) {
            
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
  
    bool detonateFlag = false;        // Set this flag to true to cause detonation
                                      // since the ignition can come from either button or spark
                                      // and we don't care which
  
    // First get some situational awareness
    
  
    FOREACH_FACE(f) {
        
        
        if (neighborStateChanged(f)) {

            byte receivedMessage = getNeighborState(f);    
            
            if (receivedMessage==BURN) {      // We just got a spark!
                
                detonateFlag=true;
                sourceFace=f;
                
            } else if (receivedMessage==IGNITE) {
                
                // Looks like the target we were trying to light did catch fire!
                
                // No need to keep sending to them
                
                targetFace=NO_FACE;
                
            }                
                                            
        }                          
    }      
    
    
    // Next update our state based on new info 
        
    // if button pressed, firecracker
    if (buttonSingleClicked()) {
        detonateFlag=true;
        sourceFace = NO_FACE;           // Manually initiated, so no source                
    }
              
  
    if (detonateFlag) {
        state=IGNITE;
        nextStateTime=now+igniteDurration_ms;
    }      
      
    if ( nextStateTime < now ) {        // Time for next timed state transition?
        
        // These are the only states that can timeout
        
        if (state==IGNITE) {
            
            // We are ready to spread the flame!
            
            // Try to pick one. Will return NO_FACE if none possible.
            
            targetFace = pickSparkTarget( sourceFace );
            
            state=BURN;
            nextStateTime=now+burnDuration_ms;
            
        } else if (state==BURN) {              // Technically don't need this `if` since this is the only possible case, but here for clarity.             
            
            state=READY;
            nextStateTime=NEVER;                 
            
        }                
                
    }            
  
  
    // In all states, we show orange on any face that has a neighbor
    // (may be covered by effect if we are EXPLODING) 
    
    FOREACH_FACE(f) {
          
        if (!isNeighborExpired(f)) {
            setFaceColor(f, readyFaceColor );
        } else {
            setFaceColor(f, OFF );
        }
          
    }
       
    // If there is an explosion in progress, blink a random face 
    // will draw over the orange set above. 
    
    if (state == IGNITE || state == BURN ) {         
        // Blink a random face white
        setFaceColor( rand( FACE_COUNT -1 ) , sparkleColor );            
    }        
    
    // Finally we set out outputs
        
    // We send out current state on all faces EXCEPT for the special
    // case when we are INFECTing, in which case we want to only send that 
    // on the target face and look READY to everyone else. 
        
    if (state==BURN) {
        
        // Burn state is special since we only send the burn to the target neighbor
        // or else all our neighbors would catch on fire


        // This is a bit of a hack. We can't send our current state to everyone because then
        // we would infect everyone. So we send READY because it is benign.        
        setState( READY ); 
        
        if (targetFace != NO_FACE ) {
                   
            // Show the spark flying away
            setFaceColor( targetFace , sparkFaceColor );            
                                       
            // We do explicitly send BURN to the target we are aiming to infect.
            setState( state , targetFace );
            
        }  
        
    } else {
            
        // For any other state besides BURN, we can just send our real state to everyone
        // Note that when we send IGNITE, the person who sent us burn will see it and
        // know that we ignited and will stop sending BURN out anymore. 
            
        setState( state );
    }        
                                       
}


