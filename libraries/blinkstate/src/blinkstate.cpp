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

#include "irdata.h"

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


// check and see if any states recently updated....

#if IR_DATA_VALUE_MAX >= (1<<6)
    #error You cant change the max value without changing the code below to not use the top two bits for multiplexing packet types!
#endif

static void updateIRFaces(uint32_t now) {

    FOREACH_FACE(f) {

        // Check for anything new coming in...
        
        if (irDataIsPacketReady(f)) {

            // Got something, so we know there is someone out there
            // TODO: Should we require the received packet to pass error checks?
            expireTime[f] = now + expireDurration_ms;
            
            const byte *packetBuffer = irDataPacketBuffer(f);
            
            
            if ( packetBuffer[0] <= IR_DATA_VALUE_MAX ) {
                
                // If the top two bits of the packet header byte are 0, then this is a user face value
                // We special case this out to keep the refresh rates up.
                // In this case, the packet should only be 2 bytes long
                // and the 2nd byte is the invert of the first for error checking


                // If packet is not 2 bytes long then we have a problem
                
                if ( irDataPacketLen( f ) == 2 ) {
                    
                    // We error check these special packets by sending the 2nd byte as the invert of the first
                    
                    byte data = packetBuffer[0];
                    byte invert = packetBuffer[1];
                    
                    if ( data == ((byte) (~invert)) ) {
                        
                        // OK, everything checks out, we got a good face value!
                        
                        inValue[f] = data;
                        
                        // Clear to send on this face immediately to ping-pong messages at max speed without collisions
                        neighboorSendTime[f] = 0;                                                
                        
                    }                                
                    
                }                    
                                
            } else {
                
                // Some other kind of packet received
                               
                
            }                                
            
            irDataMarkPacketRead( f );

        }

        // Send one out too if it is time....

        if ( neighboorSendTime[f] <= now ) {        // Time to send on this face?
            
            byte packetBuffer[2];
            
            byte data = outValue[f];
                       
            // TODO: Check the asm on this casted inversion
            packetBuffer[0] = data;            
            packetBuffer[1] = (byte) (~data);          // Invert byte as an error check for face values 

            irSendData( f , packetBuffer , 2 );

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
    
     if (value > IR_DATA_VALUE_MAX ) {

         value = IR_DATA_VALUE_MAX;

     }    

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

    outValue[face] = value;

}

