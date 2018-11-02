// Support functions for being a bootloader server (able to send the current active game)


#include <avr/pgmspace.h>       // Functions for accessing flash

#include "blinkos.h"
#include "bootloader.h" // Get bootloader protocol
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

seed_payload_t pull_request_packet_payload;

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

