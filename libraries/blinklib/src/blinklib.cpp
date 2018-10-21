/*
 *
 *  This library lives in userland and acts as a shim to th blinkos layer
 *
 *  This view tailored to be idiomatic Arduino-y. There are probably better views of the interface if you are not an Arduinohead. 
 *
 * In this view, each tile has a "state" that is represented by a number between 1 and 127.
 * This state value is continuously broadcast on all of its faces.
 * Each tile also remembers the most recently received state value from he neighbor on each of its faces.
 *
 * You supply setup() and loop().
 *
 * While in loop(), the world is frozen. All changes you make to the pixels and to data on the faces
 * is buffered until loop returns. 
 * 
 */

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>         //rand()

#include <avr/pgmspace.h>   // PROGMEM for parity lookup table

#include <stddef.h>

#include "ArduinoTypes.h"

#include "blinklib.h"

#include "blinkos.h"


#define TX_PROBE_TIME_MS           150     // How often to do a blind send when no RX has happened recently to trigger ping pong
                                           // Nice to have probe time shorter than expire time so you have to miss 2 messages
                                           // before the face will expire
    
#define RX_EXPIRE_TIME_MS         200      // If we do not see a message in this long, then show that face as expired


// Keep these handy 

loopstate_in_t const *m_loopstate_in; 

loopstate_out_t *m_loopstate_out;


// This is a parity check that I came up with that I think is robust to close together bitflips and
// also efficient to calculate. 
// We keep the 6 data bits in the middle of the byte, and then send parity bits at the top and bottom
// that cover alternating data bits. This should catch any 1 or 2 consecutive flips. 
// Top parity is even, bottom is odd. The idea here is that a string of all 1's or 0's will fail. 

// A hamming code would have been better, but we need 6 data bits rather than 4. 

// Commented code below generates the output parity checked bytes for all 63 valid input values. 
// https://www.onlinegdb.com/online_c++_compiler

/*

#include <iostream>

using namespace std;

uint8_t paritygenerator( uint8_t d ) {
    
    // TODO: We are only counting the bottom 6 bits, so could replace popcount()
    //       with some asm that only counts those with 5 shifts
    //       https://www.avrfreaks.net/forum/avr-gcc-de-optimizing-my-code
    
    uint8_t topbits   = d & 0b00010101;
    uint8_t topbitcount = __builtin_popcount( topbits );
    uint8_t topparitybit = !(topbitcount & 0b00000001 );
    
    
    uint8_t bottombits = d & 0b00101010;
    uint8_t bottombitcount = __builtin_popcount( bottombits );
    uint8_t bottomparitybit = (bottombitcount & 0b00000001 );
    
    
    return ( ( topparitybit << 7 ) | ( d << 1 ) | ( bottomparitybit ) );
    
}

#include <iostream>
#include <iomanip>

#include <bitset>
using std::setw;

void generateParityTable( void ) {
    
    for( uint8_t d =0; d < 64 ; d++ ) {
        
        std::bitset<8> x(paritygenerator( d ) );
        std::cout << "    0b" << x << ",   // " << setw(2) << (int) d << "\n";
        
    }
    
}


int main()
{

    generateParityTable();

    return 0;
}
*/

// We precompute the parity table for efficiency 
// Look how nicely those bits encode! Try and change two bits in a row - bet you can't! Good hamming!

static const uint8_t PROGMEM parityTable[] = {
    
    0b10000000,   //  0
    0b00000010,   //  1
    0b10000101,   //  2
    0b00000111,   //  3
    0b00001000,   //  4
    0b10001010,   //  5
    0b00001101,   //  6
    0b10001111,   //  7
    0b10010001,   //  8
    0b00010011,   //  9
    0b10010100,   // 10
    0b00010110,   // 11
    0b00011001,   // 12
    0b10011011,   // 13
    0b00011100,   // 14
    0b10011110,   // 15
    0b00100000,   // 16
    0b10100010,   // 17
    0b00100101,   // 18
    0b10100111,   // 19
    0b10101000,   // 20
    0b00101010,   // 21
    0b10101101,   // 22
    0b00101111,   // 23
    0b00110001,   // 24
    0b10110011,   // 25
    0b00110100,   // 26
    0b10110110,   // 27
    0b10111001,   // 28
    0b00111011,   // 29
    0b10111100,   // 30
    0b00111110,   // 31
    0b11000001,   // 32
    0b01000011,   // 33    
    0b01000110,   // 35
    0b01001001,   // 36
    0b11001011,   // 37
    0b01001100,   // 38
    0b11001110,   // 39
    0b11010000,   // 40
    0b01010010,   // 41
    0b11010101,   // 42
    0b01010111,   // 43
    0b01011000,   // 44
    0b11011010,   // 45
    0b01011101,   // 46
    0b11011111,   // 47
    0b01100001,   // 48
    0b11100011,   // 49
    0b01100100,   // 50
    0b11100110,   // 51
    0b11101001,   // 52
    0b01101011,   // 53
    0b11101100,   // 54
    0b01101110,   // 55
    0b01110000,   // 56
    0b11110010,   // 57
    0b01110101,   // 58
    0b11110111,   // 59
    0b11111000,   // 60
    0b01111010,   // 61
    0b11111101,   // 62
    0b01111111,   // 63
};

static uint8_t parityEncode( uint8_t d ) {
    return pgm_read_byte_near( parityTable+ d );
}    

static uint8_t parityDecode( uint8_t d ) {

    return (d & 0b01111110) >> 1 ;     
    
}    


// TODO: These struct even better if they are padded to a power of 2 like https://stackoverflow.com/questions/1239855/pad-a-c-structure-to-a-power-of-two

struct face_t {
    
    uint8_t inValue;           // Last received value on this face, or 0 if no neighbor ever seen since startup
    uint8_t outValue;          // Value we send out on this face
    millis_t expireTime;    // When this face will be consider to be expired (no neighboor there)
    millis_t sendTime;      // Next time we will transmit on this face (set to 0 everytime we get a good message so we ping-pong across the link)
    
};

static face_t faces[FACE_COUNT];

// Grab once from loopstate
millis_t now;

static void updateIRFaces( const ir_data_buffer_t *ir_data_buffers ) {
    
    //  Use these pointers to step though the arrays   
    face_t *face = faces;
    const ir_data_buffer_t *ir_data_buffer = ir_data_buffers;
    
    for( uint8_t f=0; f < FACE_COUNT ; f++ ) {

            // Check for anything new coming in...

        if ( ir_data_buffer->ready_flag ) {

            // Got something, so we know there is someone out there
            // TODO: Should we require the received packet to pass error checks?
            face->expireTime = now + RX_EXPIRE_TIME_MS;

            if ( ir_data_buffer->len == 1 ) {
                
                const uint8_t *packetBuffer = ir_data_buffer->data;
                                
                // We only deal with 1 byte long packets in blinklib so anything else is an error
                
                uint8_t receivedByte = packetBuffer[0];
                
                uint8_t decodedByte = parityDecode( receivedByte );
                
                if ( receivedByte == parityEncode( decodedByte ) ) {
                    
                    // This looks like a valid value!
                
                        // OK, everything checks out, we got a good face value!

                        face->inValue = decodedByte;

                        // Clear to send on this face immediately to ping-pong messages at max speed without collisions
                        face->sendTime = 0;

                } //  if ( receivedByte == parityEncode( decodedByte ) )

            }   // if ( ir_data_buffer->len == 1 )
                
        }  // if ( ir_data_buffer->ready_flag )            


        // Send one out too if it is time....

        if ( face->sendTime <= now ) {        // Time to send on this face?

            uint8_t data = parityEncode( face->outValue );

            ir_send_userdata( f , &data  , sizeof(data) );

            // Here we set a timeout to keep periodically probing on this face, but
            // if there is a neighbor, they will send back to us as soon as they get what we
            // just transmitted, which will make us immediately send again. So the only case
            // when this probe timeout will happen is if there is no neighbor there.
            
            face->sendTime = now + TX_PROBE_TIME_MS;
            
        } // if ( face->sendTime <= now ) 

        face++;
        ir_data_buffer++;

    } // for( uint8_t f=0; f < FACE_COUNT ; f++ )

}

// Returns the last received state on the indicated face
// Remember that getNeighborState() starts at 0 on powerup.
// so returns 0 if no neighbor ever seen on this face since power-up
// so best to only use after checking if face is not expired first.
// Note the a face expiring has no effect on the getNeighborState()

byte getLastValueReceivedOnFace( byte face ) {

    return faces[face].inValue;

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



byte isValueReceivedOnFaceExpired( byte face ) {

    return faces[face].expireTime < now;

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

        faces[f].outValue = value;

    }

}

// Set our broadcasted state on indicated face to newState.
// This state is repeatedly broadcast to the partner tile on the indicated face.

// By default we power up in state 0.

void setValueSentOnFace( byte value , byte face ) {

     if (value > IR_DATA_VALUE_MAX ) {

         value = IR_DATA_VALUE_MAX;

     }

    faces[face].outValue = value;

}


static buttonstate_t buttonstate;

bool buttonDown(void) {
    return buttonstate.down;
}    

static bool grabandclearbuttonflag( uint8_t flagbit ) {
    bool r = buttonstate.bitflags & flagbit;
    buttonstate.bitflags &= ~ flagbit;
    return r;
}    
    
bool buttonPressed(void) {
    return grabandclearbuttonflag( BUTTON_BITFLAG_PRESSED );
}
    
bool buttonReleased(void) {
    return grabandclearbuttonflag( BUTTON_BITFLAG_RELEASED );
}    

bool buttonSingleClicked() {
    return grabandclearbuttonflag( BUTTON_BITFLAG_SINGLECLICKED );
}    

bool buttonDoubleClicked() {
    return grabandclearbuttonflag( BUTTON_BITFLAG_DOUBECLICKED );
}    

bool buttonMultiClicked() {
    return grabandclearbuttonflag( BUTTON_BITFLAG_MULITCLICKED );
}    


// The number of clicks in the longest consecutive valid click cycle since the last time called.
byte buttonClickCount(void) {
    return buttonstate.clickcount;
}    

// Remember that a long press fires while the button is still down
bool buttonLongPressed(void) {
    grabandclearbuttonflag( BUTTON_BITFLAG_LONGPRESSED );
}    

void setupEntry() {
        
    // Call up to the userland code
    setup();    
    
}    

void loopEntry( loopstate_in_t const *loopstate_in , loopstate_out_t *loopstate_out) {

    
    now = loopstate_in->millis;  
    
    updateIRFaces( loopstate_in->ir_data_buffers );
    
    // Capture the incoming button state. We OR in the flags becuase in blinklib model we clear the flags only when read.
    buttonstate.bitflags |= loopstate_in->buttonstate.bitflags;
    buttonstate.clickcount = loopstate_in->buttonstate.clickcount;
    buttonstate.down = loopstate_in->buttonstate.down;
    
    // Call the user program

    loop();
    
}    

