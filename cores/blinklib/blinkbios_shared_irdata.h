/*
 * blinkbios_pixel_block.h
 *
 * Defines the shared memory block used to check for button presses
 *
 */

#ifndef BLINKBIOS_IRDATA_BLOCK_H_
#define BLINKBIOS_IRDATA_BLOCK_H_

#include <stdint.h>

#define  IR_FACE_COUNT 6

#define IR_RX_PACKET_SIZE     135

// State for each receiving IR LED

struct ir_rx_state_t {

    // These internal variables are only updated in ISR, so don't need to be volatile.

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


    uint8_t packetBuffer[ IR_RX_PACKET_SIZE];        // Assemble incoming packet here
    // TODO: Deeper data buffer here?

    uint8_t packetBufferLen;                         // How many bytes currently in the packet buffer?

    volatile uint8_t packetBufferReady;              // 1 if we got the trailing sync byte. Foreground reader will set this back to 0 to enable next read.

    // This struct should be even power of 2 in length for more efficient array indexing.

};

struct blinkbios_irdata_block_t {

    ir_rx_state_t ir_rx_states[ IR_FACE_COUNT ];

};

extern blinkbios_irdata_block_t blinkbios_irdata_block;


#endif /* BLINKBIOS_IRDATA_BLOCK_H_ */