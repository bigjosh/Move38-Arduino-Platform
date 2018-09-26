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

#ifndef BLINKLIB_H_
    #error You must #include blinklib.h before blinkstate.h
#endif


// Manually add our hooks.
// Must be called before using any other blinkstate functions
// TODO: Now that blinkstate is the primary game-level API, maybe make this the default?

void blinkStateBegin(void);


// Returns the last received state on the indicated face
// returns 0 if no neighbor ever seen on this face since power-up
// so best to only use after checking if face is not expired first.

byte getLastValueReceivedOnFace( byte face );


// Did the neighborState value on this face change since the
// last time we checked?

// Note the a face expiring has no effect on the last value

byte didValueOnFaceChange( byte face );

// false if messages have been recently received on the indicated face
// (currently configured to 100ms timeout in `expireDurration_ms` )

byte isValueReceivedOnFaceExpired( byte face );

// Returns false if their has been a neighbor seen recently on any face, returns true otherwise.
bool isAlone();

// Set value that will be continuously broadcast on all face.
// By default we power up with all faces sending the value 0.

void setValueSentOnAllFaces( byte newState );

void setValueSentOnFace( byte value , byte face );


#ifndef BLINKSTATE_CANNARY

    // We need to hide the direct IR functions or else they might consume IR events that we need to read
    // This is kind of hackish, but can you think of a better way?

    // It gets worse, Arduino will not less us use an #error inside the #define, so we must resort to this ugly __force_error() shenanegans
    // At least the user gets to see the error string.... :/

    #define irIsReadyOnFace(face)       __force_error("The irIsReadyOnFace() function is not available while using the blinkstate library")

    #define irGetData(led)            __force_error("The irGetData() function is not available while using the blinkstate library")

    #define irGetErrorBits(face)      __force_error("The irGetErrorBits() function is not available while using the blinkstate library")

#endif



#endif /* BLINKSTATE_H_ */
