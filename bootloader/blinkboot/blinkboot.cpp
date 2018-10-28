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
#include <avr/boot.h>
#include <stddef.h>     // NULL
#include <stdint.h>     // uintX_t

#include <util/crc16.h>     // For IR checksums
#include <util/atomic.h>         // ATOMIC_BLOCK

#include <avr/boot.h>           // Bootloader support

#include <avr/sleep.h>          // TODO: Only for development. Remove.

#include "blinkboot.h"

// TODO: Put this at a known fixed address to save registers
// Will require a new .section in the linker file. Argh.

#include "pixel.h"
#include "timers.h"
#include "utils.h"
#include "power.h"

#warning only for dev so tile can sleep
#include "button.h"

#warning
#include "debug.h"

#include "callbacks.h"      // blinkcore calling into us - the timer callbacks & run()

#include "ir.h"
#include "blinkboot_irdata.h"

#include "coarsepixelcolor.h"

#define US_PER_TICK        256
#define MS_PER_SECOND     1000
#define MS_TO_TICKS( milliseconds ) ( milliseconds *  (MS_PER_SECOND / US_PER_TICK ) )

#define MS_PER_PULL_RETRY 300

uint16_t countdown_until_next_pull;

uint8_t retry_count;               // How many pulls have went sent without any answer?
                                   // Note that while still in listen mode we do not actually send any pulls
                                   // so if we do not get a pull request at all, then we also abort


// Set timer to go off imedeately and clear the pending retry count

static void triggerNextPull() {
    cli();
    countdown_until_next_pull=0;
    sei();
    retry_count=0;
}

// Reset the time to next retry time

static void resetTimer() {
    cli();
    countdown_until_next_pull = MS_TO_TICKS( MS_PER_PULL_RETRY );
    sei();
}


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


const char __attribute__((section("testburn"))) josh[] ="12345 josh is a nice guy";



void __attribute__((section("bls")))  burn_page_to_flash( uint8_t page , const uint8_t *data );

void __attribute__ ((noinline)) burn_page_to_flash( uint8_t page , const uint8_t *data ) {

    // TODO: We can probably do these better directly. Or maybe fill the buffer while we load the packet. Then we have to clear the buffer at the beginning.

    // First erase the flash page...

    // Fist set up zh to have the page in it....

    const char *address = josh;        // This should hit the page at 0x0d00 which we currently define as testburn...

    asm("nop");

    __asm__ __volatile__
    (
    ""
    :
    :
        "z" (address)
    );

    // Do the actual page erase under cover of int protection since the

//     // SPM has to come right after the STS
/*
    cli();
    __asm__ __volatile__                         \
    (                                            \
        "sts %0, %1\n\t"                         \
        "spm\n\t"
        :
        : "i" (_SFR_MEM_ADDR(__SPM_REG)),
          "r" ((uint8_t)(__BOOT_PAGE_ERASE))
        :
    );
    sei();
*/


    for(uint16_t i=0; i<128;i+=2 ){               // 64 words (128 bytes) per page
        cli();
        boot_page_fill(  i , ('J'<<8) | 'L' );
        sei();
    }

    cli();

    // We have to leave ints off for the full erase/write cycle here
    // until we get all the int handlers up into the bootloader section.

    boot_page_erase( address );

    boot_spm_busy_wait();

    boot_page_write( address );

    boot_spm_busy_wait( );

   boot_rww_enable();       // Enable the normal memory

    sei();                  // Don't want to have an int going to normal memory until it works again.


/*
    // Next fill up the buffer

    uint16_t *p = (uint16_t *) data ;

    for( uint8_t i = 0; i < DOWNLOAD_PAGE_SIZE; i+=2 ) {

        cli();
        boot_page_fill( FLASH_BASE_ADDRESS + i , *p );
        sei();

        p++;

    }

    // Next burn the page

    cli();
    boot_page_write( FLASH_BASE_ADDRESS );
    sei();
*/

}

#define MODE_LISTENING      0        // We have not yet seen a pull request
#define MODE_DOWNLOADING    1        // We are currently downloading on faces. gamechecksum is the checksum we are looking for.
#define MODE_DONE           2        // Download finished

uint8_t mode = MODE_LISTENING;       // This changes to downloading the first time we get a pull request

uint8_t download_face;            // The face that we saw the PR on


uint16_t download_checksum;           // The expected checksum of the game we are downloading. Set by first pull request.
uint8_t download_total_pages;         // The length of the game we are downloading in pages. Set by 1st pull request.
uint8_t download_next_page;           // Next page we want to get (starts at 0)


uint16_t received_program_checksum;             // The checksum that was in the original pull request packet
uint16_t program_computed_checksum;           // The running checksum we are computing as we receive the blocks
                                                // Both of these checksum include the block number with each block.
                                                // TODO: I think this protects us from a bad length in the original pull request?

void processPendingIRPackets() {

    for( uint8_t f=0; f< IRLED_COUNT ; f++ ) {

        if (irDataIsPacketReady(f)) {

            uint8_t packetLen = irDataPacketLen(f);

            // Note that IrDataPeriodicUpdateComs() will not save a 0-byte packet, so we know
            // the buffer has at least 1 byte in it

            // IR data packet received and at least 2 bytes long

            // TODO: Dig straight into the ir data structure and save these calls?

            const  blinkboot_packet *data = (const blinkboot_packet *) irDataPacketBuffer(f);

            // TODO: What packet type should be fastest check (doesn;t really matter THAT much, only a few cycles)

            if (data->header == IR_PACKET_HEADER_PUSHFLASH ) {

                // This is a packet of flash data

                if (packetLen== sizeof( push_payload_t ) + 1  ) {        // Push packet payload plus header byte. Just an extra check that this is a valid packet.

                    // Note we do not check for program checksum while downloading. If we somehow start downloading a different game, it should result
                    // in a bad total checksum when we get to the last block so We can do something other than jumping into the mangled flash image.

                    // This is a valid push packet

                    //TODO: Check that it came from the right face? Check that we are in download mode?

                    uint8_t packet_page_number = data->push_payload.page;

                    if ( packet_page_number == download_next_page) {        // Is this the one we are waiting for?

                        Debug::tx( packet_page_number );

                        // Compute the checksum on the data in the packet just received.

                        uint8_t data_computed_checksum=0;         // We have to keep this separately so we can fold it into the total program checksum

                        for(uint8_t i=0; i<DOWNLOAD_PAGE_SIZE;i++ ) {

                            data_computed_checksum += data->push_payload.data[i];

                        }

                        uint8_t packet_checksum_computed=data_computed_checksum;

                        packet_checksum_computed+=IR_PACKET_HEADER_PUSHFLASH;       // Add in the header

                        packet_checksum_computed+=packet_page_number;               // Add in the page number

                        if ( ( packet_checksum_computed ^ 0xff ) == data->push_payload.packet_checksum) {     // We invert the page checksum on both sides. This protects against all 0's being seen as OK.

                            program_computed_checksum += packet_checksum_computed + download_next_page ;            // At this point, download_page_next == packet_page_number. We use it cause maybe then it will be warm for the increment that comes next...

                            // We got a good packet, good checksum on data, and it was the one we needed, and it is the right game!!!

                            asm("nop");

                            burn_page_to_flash( packet_page_number , data->push_payload.data );

                            download_next_page++;

                            // Blink GREEN with each packet that gets us closer (out of order packets will not change the color)
                            setRawPixelCoarse( f , download_next_page & 1 ? COARSE_GREEN : COARSE_DIMGREEN );

                            if (download_next_page == download_total_pages ) {

                                // TODO: Check the game checksum here

                                // We are down downloading!
                                // Here we would launch the downloaded game in the active area
                                // and somehow tell it to start sending pull requests on all faces but this one

                                mode=MODE_DONE;

                                setAllRawCorsePixels( COARSE_BLUE );

                                cli();
                                boot_rww_enable ();         // make it so we can jump to the newly programmed flash
                                sei();

                            }


                        } else {         if ( ( packet_checksum_computed ^ 0xff ) == data->push_payload.packet_checksum)

                            setRawPixelCoarse( f , COARSE_RED );

                        }

                    } else { //  if ( ! packet_page_number == download_next_page)

                            setRawPixelCoarse( f , COARSE_ORANGE );

                    }


                    // Force next pull to happen immediately and also start retry counting again since we just got a good packet.

                    triggerNextPull();

                } else {          // (packetLen== (PAGE_SIZE + 5 ) && packet_game_checksum == download_checksum  )

                    setRawPixelCoarse( f , COARSE_RED );

                }

            } else if (data->header == IR_PACKET_HEADER_PULLREQUEST ) {             // This is other side telling us to start pulling new game from him

                if ( mode==MODE_LISTENING ) {

                    if ( packetLen== (sizeof( pull_request_payload_t ) + 1 ) ) {         // Pull request plus header byte

                        download_total_pages = data->pull_request_payload.pages;

                        download_checksum    = data->pull_request_payload.program_checksum;

                        #warning we dont check the checksum on pull requests yet
                        //computered_program_checksum = 0;      // This inits to 0 so we don't have to explicity set it

                        download_face = f;

                        //download_next_page=0;                 // It inits to zero so we don't need this explicit assignment

                        setAllRawCorsePixels( COARSE_OFF );

                        mode = MODE_DOWNLOADING;

                        setRawPixelCoarse( f , COARSE_BLUE );

                        triggerNextPull();

                    }  else { // Packet length wrong for pull request

                        setRawPixelCoarse( f , COARSE_RED );

                    }

                } // ( mode==MODE_LISTENING ) (ignore pull requests if not listening

            }  // if (data->header == IR_PACKET_HEADER_PULLREQUEST )

            irDataMarkPacketRead(f);

        }  // irIsdataPacketReady(f)

    }    // for( uint8_t f=0; f< IR_FACE_COUNT; f++ )

}

inline void copy_built_in_game() {

    // TODO: implement
    // Copy the built in game to the active area in flash

}


inline void sendNextPullPacket() {

    // Note that if we are blocked from sending because there is an incoming packet, we just ignore it
    // hopefully it is the packet we are waiting for. If not, we will time out and retry.

    if (irSendBegin( download_face )) {

        irSendByte( IR_PACKET_HEADER_PULLFLASH );

        irSendByte( download_next_page );

        irSendComplete();

    } else {

        // Send failed.

    }

}

inline void try_to_download() {

    while (1) {

        processPendingIRPackets();       // Read any incoming packets looking for pulls & pull requests

        if ( countdown_until_next_pull ==0 ) {

            // Time to send next pull ?

            #warning
            //retry_count++;

            if (retry_count >= GIVEUP_RETRY_COUNT ) {

                // Too long. Abort to the built-in game and launch

                copy_built_in_game();

                return;

            }

            // We actually only do pulls if we have already seen a pull request that put us into download mode
            // if not, we do nothing and just wait for a pull request until the retry counter runs out.

            if (mode==MODE_DOWNLOADING) {       // Are we actively downloading?

                // Try sending a pull packet on the face we got the pull request from
                sendNextPullPacket();

            }

            // Set up counter to trigger next pull packet

            resetTimer();

        }


    }


}


static void sleep(void) {

    pixel_disable();        // Turn off pixels so battery drain
    ir_disable();           // TODO: Wake on pixel
    button_ISR_on();        // Enable the button interrupt so it can wake us

    power_sleep();          // Go into low power sleep. Only a button change interrupt can wake us

    button_ISR_off();       // Set everything back to thew way it was before we slept
    ir_enable();

    pixel_enable();

}

// This is the entry point where the blinkcore platform will pass control to
// us after initial power-up is complete

// We make this weak so that a game can override and take over before we initialize all the higher level stuff

void run(void) {

    Debug::init();

    power_init();       // Get sleep functions

    // We only bother enabling what we need to save space

    pixel_init();

    ir_init();

    button_init();

    ir_enable();

    pixel_enable();

    // TODO: We don't need this in real boot loader. Just pass off to game once loaded.
    button_enable_pu();

    setAllRawCorsePixels( COARSE_ORANGE );

    while (1) {

        try_to_download();

        // TODO: Launch the active game (it will either be the newly downloaded game or the built in one)
        sleep();

    }
    //TODO: Jump to active game
}
