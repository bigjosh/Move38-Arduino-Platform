// Support functions for being a bootloader server (able to send the current active game)


#include <avr/pgmspace.h>       // Functions for accessing flash

#include "blinkos.h"
#include "blinkboot.h"
#include "shared.h"
#include "blinkos_timer.h"
#include "blinkos_irdata.h"


#include "pixel.h"      // We go direct to the pixel layer her since we are out of the loop.
#include "debug.h"

#define PULL_REQUEST_PROBE_SPACING_MS 100               // THis is while we are hunting for neighbors to try to get them to pull from us


#define PULL_REQUEST_TIMEOUT_AFTER_PUSH_MS 450          // We will not start sending more pull request until this long after we sent a push
                                                        // To give the guy who asked for the push time to porcess it and ask for the next one

OS_Timer next_pull_request_timer;           // Next time to send a pull request
                                            // Spaced out so that we finish sending to a partner before starting with someone else.

uint8_t next_pull_request_target;           // Who to send the next pull request to

// I was so excited we were going to be able to statically initialize these packets at compile time
// and save so much time and space, but...
// "sorry, unimplemented: non-trivial designated initializers not supported"

pull_request_payload_t pull_request_packet_payload;

void blinkos_blinkboot_setup() {

    // TODO: Somehow precompute these at compile time since they do not change after that?

    uint8_t computed_checksum=IR_PACKET_HEADER_PULLREQUEST;

    pull_request_packet_payload.pages= DOWNLOAD_MAX_PAGES;
    computed_checksum+=DOWNLOAD_MAX_PAGES;

    pull_request_packet_payload.program_checksum = 0x1234;

    computed_checksum += pull_request_packet_payload.program_checksum & 0xff;
    computed_checksum += pull_request_packet_payload.program_checksum >> 8;

    pull_request_packet_payload.packet_checksum = ~computed_checksum;

};


uint8_t blinkos_blinkboot_sendPullRequest( uint8_t face ) {

    return ir_send_data( face , &pull_request_packet_payload , sizeof( pull_request_packet_payload )  , IR_PACKET_HEADER_PULLREQUEST );

}


// Send response to a pull message that contains one page of flash memory
// Called from the IR packet handler when it sees the PULL header in an incoming message

/* Here is the push packet payload structure...

    struct push_payload_t {                 // Response to a pull with the flash block we asked for
        uint8_t data[DOWNLOAD_PAGE_SIZE];   // An actual page of the flash memory
        uint8_t page;                       // The block number in this packet
        uint8_t page_checksum;              // Simple longitudinal checksum then inverted. This comes at the end so we can compute it on the fly.
    };

*/

void blinkos_blinkboot_sendPushFlash( uint8_t face , uint8_t page ) {


    if (irSendBegin( face )) {

        if (page & 1 ) {

            displayedRawPixelSet.rawpixels[face] = rawpixel_t( 255 , 0 , 255 );
        } else {

            displayedRawPixelSet.rawpixels[face] = rawpixel_t( 255 ,  160 ,  255 );

        }

        uint8_t computed_checksum=IR_PACKET_HEADER_PUSHFLASH;

        irSendByte( IR_PACKET_HEADER_PUSHFLASH );

        // This feels wrong. There really should be a flash pointer type.
        uint8_t *flashptr = ((uint8_t *) 0) + (page * DOWNLOAD_PAGE_SIZE);

        for(uint8_t i=0; i<DOWNLOAD_PAGE_SIZE; i++ ) {

            uint8_t b = pgm_read_byte( flashptr++ );

            irSendByte( b );

            computed_checksum+=b;

        }

        irSendByte( page );

        computed_checksum+=page;

        irSendByte( computed_checksum ^ 0xff );            // We invert the checksum on both sides to avoid matching on a string of 0's

        irSendComplete();

        Debug::tx('l');


    }    else {

        displayedRawPixelSet.rawpixels[face] = rawpixel_t( 0 , 255 , 255 );

    }


    next_pull_request_timer.set(PULL_REQUEST_TIMEOUT_AFTER_PUSH_MS);     // Give the last guy who asked a second to ask for the next page before we go to the next guy

}


static PROGMEM const uint8_t next_face[FACE_COUNT] = { 2 , 3 , 4 , 5 , 1 , 0 } ;

// Called instead of the game's loop when we are in game transmit mode

void blinkos_blinkboot_loop() {

    if ( next_pull_request_timer.isExpired()) {

        Debug::tx('R');

        displayedRawPixelSet.rawpixels[next_pull_request_target] = rawpixel_t( 255 , 255 , 0 );

        blinkos_blinkboot_sendPullRequest( next_pull_request_target );

        displayedRawPixelSet.rawpixels[next_pull_request_target] = rawpixel_t( 255 , 255 , 255 );

        next_pull_request_target = pgm_read_byte( next_face + next_pull_request_target);      // Skip to opposite side so we do not do adjacent faces. I think this is the most efficient way to  do this.

        next_pull_request_timer.set(PULL_REQUEST_PROBE_SPACING_MS);

    }

}

