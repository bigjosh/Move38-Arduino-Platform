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

#include <avr/pgmspace.h>   // PROGMEM for parity lookup table
#include <avr/interrupt.h>  // cli() and sei() so we can get snapshots of multibyte variables

#include <avr/sleep.h>      // sleep_cpu() so we can rest between interrupts.

#include <stddef.h>

#include "ArduinoTypes.h"

#include "blinklib.h"

// Here are our magic shared memory links to the BlinkBIOS running up in the bootloader area.
// These special sections are defined in a special linker script to make sure that the addresses
// are the same on both the foreground (this blinklib program) and the background (the BlinkBIOS project compiled to a HEX file)

// The actual memory for these blocks is allocated in main.cpp. Remember, it overlaps with the same blocks in BlinkBIOS code running in the bootloader!

#include "shared/blinkbios_shared_button.h"
#include "shared/blinkbios_shared_millis.h"
#include "shared/blinkbios_shared_pixel.h"
#include "shared/blinkbios_shared_irdata.h"
#include "shared/blinkbios_shared_slack.h"

#include "shared/blinkbios_shared_functions.h"     // Gets us ir_send_packet()


#define TX_PROBE_TIME_MS           150     // How often to do a blind send when no RX has happened recently to trigger ping pong
                                           // Nice to have probe time shorter than expire time so you have to miss 2 messages
                                           // before the face will expire

#define RX_EXPIRE_TIME_MS         200      // If we do not see a message in this long, then show that face as expired


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

// Want to know how the magic is done?
// HINT: The first and last bits are simple odd parity. Odd because all 0's data would fail.
//       One is of all the middle bits, the other is only of the even bit slots.

// NOTE: If you want to change this, it must still match IR_MAX_VALUE

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
    0b11000100,   // 34
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

// This is a special byte that signals that this is a long data packet
// It must appear in the first byte of the data, and the final byte is an inverted checksum of all bytes including header byte

#define LONG_DATA_SPECIAL_VALUE     0b10101010

// This is a special byte that triggers a force sleep cycle when received
// It must appear in the first byte of data, and the second byte muct be a 2nd copy.
// When we get it, we virally send out force sleep packets on all the faces that we did not get it on,
// and then we go to sleep.

#define FORCE_SLEEP_SPECIAL_VALUE   0b01010101

static uint8_t parityEncode( uint8_t d ) {
    return pgm_read_byte_near( parityTable+ d );
}

// The actual data is hidden in the middle

static uint8_t parityDecode( uint8_t d ) {

    return (d & 0b01111110) >> 1 ;

}


// TODO: These structs even better if they are padded to a power of 2 like https://stackoverflow.com/questions/1239855/pad-a-c-structure-to-a-power-of-two

struct face_t {

    uint8_t inValue;        // Last received value on this face, or 0 if no neighbor ever seen since startup
    uint8_t outValue;       // Value we send out on this face
    millis_t expireTime;    // When this face will be consider to be expired (no neighbor there)
    millis_t sendTime;      // Next time we will transmit on this face (set to 0 every time we get a good message so we ping-pong across the link)

};


static face_t faces[FACE_COUNT];

// Millis snapshot for this pass though loop
millis_t now;

unsigned long millis() {
    return now;
}

// Returns the inverted checksum of all bytes

uint8_t computeChecksum( volatile const uint8_t *buffer , uint8_t len ) {

    uint8_t computedChecksum = 0;

    for( uint8_t l=0; l < len ; l++ ) {

        computedChecksum += *buffer++;

    }

    return computedChecksum ^ 0xff ;

}


#if  ( ( IR_LONG_PACKET_MAX_LEN + 3  ) > IR_RX_PACKET_SIZE )

    #error There has to be enough room in the blinkos packet buffer to hold the user packet plus 2 header bytes and one checksum byte

#endif

// The length of the long packet in each face buffer
// The packet starts at byte 3 because there are 2 header bytes
// This len already has the header bytes deducted and also the checksum at the end.
// TODO: If we share the static packetbuffers directly up from blinkOS then this could all be shorted and sweeter
static byte  longPacketLen[FACE_COUNT];
static byte *longPacketData[FACE_COUNT];

byte getPacketLengthOnFace( uint8_t face ) {
    return longPacketLen[ face ];
}

boolean isPacketReadyOnFace( uint8_t face ) {
    return getPacketLengthOnFace(face) != 0;
}

const byte *getPacketDataOnFace( uint8_t face ) {
    return longPacketData[face];
}

void markLongPacketRead( uint8_t face ) {
    // We don't want to mark it read if it is not actually pending because then we might be throwing away an unread packet
    #warning
    cli();
    if ( longPacketLen[face] ) {
        longPacketLen[face]=0;
        blinkbios_irdata_block.ir_rx_states[face].packetBufferReady=0;
    }
    sei();
}

// Jump to the send packet function all way up in the bootloader

uint8_t blinkbios_irdata_send_packet(  uint8_t face, const uint8_t *data , uint8_t len ) {

    // Call directly into the function in the bootloader. This symbol is resoved by the linker to a
    // direct call to the taget address.
    return BLINKBIOS_IRDATA_SEND_PACKET_VECTOR(face,data,len);

}

#define SBI(x,b) (x|= (1<<b))           // Set bit
#define CBI(x,b) (x&=~(1<<b))           // Clear bit
#define TBI(x,b) (x&(1<<b))             // Test bit

uint8_t rx_buffer_fresh_bitflag;    // One bit per IR LED indicates that we just got a new state update packet on this face
                                    // Needed to know when it is clear to send on that face to keep up the ping-pong
                                    // when packets are involved
                                    // We could also expose a "valueOnFaceJustRefreshed()" function but we are getting so far
                                    // away from the original blink state model here! Really packets do not even fit in this model
                                    // but give the people what they want!

// This is the easy way to do this, but uses RAM unnecessarily.
// TODO: Make a scatter version of this to save RAM & time

static uint8_t ir_send_packet_buffer[ IR_LONG_PACKET_MAX_LEN + 3 ];

uint8_t sendPacketOnFace( byte face , const void *data, byte len ) {

    if ( len > IR_LONG_PACKET_MAX_LEN ) {

        // Ignore request to send oversized packet

        return 0;

    }

    if ( ! TBI( rx_buffer_fresh_bitflag , face ) ) {
        // We never blind send a packet. Instead we wait for a normal (short) value message to come in
        // and then we reply with the packet to avoid collisions. We depend on the value messages to
        // do handshaking and also to discover when there is a neighbor on the other side.

        // The caller should see this 0 and then try to send again until they hit a moment when we know it is safe to send
        return 0;

    }

    const uint8_t *s = (const uint8_t *) data ;           // Just to convert from void to uint8_t

    uint8_t *b = ir_send_packet_buffer;

    // Build up the packet in the buffer

    *b++= LONG_DATA_SPECIAL_VALUE;

    uint8_t packetLen = len+2;      // One extra byte for the LONG_DATA_PACKET_HEADER and one for the checksum

    uint8_t computedChecksum= LONG_DATA_SPECIAL_VALUE;    // The header bytes are included in the checksum

    while (len--) {

        computedChecksum += *s;

        *b++= *s++;

    }

    *b = computedChecksum ^ 0xff;   // invert checksum

    // Note that this automatically adds the correct packet header byte for us so we do not include it!
    uint8_t sent_flag = blinkbios_irdata_send_packet( face , ir_send_packet_buffer , packetLen );

    if (sent_flag) {

          faces[face].sendTime = now + TX_PROBE_TIME_MS;

    }

    return sent_flag ;

}


static void clear_packet_buffers() {

    FOREACH_FACE(f) {

        blinkbios_irdata_block.ir_rx_states[f].packetBufferReady = 0;

    }
}

#warning debug
#define SP_PINA_1() asm(" sbi 0x0e, 2")
#define SP_PINA_0() asm(" cbi 0x0e, 2")
#define _SFR_MEM8X(mem_addr)  (*(volatile uint8_t *)(mem_addr))
 #define SP_TX_NOW(x) ( _SFR_MEM8X(0xc6 ) = x )
#define SP_TX(x)   do { while (!(_SFR_MEM8X(0xC0)&(1<<5))); SP_TX_NOW(x);}while (0)         // Wait for buffer to be clear so we don't overwrite in progress

static void force_sleep_cycle() {



    FOREACH_FACE(f) {
        blinkbios_pixel_block.rawpixels[f] = rawpixel_t(255,255,255);     // TODO: Check and make sure this doesn't constructor each time
    }

    // Ok, now we are virally sending FORCE_SLEEP out on all faces to spread the word
    // and the pixels are off so the user is happy and we are saving power.

    // First send the force sleep packet out to all our neighbors
    // We are indiscriminate, just splat it 100 times everywhere.
    // This is a brute force approach to make sure we get though even with collisons
    // and long packets in flight.


    // We need a pointer for the value to send it...
    uint8_t force_sleep_packet = FORCE_SLEEP_SPECIAL_VALUE;

    for( uint8_t n=0; n<5; n++ ) {

        FOREACH_FACE(f) {

            //while ( blinkbios_is_rx_in_progress( f ) );     // Wait to clear to send (no guarantee, but better than just blink sending)

            blinkbios_irdata_send_packet( f , &force_sleep_packet , sizeof( force_sleep_packet ) );

        }

    }
    
    // We need to save the time now because it will keep ticking while we are in pre-sleep (where were can get 
    // woken back up by a packet). If we did not save it and then restore it later, then all the user timers
    // would be expired when we woke. 

    // Save the time now so we can go back in time when we wake up
    cli();
    millis_t save_time = blinkbios_millis_block.millis;
    sei();

    // OK we now appear asleep
    // We are not sending IR so some power savings
    // For the next 2 hours will will wait for a wake up signal
    // TODO: Make this even more power efficient by sleeping between checks for incoming IR.

    blinkbios_button_block.bitflags=0;

    // Here we explicitly set the register rather than using functions to save a few bytes.
    // We not only save the 2 bytes here, but also because we do not need an explict sleep_mode_enable() elsewhere.

    //set_sleep_mode( SLEEP_MODE_IDE );      // Wake on pin change and Timer2. <1uA

    /*
        3caa:	83 b7       	in	r24, 0x33	; 51
        3cac:	81 7f       	andi	r24, 0xF1	; 241
        3cae:	84 60       	ori	r24, 0x04	; 4
        3cb0:	83 bf       	out	0x33, r24	; 51
    */

    SMCR = _BV(SE);     // Enable sleep, idle mode
    /*
        3caa:	85 e0       	ldi	r24, 0x05	; 0
        3cac:	83 bf       	out	0x33, r24	; 51
        3cae:	88 95       	sleep
    */

    clear_packet_buffers();     // Clear out any left over packets that were there when we started this sleep cyclel and might trigger us to wake unapropriately

    uint8_t saw_packet_flag =0;

    SP_TX('I');

    // Wait in idle mode until we either see a non-force-sleep packet or a button press or woke.
    // Why woke? Because eventually the BIOS will make us powerdown sleep inside this loop
    // When that happens, it will take a button press to wake us

    blinkbios_button_block.wokeFlag = 1;    // // Set to 0 upon waking from sleep

    while (!saw_packet_flag && !(blinkbios_button_block.bitflags & BUTTON_BITFLAG_PRESSED) && blinkbios_button_block.wokeFlag) {

        // TODO: This sleep mode currently uses about 2mA. We can get that way down by...
        //       1. Adding a supporess_display_flag to pixel_block to skip all of the display code when in this mode
        //       2. Adding a new_pack_recieved_flag to ir_block so we only scan when there is a new packet

        sleep_cpu();

        ir_rx_state_t *ir_rx_state = blinkbios_irdata_block.ir_rx_states;

        FOREACH_FACE( f ) {

            if (ir_rx_state->packetBufferReady) {

                if (ir_rx_state->packetBuffer[1] != FORCE_SLEEP_SPECIAL_VALUE ) {

                    SP_TX('M');
                    SP_TX(ir_rx_state->packetBuffer[0] );
                    SP_TX(ir_rx_state->packetBuffer[1] );

                    saw_packet_flag =1;

                }

                ir_rx_state->packetBufferReady=0;

            }

            ir_rx_state++;
        }

    }


    cli();
    blinkbios_millis_block.millis = save_time;
    BLINKBIOS_POSTPONE_SLEEP_VECTOR();              // It is ok top call like this to reset the inactivity timer
    sei();

    // Forced sleep mode
    // Really need button down detection in bios so we only wake on lift...
    // BLINKBIOS_SLEEP_NOW_VECTOR();

    // Clear out old packets (including any old FORCE_SLEEP packets so we don't go right back to bed)

    clear_packet_buffers();

}

// Are we in forced sleep mode?

uint8_t forced_sleep_flag =0;

static void RX_IRFaces() {

    //  Use these pointers to step though the arrays
    face_t *face = faces;
    volatile ir_rx_state_t *ir_rx_state = blinkbios_irdata_block.ir_rx_states;

    for( uint8_t f=0; f < FACE_COUNT ; f++ ) {

            // Check for anything new coming in...

        if ( ir_rx_state->packetBufferReady ) {

            // Got something, so we know there is someone out there
            // TODO: Should we require the received packet to pass error checks?
            face->expireTime = now + RX_EXPIRE_TIME_MS;

            // This is slightly ugly. To save a buffer, we get the full packet with the header byte.

            volatile const uint8_t *packetData = (ir_rx_state->packetBuffer)+1;    // Start at the byte after the header
            uint8_t packetDataLen = (ir_rx_state->packetBufferLen)-1;     // deduct the packet header byte

            if ( packetDataLen == 1 ) {         // normal user face value, One header byte + One data byte

                // This is a normal blinklib 1-byte face value

                uint8_t receivedByte = packetData[0];

                if (receivedByte==FORCE_SLEEP_SPECIAL_VALUE) {

                    force_sleep_cycle();

                } else {

                    uint8_t decodedByte = parityDecode( receivedByte );

                    // Is this a valid symbol in our parity system?

                    if ( receivedByte == parityEncode( decodedByte ) ) {

                        // This looks like a valid value!

                        // OK, everything checks out, we got a good face value!

                        face->inValue =decodedByte;

                        // Clear to send on this face immediately to ping-pong messages at max speed without collisions
                        face->sendTime = 0;

                        SBI( rx_buffer_fresh_bitflag , f );     // Mark that we just got a valid message on this face so we can ping pong with packets. Code smell!

                    } //  if ( receivedByte == parityEncode( decodedByte ) )

                    // Mark the data buffer as consumed. This clears it out so it will be ready to receive the next packet
                    // If we don't do this, then we might miss the response to our ping
                }

                ir_rx_state->packetBufferReady=0;

            } else if (packetDataLen>2 && packetData[0] == LONG_DATA_SPECIAL_VALUE ) {

                if ( computeChecksum( packetData , packetDataLen-1)  ==  packetData[ packetDataLen - 1 ] ) {

                    // Ok this packet checks out folks!

                    longPacketLen[f] = packetDataLen-2;           // We deduct 2 from he length to account for the header byte and the trailing checksum byte
                    longPacketData[f] = (byte *) (packetData+ 1);       // Skip the header bytes

                }

                // NOTE that we do not mark this packet as read! This will give the suer a chance to read it directly from the IR buffer.

                // Clear to send on this face immediately to ping-pong messages at max speed without collisions
                face->sendTime = 0;

            } else {        // We don't recognize this packet

                // Note that we do not clear to send on this face. There was some kind of problem, maybe corruption
                // so there might still be stuff in flight and we don't want to step on it.
                // Instead we let the timer take over and let things cool down until one side blinks sends to
                // start things up again.

                // Packet too short so consume it to make room for next.

                ir_rx_state->packetBufferReady=0;

            }



        }  // if ( ir_data_buffer->ready_flag )

        face++;
        ir_rx_state++;

    } // for( uint8_t f=0; f < FACE_COUNT ; f++ )

}

static void TX_IRFaces() {

    //  Use these pointers to step though the arrays
    face_t *face = faces;

    for( uint8_t f=0; f < FACE_COUNT ; f++ ) {
        // Send one out too if it is time....

        if ( face->sendTime <= now ) {        // Time to send on this face?
                                              // Note that we do not use the rx_fresh flag here because we want the timeout
                                              // to do automatic retries to kickstart things when a new neighbor shows up or
                                              // when an IR message gets missed

            uint8_t data = parityEncode( face->outValue );

            if (blinkbios_irdata_send_packet( f , &data  , sizeof(data) ) ) {
                // Here we set a timeout to keep periodically probing on this face, but
                // if there is a neighbor, they will send back to us as soon as they get what we
                // just transmitted, which will make us immediately send again. So the only case
                // when this probe timeout will happen is if there is no neighbor there.

                // If ir_send_userdata() returns 0, then we could not send becuase there was an RX in progress on this face.
                // Because we do not reset the sentTime in that case, we will automatically try again next pass.

                face->sendTime = now + TX_PROBE_TIME_MS;
            }

        } // if ( face->sendTime <= now )

        face++;

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



// --------------Button code


// Here we keep a local snapshot of the button block stuff

static uint8_t buttonSnapshotDown;               // 1 if button is currently down (debounced)

static uint8_t buttonSnapshotBitflags;

static uint8_t buttonSnapshotClickcount;         // Number of clicks on most recent multiclick


bool buttonDown(void) {
    return buttonSnapshotDown != 0;
}

static bool grabandclearbuttonflag( uint8_t flagbit ) {
    bool r = buttonSnapshotBitflags & flagbit;
    buttonSnapshotBitflags &= ~ flagbit;
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
    return grabandclearbuttonflag( BUTTON_BITFLAG_DOUBLECLICKED );
}

bool buttonMultiClicked() {
    return grabandclearbuttonflag( BUTTON_BITFLAG_MULITCLICKED );
}


// The number of clicks in the longest consecutive valid click cycle since the last time called.
byte buttonClickCount(void) {
    return buttonSnapshotClickcount;
}

// Remember that a long press fires while the button is still down
bool buttonLongPressed(void) {
    return grabandclearbuttonflag( BUTTON_BITFLAG_LONGPRESSED );
}

// 6 second press. Note that this will trigger seed mode if the blink is alone so
// you will only ever see this if blink has neighbors when the button hits the 6 second mark.
// Remember that a long press fires while the button is still down
bool buttonLongLongPressed(void) {
    return grabandclearbuttonflag( BUTTON_BITFLAG_6SECPRESSED );
}


// --- Utility functions

Color makeColorRGB( byte red, byte green, byte blue ) {

    // Internal color representation is only 5 bits, so we have to divide down from 8 bits
    return Color( red >> 3 , green >> 3 , blue >> 3 );

}

Color makeColorHSB( uint8_t hue, uint8_t saturation, uint8_t brightness ) {

    uint8_t r;
    uint8_t g;
    uint8_t b;

    if (saturation == 0)
    {
        // achromatic (grey)
        r =g = b= brightness;
    }
    else
    {
        unsigned int scaledHue = (hue * 6);
        unsigned int sector = scaledHue >> 8; // sector 0 to 5 around the color wheel
        unsigned int offsetInSector = scaledHue - (sector << 8);  // position within the sector
        unsigned int p = (brightness * ( 255 - saturation )) >> 8;
        unsigned int q = (brightness * ( 255 - ((saturation * offsetInSector) >> 8) )) >> 8;
        unsigned int t = (brightness * ( 255 - ((saturation * ( 255 - offsetInSector )) >> 8) )) >> 8;

        switch( sector ) {
            case 0:
            r = brightness;
            g = t;
            b = p;
            break;
            case 1:
            r = q;
            g = brightness;
            b = p;
            break;
            case 2:
            r = p;
            g = brightness;
            b = t;
            break;
            case 3:
            r = p;
            g = q;
            b = brightness;
            break;
            case 4:
            r = t;
            g = p;
            b = brightness;
            break;
            default:    // case 5:
            r = brightness;
            g = p;
            b = q;
            break;
        }
    }

    return( makeColorRGB( r  , g   , b  ) );
}

// OMG, the Ardiuno rand() function is just a mod! We at least want a uniform distibution.

// Here we implement the SimpleRNG pseudo-random number generator based on this code...
// https://www.johndcook.com/SimpleRNG.cpp

// TODO: Make this calculation smaller with shorter ints, add a way to put entropy into the seeds

#define GETNEXTRANDUINT_MAX ( (word) -1 )

static word GetNextRandUint(void) {

    // These values are not magical, just the default values Marsaglia used.
    // Any unit should work.

    // We make them local static so that we only consume the storage if the random()
    // functions are actually ever called.

    static unsigned long u = 521288629UL;
    static unsigned long v = 362436069UL;

    v = 36969*(v & 65535) + (v >> 16);
    u = 18000*(u & 65535) + (u >> 16);

    return (v << 16) + u;

}

// return a random number between 0 and limit inclusive.
// TODO: Use entropy from the button or decaying IR LEDs
// https://stackoverflow.com/a/2999130/3152071

uint16_t random( uint16_t limit ) {

    word divisor = GETNEXTRANDUINT_MAX/(limit+1);
    word retval;

    do {
        retval = GetNextRandUint() / divisor;
    } while (retval > limit);

    return retval;
}

long map(word x, word in_min, word in_max, word out_min, word out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Returns the device's unique 8-byte serial number
// TODO: This should this be in the core for portability with an extra "AVR" byte at the front.

// 0xF0 points to the 1st of 8 bytes of serial number data
// As per "13.6.8.1. SNOBRx - Serial Number Byte 8 to 0"


const byte * const serialno_addr = ( const byte *)   0xF0;


// Read the unique serial number for this blink tile
// There are 9 bytes in all, so n can be 0-8


byte getSerialNumberByte( byte n ) {

    if (n>8) return(0);

    return serialno_addr[n];

}

// Remembers if we have woken from either a BIOS sleep or
// a blinklib forced sleep.

uint8_t hasWokenFlag =0;

// Returns 1 if we have slept and woken since last time we checked
// Best to check as last test at the end of loop() so you can
// avoid intermediate display upon waking.

uint8_t hasWoken(void) {

    uint8_t ret = 0;

    if (hasWokenFlag) {
        ret =1;
        hasWokenFlag = 0;
    }

    if (blinkbios_button_block.wokeFlag==0) {       // This flag is set to 0 when waking!
        ret=1;
        blinkbios_button_block.wokeFlag==1;
    }

    return ret;

}

// --- Pixel functions

// Change the tile to the specified color
// NOTE: all color changes are double buffered
// and the display is updated when loop() returns


// Set the pixel on the specified face (0-5) to the specified color
// NOTE: all color changes are double buffered
// and the display is updated when loop() returns

// A buffer for the colors.
// We use a buffer so we can update all faces at once during a vertical
// retrace to avoid visual tearing from partially applied updates

void setColorOnFace( Color newColor , byte face ) {

    // This is so ugly, but we need to match the volatile in the shared block to the newColor
    // There must be a better way, but I don't know it other than a memcpy which is even uglier!

    // This at least gets the semantics right of coping a snapshot of the actual value.

    blinkbios_pixel_block.pixelBuffer[face].as_uint16 =  newColor.as_uint16;              // Size = 1940 bytes


    // This BTW compiles much worse

    //  *( const_cast<Color *> (&blinkbios_pixel_block.pixelBuffer[face])) =  newColor;       // Size = 1948 bytes

}


void setColor( Color newColor) {

    FOREACH_FACE(f) {
        setColorOnFace( newColor , f );
    }

}


void setFaceColor(  byte face, Color newColor ) {

    setColorOnFace( newColor , face );

}

#warning debug
#define SP_INIT() do{  _SFR_MEM8(0xC0)=0x01; _SFR_MEM8(0xC1)=0x03;  _SFR_MEM16(0xC4) = 0; } while (0)       // Set TX 1Mbs
#define SP_TX_NOW(x) ( _SFR_MEM8(0xc6 ) = x )                                                                           // Write to UDR0 Serial port data register

// This is the main event loop that calls into the arduino program
// (Compiler is smart enough to jmp here from main rather than call!
//     It even omits the trailing ret!
//     Thanks for the extra 4 bytes of flash gcc!)

void __attribute__((noreturn)) run(void)  {

    // TODO: Is this right? Should hasWoke() return true or false on the first check after start up?

    blinkbios_button_block.wokeFlag = 1;        // Clear any old wakes (wokeFlag is cleared to 0 on wake)

    blinkbios_button_block.bitflags = 0x00;     // Clear any old button actions and start fresh

    setup();

    while (1) {

        // Capture time snapshot
        // It is 4 bytes long so we cli() so it can not get updated in the middle of us grabbing it

        cli();
        now = blinkbios_millis_block.millis;
        sei();

        // Here we check to enter seed mode. The button must be held down for 6 seconds and we must not have any neighbors
        // Note that we directly read the shared block rather than our snapshot. This lets the 6 second flag latch and
        // so to the user program if we do not enter seed mode because we have neighbors. See?

        if (( blinkbios_button_block.bitflags & BUTTON_BITFLAG_6SECPRESSED) && isAlone() ) {

            // Enter SEED mode!

            BLINKBIOS_BOOTLOADER_SEED_VECTOR();

            __builtin_unreachable();
        }

        if ( ( blinkbios_button_block.bitflags & BUTTON_BITFLAG_7SECPRESSED)  ) {

            force_sleep_cycle();

            // Clear out the press that put us to sleep so we do not see it again
            // Also clear out everything else so we start with a clean slate on waking
            blinkbios_button_block.bitflags = 0;

        }

        cli();
        buttonSnapshotDown       = blinkbios_button_block.down;
        buttonSnapshotBitflags  |= blinkbios_button_block.bitflags;     // Or any new flags into the ones we got
        blinkbios_button_block.bitflags=0;                              // Clear out the flags now that we have them
        buttonSnapshotClickcount = blinkbios_button_block.clickcount;
        sei();



        // Update the IR RX state
        // Receive any pending packets
        RX_IRFaces();

        loop();

        // Update the pixels to match our buffer

        BLINKBIOS_DISPLAY_PIXEL_BUFFER_VECTOR();

         // Go ahead and release any packets that the loop() forgot to mark as read
         // We count on the fact that markLongPacketRead() here will only release pending packets
         // If we did not do this, then if the game forgot to clear a buffer then that face would be blocked
         // From OS messages forever. We could work around that by having separate user and OS buffers, but more memory
         // and complexity.

         FOREACH_FACE(f) {
            markLongPacketRead(f);
         }

         // Transmit any IR packets waiting to go out
         // Note that we do this after loop had a chance to update them.
         TX_IRFaces();

         rx_buffer_fresh_bitflag = 0;      // We missed our chance to send on the messages received on this pass.
                                           // RX_IRfaces() will set these again for messages received on the next pass.

    }


}

void seedMe() {

   // JUMP_TO_BOOTLOADER_SEED();

}    