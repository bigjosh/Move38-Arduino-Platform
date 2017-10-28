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


// We use this trickery to let the blinkstate library see the protoypes for its own functions
// and the IR functions from blinklib, but hide the IR functions when a user includes 
// this header file.
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html

#if "__BASE_FILE__" != "blinkstate.h"

    // We need to hide the direct IR functions or else they might consume IR events that we need to read
    // This is kind of hackish, but can you think of a better way?

    #define irIsReadyOnFace(face)       #error The irIsReadyOnFace() function is not available while using the blinkstate library 

    #define irGetData(led)              #error The irGetData() function is not available while using the blinkstate library 

    #define irGetErrorBits(face)        #error The irGetErrorBits() function is not available while using the blinkstate library 

#endif

#endif /* BLINKSTATE_H_ */