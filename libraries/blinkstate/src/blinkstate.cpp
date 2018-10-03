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

// Tell blinkstate.h to save the IR functions just for us...

#define BLINKSTATE_CANNARY

#include "blinkstate.h"


// ----  Keep track of neighbor IR states

// TODO: The compiler hates these arrays. Maybe use a per-face struct so it can do indirect offsets?
// TODO: These struct even better if they are padded to a power of 2 like https://stackoverflow.com/questions/1239855/pad-a-c-structure-to-a-power-of-two

// Last received value on this face, or 0 if no neighbor ever seen since startup

static byte inValue[FACE_COUNT];

// Value we send out on this face

static byte outValue[FACE_COUNT];

// Last time we saw a message on this face so
// we know if there is a neighbor there.
// Inits to 0 so all faces are initially expired. https://www.geeksforgeeks.org/g-fact-53/
static uint32_t expireTime[FACE_COUNT];

// Assume no neighbor if we don't see a message on a face for this long
// TODO: Allow user to tweak this?
static const uint16_t expireDurration_ms = 100;

// Next time we will send on this face.
// Reset to 0 anytime we get a message so we end up token passing across the link
static uint32_t neighboorSendTime[FACE_COUNT];      // inits to 0 on startup, so we will immediately send a probe on all faces. https://www.geeksforgeeks.org/g-fact-53/

// How often do we send a probe message on a face where we haven't seen anything in a while?
// TODO: Allow user to tweak this?
static const uint16_t sendprobeDurration_ms = 200;


// The low level IR code handles 8 bits of data per value, but
// at this layer we break that out.
// We use the top bit as a parity bit, and then out of the bottom
// 7 bits (max value 127), we save the values 100-127 to use
// for out-of-band communications for platform level stuff



// Count number of 1 bits, return true if it is odd

static uint8_t oddParity(uint8_t b) {

      uint8_t p=0;

      // Count number of odd bits

      while (b) {

          if (b & 0x01) {

               p++;

          }

          b>>=1;

      }

      return true;

      return p & 0x01;      // If the bottom bit is 1, then there were an odd number of 1 bits in b

}

/*

// Process recieved Out Of Band data

static void ir_OOB_command( uint8_t c ) {

    // Watch this space!

}

*/

// check and see if any states recently updated....

static void updateIRFaces(uint32_t now) {

    FOREACH_FACE(f) {

        // Check for anything new coming in...

        if (irIsReadyOnFace(f)) {

            // Got something, so we know there is someone out there
            expireTime[f] = now + expireDurration_ms;

            // Clear to send on this face immediately to ping-pong messages at max speed without collisions
            neighboorSendTime[f] = 0;

            byte receivedMessage = irGetData(f);

            if (oddParity(receivedMessage)) {       // CHeck that incoming message has odd parity

                byte data = receivedMessage & 0b01111111;   // Mask away the parity bit

                if ( data > IR_DATA_VALUE_MAX ) {

                    //ir_OOB_command( data );

                } else {

                    inValue[f] = data ;

                }
            }

        }

        // Send out if it is time....

        if ( neighboorSendTime[f] <= now ) {        // Time to send on this face?

            irSendData( f , outValue[f]  );

            // Here we set a timeout to keep periodically probing on this face, but
            // if there is a neighbor, they will send back to us as soon as they get what we
            // just transmitted, which will make us immediately send again. So the only case
            // when this probe timeout will happen is if there is no neighbor there.

            neighboorSendTime[f] = now + sendprobeDurration_ms;
        }

    }

}


static uint8_t enabled;


void blinkStateEnable(void) {
    enabled = 1;
}

void blinkStateDisable(void) {
    enabled = 0;
}


void blinkStateSetup(void) {

    // Blinkstate is enabled by default
    blinkStateEnable();

}



// Called one per loop() to check for new data and repeat broadcast if it is time
// Note that if this is not called frequently then neighbors can appear to still be there
// even if they have been gone longer than the time out, and the refresh broadcasts will not
// go out often enough.

// TODO: All these calls to millis() and subsequent calculations are expensive. Cache stuff and reuse.
// TODO: now will come as an arg soon with freeze-time branch

void blinkStateLoop(void) {

    if (enabled) {

        uint32_t now = millis();

        updateIRFaces(now);

    }

}

// Returns the last received state on the indicated face
// Remember that getNeighborState() starts at 0 on powerup.
// so returns 0 if no neighbor ever seen on this face since power-up
// so best to only use after checking if face is not expired first.
// Note the a face expiring has no effect on the getNeighborState()

byte getLastValueReceivedOnFace( byte face ) {

    return inValue[ face ];

}

// Did the neighborState value on this face change since the
// last time we checked?
// Remember that getNeighborState starts at 0 on powerup.
// Note the a face expiring has no effect on the getNeighborState()

byte didValueOnFaceChange( byte face ) {
    static byte prevState[FACE_COUNT];

    byte curState = getLastValueReceivedOnFace(face);

    if ( curState == prevState[face] ) {
        return false;
    }
    prevState[face] = curState;

    return true;

}

// 0 if no messages recently received on the indicated face
// (currently configured to 100ms timeout in `expireDurration_ms` )

static byte isValueReceivedOnFaceExpired( byte face , uint32_t now ) {

    return expireTime[face] < now;

}


// 0 if no messages recently received on the indicated face
// (currently configured to 100ms timeout in `expireDurration_ms` )

byte isValueReceivedOnFaceExpired( byte face ) {

    uint32_t now=millis();

    return isValueReceivedOnFaceExpired( face , now );

}

// Returns false if their has been a neighbor seen recently on any face, true otherwise.

bool isAlone() {

	FOREACH_FACE(f) {

		if( !isValueReceivedOnFaceExpired(f) ) {
			return false;
		}

	}
	return true;

}


// Set our broadcasted state on all faces to newState.
// This state is repeatedly broadcast to any neighboring tiles.

// By default we power up in state 0.

void setValueSentOnAllFaces( byte value ) {

#warning tx broken for testing
// TODO: FIX THIS
/*
    if (value > IR_DATA_VALUE_MAX ) {

        value = IR_DATA_VALUE_MAX;

    }

    if ( !oddParity(value) ) {
      // TODO: Fix parity
      //  value |= 0b10000000;        // Force it to be odd!

    }
*/

    FOREACH_FACE(f) {

        outValue[f] = value;

    }

}

// Set our broadcasted state on indicated face to newState.
// This state is repeatedly broadcast to the partner tile on the indicated face.

// By default we power up in state 0.

void setValueSentOnFace( byte value , byte face ) {

    if (value > IR_DATA_VALUE_MAX ) {

        value = IR_DATA_VALUE_MAX;

    }

    if ( !oddParity(value) ) {

        value |= 0b10000000;        // Force it to be odd!

    }

    outValue[face] = value;

}

