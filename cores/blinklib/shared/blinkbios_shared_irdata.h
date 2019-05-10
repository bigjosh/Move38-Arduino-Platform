/*
 * blinkbios_pixel_block.h
 *
 * Defines the shared memory block used to check for button presses
 *
 */

#ifndef BLINKBIOS_IRDATA_BLOCK_H_
#define BLINKBIOS_IRDATA_BLOCK_H_

// #define USER_VOLATILE or BIOS_VOLATILE based on the presence of #define BIOS_VOLATILE_FLAG
#include "blinkbios_shared_volatile.h"

#include <stdint.h>

#define IR_FACE_COUNT 6
#define IR_FACE_BITMASK 0b00111111      // Mask for the 6 faces. Fails hard when we start making nonagon tiles. 

// Must be big enough to hold the biggest packet, which internally is PUSH packet.
#define IR_RX_PACKET_SIZE     40

// State for each receiving IR LED

struct ir_rx_state_t {

    BOTH_VOLATILE uint8_t packetBufferReady;                        // 1 if we got the trailing sync byte. Foreground reader will set this back to 0 to enable next read.

    USER_VOLATILE uint8_t packetBufferLen;                          // How many bytes currently in the packet buffer? Does not include checksum when bufferReady is set

    USER_VOLATILE uint8_t packetBuffer[ IR_RX_PACKET_SIZE+1 ];      // Assemble incoming packet here. +1 to hold the type byte. Type byte comes first, but shoul;d be ignored since the BIOS consumes anything with a type besides USERDATA
    // TODO: Deeper data buffer here?


    // These internal variables not interesting to user code.
    // They are only updated in ISR, so don't need to be volatile.

    uint8_t windowsSinceLastTrigger;                // How long since we last saw a trigger on this IR LED?

    // We add new samples to the bottom and shift up.
    // The we look at the pattern to detect data bits
    // This is better than using a counter because with a counter we would need
    // to check for overflow before incrementing. With a shift register like this,
    // 1's just fall off the top automatically and We can keep shifting 0's forever.

    uint8_t byteBuffer;                             // Buffer for RX byte in progress. Data bits shift up until full byte received.
    // We prime with a '1' in the bottom bit when we get a valid start.
    // This way we can test for 0 to see if currently receiving a byte, and
    // we can also test for '1' in top position to see if full byte received.


    // This struct should be even power of 2 in length for more efficient array indexing. 
    
    // For future use?
    uint8_t slack[4];
          

};

struct blinkbios_irdata_block_t {

    ir_rx_state_t ir_rx_states[ IR_FACE_COUNT ];

    uint8_t download_in_progress_flag;          // Set if a download is in progress so subsequent SEED packets will be ignored
    
    // For future use?
    uint8_t slack[4];
    

};

extern blinkbios_irdata_block_t blinkbios_irdata_block;

// You can use this function to see if there is an RX in progress on a face
// Don't transmit if there is an RX in progress or you'll make a collision

inline uint8_t blinkbios_is_rx_in_progress( uint8_t face ) {

    // This uses the fact that anytime we are actively receiving a byte, the byte buffer will
    // be nonzero. Even if we are getting an all-0 byte, the top bit will be marching up.
    // This is handy, but it does allow a collision during the leading sync. See below.

    return blinkbios_irdata_block.ir_rx_states[face].byteBuffer;

}


#endif /* BLINKBIOS_IRDATA_BLOCK_H_ */