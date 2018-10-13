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

// The value of the data sent and received on faces via IR can be between 0 and IR_DATA_VALUE_MAX
// If you try to send higher than this, the max value will be sent.

#define IR_DATA_VALUE_MAX 63

// The blinksatate library continuously transmits on all IR LEDs
// when enabled. If you don't want this, then disable it.

// blinkstate is enabled by default, but you can renable it after you have disabled it.
void blinkStateEnable(void);

// If called from setup(), then no blinkstate will be transmitted at all

void blinkStateDisable(void);

// Setup the blinkstate layer.
// Enables blinkstate functionality by default
void blinkStateSetup(void);


// Called once per loop.
void blinkStateLoop(void);

// Returns the last received value on the indicated face
// Between 0 and IR_DATA_VALUE_MAX inclusive
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

// Set value that will be continuously broadcast on specified face.
// Value should be between 0 and IR_DATA_VALUE_MAX inclusive.
// If a value greater than IR_DATA_VALUE_MAX is specified, IR_DATA_VALUE_MAX will be sent.
// By default we power up with all faces sending the value 0.

void setValueSentOnFace( byte value , byte face );

// Same as setValueSentOnFace(), but sets all faces in one call.

void setValueSentOnAllFaces( byte value );



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
