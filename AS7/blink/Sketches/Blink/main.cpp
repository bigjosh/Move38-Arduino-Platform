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


// The color we show on an occupied face in READY state 
static Color readyFaceColor = RED; //MAKECOLOR_RGB( 10, 8 , 0 );

void setup() {
  // put your setup code here, to run once:
  //readyFaceColor = makeColorHSB( 25, 255, 20);     // A nice smoldering orange  
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
    

Color colors[] = {  GREEN, RED , BLUE };

uint8_t colorIndex=0;

uint8_t pixelIndex =0; 

uint32_t nextTime=0;

void loop() {

    if (millis()> nextTime) {
                
        if (pixelIndex==FACE_COUNT) {
            
            setColor( colors[colorIndex] );
            
            colorIndex++;
            
            if (colorIndex==COUNT_OF( colors )) {
                colorIndex=0;
            }
            
            pixelIndex=0;
            
        } else {
            
            setColor( OFF);
            setFaceColor( pixelIndex , colors[colorIndex] );
            
            pixelIndex++;            
            
        }                                        
        
        nextTime = millis() + 200; 
        
    }        

    
}


void loop2() {
        
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
            
        } else if (state==BURN) {              // Technically don't need this if since this is the only possible case, but here for clarity.             
            
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
            setFaceColor(f, RED );
        }
          
    }
       
    // If there is an explosion in progress, blink a random face 
    // will draw over the orange set above. 
    
    if (state == IGNITE || state == BURN ) {         
        // Blink a random face white
        setFaceColor( rand( FACE_COUNT -1 ) , BLUE );            
    }        
    
    // Finally we set out outputs
        
    // We send out current state on all faces EXCEPT for the special
    // case when we are INFECTing, in which case we want to only send that 
    // on the target face and look READY to everyone else. 
    
    setState( state ); 
    
    if (state==BURN) {
        
        // Burn state is special since we only sent the burn to the target neighbor
        // or else all our neighbors would catch on fire


        // This is a bit of a hack. We can't send our current state to everyone because then
        // we would infect everyone. So we send READY because it is benign.        
        setState( READY ); 
        
        if (targetFace != NO_FACE ) {
                   
            // Show the spark flying away
            setFaceColor( targetFace , BLUE );            
                                       
            // We do explicitly send BURN to the target we are aiming to infect.
            setState( state , targetFace );
            
        } else {
            
            // If we are READY then everyone will see
            // If we are IGNITE, then the neighbor who sent us the spark will see that it worked
            
            setState( state ); 
            
        }            
        
    }        
            
}


