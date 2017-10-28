/*
 * blinkstate.h
 *
 * This defines a statefull view of the blinks tile interactions with neighbors.
 *
 * In this view, each tile has a "state" that is represented by a number between 1 and 127.
 * This state value is continuously broadcast on all of its faces.
 * Each tile also remembers the most recently received state value from he neighbor on each of its faces. 
 * 
 * Note that this library depends on the blinklib library for communications with neighbors. The blinklib
 * IR read functions are not available when using the blinkstate library. 
 *
 * Note that the beacon transmissions only occur when the loop() function returns, so it is important
 * that sketches using this model return from loop() frequently.
 *
 */ 

#ifndef BLINKSTATE_H_
#define BLINKSTATE_H_

#ifndef BLINKCORE_H_
    #error You must #include blinkcore.h before blinkstate.h
#endif    

// Returns the last received state of the indicated face, or
// 0 if no messages received recently on indicated face

byte getNeighborState( byte face );

// Returns true if we have recently received a valid message from a neighbor
// on the indicated face

// Set our state to newState. This state is repeatedly broadcast to any
// neighboring tiles. 

// Note that setting our state to 0 make us stop broadcasting and effectively 
// disappear from the view of neighboring tiles. 

// By default we power up in state 0. 

void setState( byte newState );


// The blinkstate file needs to be able to call the blinklib IR functions, but we need to cover them
// for anyone else. We could have two copies of blnkistate.h (one for blinkstate's exclusive use) 
// but then they could get out of sync and that is ugly. 
// An clean way would be to use __BASE_FILE__..... but Arduino doesn't give us that. 
// Instead we use this canary which is #defined in blinkstate.cpp

#ifndef BLINKSTATE_CANNARY

    // We need to hide the direct IR functions or else they might consume IR events that we need to read
    // This is kind of hackish, but can you think of a better way?
    
    // It gets worse, Arduino will not less us use an #error inside the #define, so we must resort to this ugly __force_error() shenanegans
    // At least the user gets to see the error string.... :/

    #define irIsReadyOnFace(face)       __force_error("The irIsReadyOnFace() function is not available while using the blinkstate library")

    #define irGetData(led)            __force_error("The irGetData() function is not available while using the blinkstate library")

    #define irGetErrorBits(face)      __force_error("The irGetErrorBits() function is not available while using the blinkstate library")

#endif


// Manually add our hooks

void blinkStateBegin(void);


#endif /* BLINKSTATE_H_ */