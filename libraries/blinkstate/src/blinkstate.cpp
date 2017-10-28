/*
 *
 *   This library presents a higher level abstraction of state and communications and sits on top of the blinklib IR functions. 
 *
 * In this view, each tile has a "state" that is represented by a number between 1 and 127.
 * This state value is continuously broadcast on all of its faces.
 * Each tile also remembers the most recently received state value from he neighbor on each of its faces. 
 * 
 * This library takes over the blinklib IR read functions since it needs to consume the events they produce. 
 * 
 * Note that the beacon transmissions only occur when the loop() function returns, so it is important
 * that sketches using this model return from loop() frequently. This is modeled on the idomatic Arduino 
 * way of handling periodic callbacks...
 * https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/cores/arduino/main.cpp#L47
 * 
 */     

#include <avr/pgmspace.h>
#include <limits.h>
#include <stdint.h>

#define DEBUG_MODE

#include <stddef.h>

#include "blinklib.h"
#include "chainfunction.h"

#define STATE_BROADCAST_RATE_MS 100             // Minimum number of milliseconds between broadcasting the same state again

#define STATE_EXIRE_TIME_MS     250              // If we have not heard anything on this this face in this long, then assume no neighbor there




static unsigned long expireTime[FACE_COUNT];     // time when last received state will expire

// check and see if any states recently updated....

static void readStates(void) {
    
    FOREACH_FACE(x) {
        
        if ( irIsReadyOnFace(x) ) {
            
            // We could cache this calculation, but for now this is simpler.
            
            expireTime[x] = millis() + STATE_EXIRE_TIME_MS;
            
            irGetData(x);       // We read and dump just to clear the ready flag.
            
        }
        
    }
    
}

static byte localState=0;
static byte localStateNextSendTime;

// Broadcast our local state

static void broadcastState(void) {
    
    irBroadcastData( localState );
    localStateNextSendTime = millis() + STATE_BROADCAST_RATE_MS;
    
}


// Called one per loop() to check for new data and repeat broadcast if it is time
// Note that if this is not called frequently then neighbors can appear to still be there
// even if they have been gone longer than the time out, and the refresh broadcasts will not
// go out often enough.

// TODO: All these calls to millis() and subsequent calculations are expensive. Cache stuff and reuse. 

void blinkStateOnLoop(void) {

    // Check for anything coming in...
    readStates();
    // Check for anything going out...
    
    if ( localStateNextSendTime <= millis() ) {         // Time for next broadcast?
        
        broadcastState(); 
        
    }         
    
}

// Make a record to add to the callback chain 

Chainfunction blinkStateOnLoopChain( blinkStateOnLoopChain );

// Something tricky here:  I can not good place to automatically add
// our onLoop() hook at compile time, and we
// don't want to follow idiomatic Arduino ".begin()" pattern, so we
// hack it by adding here the first time anything trhat could use state
// stuff is called. This is an ugly hack. :/

// TODO: This is a good place for a GPIO register bit. Then we could inline the test to a single instruction.,

static uint8_t hookRegisteredFlag=0;        // Did we already register?

static void registerHook(void) {
    if (!hookRegisteredFlag) {
        addOnLoop( &blinkStateOnLoopChain ); 
        hookRegisteredFlag=1;
    }        
}    


// Returns the last received state of the indicated face, or
// 0 if no messages received recently on indicated face

byte getNeighborState( byte face ) {
    
    registerHook();     // Check everytime. Maybe a begin() better?
        
    if (irIsReadyOnFace(face)) {
            
        // New data came in since we last readStates(), so need to update the expire time...            
        expireTime[face] = millis() + STATE_EXIRE_TIME_MS;
                        
        // This will clear the ready flag in the case where new data just came in.
        
        return irGetData(face);

    }  else {
        
        if ( expireTime[face] <= millis() ) { 
                                                       
            // We use the fact that irGetData() always returns the *last* recieved data
            // to avoid needing o store a copy ourselves. 
               
            return irGetData(face);
        
        }  else {
                
            // Face expired
            return 0;
        
        }        
        
    }        
                    
    
}    




// Set our state to newState. This state is repeatedly broadcast to any
// neighboring tiles.

// Note that setting our state to 0 make us stop broadcasting and effectively
// disappear from the view of neighboring tiles.

// By default we power up in state 0.


void setState( byte newState ) {

    registerHook();     // Check everytime. Maybe a begin() better?
    
    if ( newState != localState ) {
    
        localState = newState;
    
        broadcastState();            // Grease the wheels and send immmedeately if needed rather than waiting for next onLoop()
    
    }    
    
}    

