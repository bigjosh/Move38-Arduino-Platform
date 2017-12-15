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
#include <stdlib.h>         //rand()

#define DEBUG_MODE

#include <stddef.h>

#include "blinklib.h"

#include "chainfunction.h"

// Tell blinkstate.h to save the IR functions just for us...

#define BLINKSTATE_CANNARY

#include "blinkstate.h"

#define STATE_BROADCAST_RATE_MS  80-32           // Minimum number of milliseconds between broadcasting the same state again. The 32 is to offset the 32 steps of random jitter we add.

#define STATE_EXPIRE_TIME_MS     250             // If we have not heard anything on this this face in this long, then assume no neighbor there


// TODO: The compiler hates these arrays. Maybe use struct so it can do indirect offsets?

static byte lastValue[FACE_COUNT];               // Last received value
static unsigned long expireTime[FACE_COUNT];     // time when last received state will expire


static void updateRecievedState( uint8_t face ) {

    if ( irIsReadyOnFace(face) ) {
            
        lastValue[face] = irGetData(face);       
            
        // We could cache this calculation, but for now this is simpler.
            
        expireTime[face] = millis() + STATE_EXPIRE_TIME_MS;
            
    } 
    
}    


// check and see if any states recently updated....

static void updateRecievedStates(void) {
    
    FOREACH_FACE(x) {
        
        updateRecievedState( x );
        
    }
    
}

static byte localState=0;
static unsigned long localStateNextSendTime;

// Broadcast our local state

static void broadcastState(void) {
    
    irBroadcastData( localState );
    localStateNextSendTime = millis() + STATE_BROADCAST_RATE_MS + ( ( rand() & 15 ) * 2) ;
    
}


// Called one per loop() to check for new data and repeat broadcast if it is time
// Note that if this is not called frequently then neighbors can appear to still be there
// even if they have been gone longer than the time out, and the refresh broadcasts will not
// go out often enough.

// TODO: All these calls to millis() and subsequent calculations are expensive. Cache stuff and reuse. 

void blinkStateOnLoop(void) {

    // Check for anything coming in...
    updateRecievedStates();
    
    
    // Check for anything going out...
    
    if ( (localState!=0) && (localStateNextSendTime <= millis()) ) {         // Anything to send (state 0=don't send)? Time for next broadcast? 
        
        broadcastState(); 
        
    }         
    
}

// Make a record to add to the callback chain 

static struct chainfunction_struct blinkStateOnLoopChain = {
     .callback = blinkStateOnLoop,
     .next     = NULL                  // This is a waste because it gets overwritten, but no way to make this un-initialized in C
};

// Something tricky here:  I can not find a good place to automatically add
// our onLoop() hook at compile time, and we
// don't want to follow idiomatic Arduino ".begin()" pattern, so we
// hack it by adding here the first time anything that could use state
// stuff is called. This is an ugly hack. :/

// TODO: This is a good place for a GPIO register bit. Then we could inline the test to a single instruction.,

static uint8_t hookRegisteredFlag=0;        // Did we already register?

static void registerHook(void) {
    if (!hookRegisteredFlag) {
        addOnLoop( &blinkStateOnLoopChain ); 
        hookRegisteredFlag=1;
    }        
}    

// Manually add our hooks

void blinkStateBegin(void) {
    registerHook();
}    



// Returns the last received state of the indicated face, or
// 0 if no messages received recently on indicated face

byte getNeighborState( byte face ) {
    
    registerHook();     // Check everytime. Maybe a begin() better?
    
    updateRecievedState( face ) ;       // Refresh
               
    if ( expireTime[face] > millis() ) {        // Expire time in the future?
                                                                      
        return lastValue[ face ]; 
        
    }  else {
                
        // Face expired
        return 0;
        
    }        
        
    
}    




// Set our state to newState. This state is repeatedly broadcast to any
// neighboring tiles.

// Note that setting our state to 0 make us stop broadcasting and effectively
// disappear from the view of neighboring tiles.

// By default we power up in state 0.


void setState( byte newState ) {

    registerHook();     // Check everytime. Maybe a begin() better?
    
    if ( newState != localState ) {     // Ignore redundant setting to same state (
    
        localState = newState;
    
        broadcastState();            // Grease the wheels and send immmedeately if needed rather than waiting for next onLoop()
    
    }    
    
}    

