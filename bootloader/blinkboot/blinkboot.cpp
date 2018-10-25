/*

    This is the bootloader for the Move38 Blinks platform.
    More info at http://Move38.com

    It is VERY tight to fit into the 2K bootload space on the ATMEGA168PB chip.

    It has one job- look for an incoming "flash pull request" and, if seen, start downloading
    and programming the received code into the active area, and starts the new game when received.

    It is very streamlined to do just this.

    If it does not see the pull request, then it copies the built-in game down to the active
    game area at the bottom of the flash and starts it.

*/


#include <avr/pgmspace.h>
#include <stddef.h>     // NULL
#include <stdint.h>     // uintX_t

#include <util/crc16.h>     // For IR checksums
#include <util/atomic.h>         // ATOMIC_BLOCK

#include <avr/boot.h>           // Bootloader support

#include <avr/sleep.h>          // TODO: Only for development. Remove.

// TODO: Put this at a known fixed address to save registers
// Will require a new .section in the linker file. Argh.

#include "pixel.h"
#include "timers.h"
#include "utils.h"
#include "power.h"

#include "debug.h"

#include "callbacks.h"      // blinkcore calling into us - the timer callbacks & run()

#include "ir.h"
#include "blinkboot_irdata.h"

#include "coarsepixelcolor.h"

#define US_PER_TICK        256
#define MS_PER_SECOND     1000
#define MS_TO_TICKS( milliseconds ) ( milliseconds *  (MS_PER_SECOND / US_PER_TICK ) )

// This buys us 16 seconds of timer time
// https://www.google.com/search?q=(2%5E16+*+250us)&rlz=1C1CYCW_enUS687US687&oq=(2%5E16+*+250us)&aqs=chrome..69i57j6.21078j0j4&sourceid=chrome&ie=UTF-8

#define MS_PER_PULL_RETRY 250

uint16_t countdown_until_next_pull;

uint8_t retry_count;               // How many pulls have went sent without any answer?
                                   // Note that while still in listen mode we do not actually send any pulls
                                   // so if we do not get a pull request at all, then we also abort


#define GIVEUP_RETRY_COUNT 10     // Comes out to 2.5 secs. Give up if we do not see a good push in this amount of time.


// Below are the callbacks we provide to blinkcore

// This is called by timer ISR about every 512us with interrupts on.

void timer_256us_callback_sei(void) {

    if (countdown_until_next_pull) {
        countdown_until_next_pull--;
    }

}

// This is called by timer ISR about every 256us with interrupts on.

void timer_128us_callback_sei(void) {
    IrDataPeriodicUpdateComs();
}



#define MODE_LISTENING      0        // We have not yet seen a pull request
#define MODE_DOWNLOADING    1        // We are currently downloading on faces. gamechecksum is the checksum we are looking for.

uint8_t mode = MODE_LISTENING;       // This changes to downloading the first time we get a pull request

uint8_t download_face;            // The face that we saw the PR on


uint16_t download_checksum;           // The expected checksum of the game we are downloading. Set by first pull request.
uint8_t download_total_pages;         // The length of the game we are downloading in pages. Set by 1st pull request.
uint8_t download_next_page;           // Next page we want to get (starts at 0)

// TODO: These should get moved to a file that is shared between blinkboot and blinkos
#define DOWLONAD_MAX_PAGES 56     // The maximum length of a downloaded game

#define DOWNLOAD_PAGE_SIZE 128                 // Flash pages size for ATMEGA168PB

// These header bytes are chosen to try and give some error robustness.
// So, for example, a header with a repeating pattern would be less robust
// because it is possible something blinking in the environment might replicate it

#define IR_PACKET_HEADER_PULLREQUEST     0b01101010      // If you get this, then the other side is saying they want to send you a game
#define IR_PACKET_HEADER_PULLFLASH       0b01011101      // You send this to request the next block of a game
#define IR_PACKET_HEADER_PUSHFLASH       0b01011101      // This contains a block of flash code to be programmed into the active area

void processPendingIRPackets() {

    for( uint8_t f=0; f< IRLED_COUNT ; f++ ) {

        if (irDataIsPacketReady(f)) {

            uint8_t packetLen = irDataPacketLen(f);

            // Note that IrDataPeriodicUpdateComs() will not save a 0-byte packet, so we know
            // the buffer has at least 1 byte in it

            // IR data packet received and at least 2 bytes long

            uint8_t const *data = irDataPacketBuffer(f);

            // TODO: What packet type should be fastest check (doesn;t really matter THAT much, only a few cycles)

            if (*data== IR_PACKET_HEADER_PUSHFLASH ) {

                uint16_t packet_game_checksum = data[1] | ( data[2]<<8 );

                if (packetLen== (DOWNLOAD_PAGE_SIZE + 5 ) && packet_game_checksum == download_checksum  ) {        // Just an extra check that this is a valid packet?

                    // This is a valid push packet, and for the game we are downloading

                    uint8_t packet_page_number = data[3];

                    if ( packet_page_number == download_next_page) {        // Is this the one we are waiting for?

                        uint16_t packet_page_checksum_computed=0;

                        for(uint8_t i=4; i<4+DOWNLOAD_PAGE_SIZE;i++ ) {

                            packet_page_checksum_computed += data[i];

                        }

                        uint8_t packet_page_checksum_received = data[ 4 + DOWNLOAD_PAGE_SIZE ];

                        if (packet_page_checksum_received == packet_page_checksum_computed ) {

                            // We got a good packet, good checksum on data, and it was the one we needed, and it is the right game!!!

                            download_next_page++;

                            if (download_next_page == download_total_pages ) {

                                // TODO: Check the game checksum here

                                // We are down downloading!
                                // Here we would launch the downloaded game in the active area
                                // and somehow tell it to start sending pull requests on all faces but this one


                            }

                            // Blink GREEN/BLUE with each packet that gets us closer (out of order packets will not change the color)
                            setRawPixelCoarse( f , download_next_page & 1 ? COARSE_BLUE : COARSE_GREEN );

                        } else {        // if (packet_page_checksum_received != packet_page_checksum_computed )


                            setRawPixelCoarse( f , COARSE_RED );

                        }

                    } //  if ( packet_page_number == download_next_page)


                    // Force next pull to happen immediately and also start retry counting again since we just got a good packet.
                    cli();
                    countdown_until_next_pull=0;
                    sei();
                    retry_count=0;

                } else {          // (packetLen== (PAGE_SIZE + 5 ) && packet_game_checksum == download_checksum  )

                    setRawPixelCoarse( f , COARSE_RED );

                }

            } else if (*data== IR_PACKET_HEADER_PULLREQUEST ) {

                if (packetLen==5) {     // Extra check that it is valid

                    if ( mode==MODE_LISTENING ) {

                        download_total_pages = data[1];
                        // highestblock this face = data[2];
                        download_checksum    = data[3] | (data[4]<<8);

                        download_face = f;
                        //download_next_page=0;         // It inits to zero so we don't need this explicit assignment
                        mode = MODE_DOWNLOADING;

                        setRawPixelCoarse( f , COARSE_GREEN );

                    }

                }

            }  // Case out packet types

            irDataMarkPacketRead(f);

        }  // irIsdataPacketReady(f)

    }    // for( uint8_t f=0; f< IR_FACE_COUNT; f++ )

}

inline void copy_built_in_game() {

    // TODO: implement
    // Copy the built in game to the active area in flash

}


inline void sendNextPullPacket() {

    // Note that if we are blocked from sending becuase there is an incoming packet, we just ignore it
    // hopefully it is the packet we are waiting for. If not, we will time out and retry.

    if (irSendBegin( download_face )) {

        irSendByte( IR_PACKET_HEADER_PULLFLASH );

        irSendByte( download_next_page );

        irSendComplete();

    }

}

inline void try_to_download() {

    while (1) {

        processPendingIRPackets();       // Read any incoming packets looking for pulls & pull requests

        if ( countdown_until_next_pull ==0 ) {

            // Time to send next pull ?

            retry_count++;

            if (retry_count >= GIVEUP_RETRY_COUNT ) {

                // Too long. Abort to the built-in game and launch

                copy_built_in_game();

                return;

            }

            // We actually only do pulls if we have already seen a pull request that put us into download mode
            // if not, we do nothing and just wait for a pull request until the rety counter runs out.

            if (mode==MODE_DOWNLOADING) {       // Are we actively downloading?

                // Try sending a pull packet on the face we got the pull request from

                sendNextPullPacket();

            }

            // Set up counter to trigger next pull packet

            cli();
            countdown_until_next_pull = MS_TO_TICKS( MS_PER_PULL_RETRY );
            sei();

        }


    }


}


// This is the entry point where the blinkcore platform will pass control to
// us after initial power-up is complete

// We make this weak so that a game can override and take over before we initialize all the higher level stuff

void run(void) {

    // We only bother enabling what we need to save space

    pixel_init();

    ir_init();

    ir_enable();

    pixel_enable();

    setAllRawCorsePixels( COARSE_ORANGE );

    try_to_download();

    // TODO: Lanuch the active game (it will either be the newly downloaded game or the built in one)

    set_sleep_mode( SLEEP_MODE_PWR_DOWN );
    sleep_enable();
    sleep_cpu();

    //TODO: Jump to active game
}
