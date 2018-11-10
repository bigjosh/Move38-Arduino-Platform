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

#include <assert.h>

#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <stddef.h>     // NULL
#include <stdint.h>     // uintX_t

#include <util/crc16.h>     // For IR checksums
#include <util/atomic.h>         // ATOMIC_BLOCK

#include <avr/boot.h>           // Bootloader support

// TODO: Get rid of this when not needed
#define F_CPU 8000000
#include "util/delay.h"

#include <avr/sleep.h>          // TODO: Only for development. Remove.

#include "bootloader.h"

#include "jump.h"

// TODO: Put this at a known fixed address to save registers
// Will require a new .section in the linker file. Argh.

#include "pixel.h"
#include "timers.h"
#include "utils.h"
#include "power.h"

#warning only for dev so tile can sleep
#include "button.h"

#warning debug included
#include "debug.h"

#include "callbacks.h"      // blinkcore calling into us - the timer callbacks & run()

#include "ir.h"
#include "blinkboot_irdata.h"

#include "coarsepixelcolor.h"

#include "blinkos_headertypes.h"

#define DOWNLOAD_MAX_PAGES 0x30     // The length of a downloaded game. Currently we always download all block even for shorter games.
                                    // Turns out that flash stores 0xff in unused space, and 1 bits send quickly in our IR protocol so
                                    // maybe not worth the extra overhead of setting the length? Woud could always scan backwards though the 0xffs?


#define FLASH_PAGE_SIZE         SPM_PAGESIZE                     // Fixed by hardware

// The game size is the DOWNLOAD_MAX_PAGES * FLASH_PAGE_SIZE and must fit between FLASH_ACTIVE_BASE and FLASH_BUILTIN_BASE
// and also between FLASH_BUILTIN_BASE and the subbls section

#define FLASH_ACTIVE_BASE       ((uint8_t *)0x0000)     // Flash starting address of the active area
#define FLASH_BUILTIN_BASE      ((uint8_t *)0x1800)     // flash starting address of the built-in game
#define FLASH_BOOTLOADER_BASE   ((uint8_t *)0x3800)     // Flash starting address of the bootloader. Fixed by hardware.


#define US_PER_TICK        256UL
#define US_PER_MS         1000UL
#define TICK_PER_MS       ( US_PER_MS / US_PER_TICK)
#define MS_TO_TICKS( milliseconds ) ( milliseconds / MS_PER_TICK )

#define TICKS_PER_COUNT    256UL                                // Prescaller
#define US_PER_COUNT     (US_PER_TICK * TICKS_PER_COUNT)        // ~65ms per count
#define MS_PER_COUNT     (US_PER_COUNT/US_PER_MS)

#define MS_TO_COUNTS( ms )  ((ms / MS_PER_COUNT )+1)      // Always round up to longer delay

// Here lives all the bootloader state

// We keep three countdown timeouts running
// UPSTREAM - Last time we got a PUSH or SEED on source face- our parent is still there
// DOWNSTREAM- Last time we got a PULL or ACTIVE - we still have children
// SEED - When we will send a seed on the next face in the seed sequence. Can be sped up by finishing a PUSH or getting an ACTIVE or FINISHED.
//
// UPSTREAM (give up)
// If upstream expires, then we give up because without a parent there is no point in going on.
// Our children will make the same decision when they loose us.
// This has to be long enough that our upstream parent could be sending a PUSH on all
// 5 other faces before it gets around to us(?).
// Assume a PUSH takes 0.5 sec * 5 faces = 2.5 sec * 3 missed packets = 7.5 sec
// We reset the giveup timer any time we see a seed or productive push from our source, since this indicates
// he is still in control of the process. Non productive pushes don't count since they could be coming from,
// say, a blink with a different game or one that himself had an aborted download.
// When we get a LETSGO, then we force the giveup to timeout so that we dump out of the downloading
// loop and load the downloaded game (if it downloaded completely - else we load built-in).
//
// DOWNSTREAM (finished)
// If downstream expires, then we switch to sending FINISHED to our source and this percolates
// down the chain until the root's downstream expires, telling him everyone is finished.
// If we sent even one PUSH, then we know we have active children, so this only has to be long enough to
// make one full revolution without any answers (so SEED timeout * 6 faces for root or * 5 faces for others).
// To be safe against maybe missing a packet, we can make this timeout be  ( 3 * ) that time so we'd
// have to miss 3 packets in a row to miss an active child.
//
// SEED
// Timeout should be long enough for the receiver to do a pull after finishing a pending PUSH.
// Receiver always checks source after every push, so only has to be long enough for 1 push.
// In case of a missed packet, we will come back around.

// After getting a PUSH, the receiver should send a proactive PULL so the next time around the
// source will be able to skip the SEED. We also know the coast is clear right after we got the PUSH
// since the sender will be sending to other faces before coming back to us (to find the PULL waiting).

// TODO: Do these countdowns need to be volatile?
// I think we only process packets in the foreground, right?

volatile uint8_t countdown_startup;  // This is the initial time when we enter seed mode that we will wait to see our
                                     // first child before giving up


#define COUNTDOWN_STARTUP_ROOT_MS 5000UL
#define COUNTDOWN_STARTUP_ROOT_COUNT  MS_TO_COUNTS( COUNTDOWN_STARTUP_ROOT_MS )

#if (COUNTDOWN_STARTUP_ROOT_COUNT > 255 )     // So ugly. There must be a way to get the max value of a type at compile time?
    #error COUNTDOWN_STARTUP_ROOT_COUNT must fit into uint8_t
#endif

// Shorten this in production

#define COUNTDOWN_STARTUP_CHILD_MS 5000UL
#define COUNTDOWN_STARTUP_CHILD_COUNT  MS_TO_COUNTS( COUNTDOWN_STARTUP_CHILD_MS )

#if (COUNTDOWN_STARTUP_CHILD_COUNT > 255 )     // So ugly. There must be a way to get the max value of a type at compile time? Like maxvalueof(uint8_t)?
#error COUNTDOWN_STARTUP_CHILD_COUNT must fit into uint8_t
#endif

// This is the time a child will wait after they are not active until they get a GETSGO
// TODO: This might be too short for really big collections of blinks

#define COUNTDOWN_STARTUP_LETSGOWAIT_MS 10000UL
#define COUNTDOWN_STARTUP_LETSGOWAIT_COUNT  MS_TO_COUNTS( COUNTDOWN_STARTUP_LETSGOWAIT_MS )

#if (COUNTDOWN_STARTUP_LETSGOWAIT_COUNT > 255 )     // So ugly. There must be a way to get the max value of a type at compile time? Like maxvalueof(uint8_t)?
    #error COUNTDOWN_STARTUP_LETSGOWAIT_COUNT must fit into uint8_t
#endif


volatile uint8_t countdown_seed;  // When do give up on current face and step to the next one in sequence?
                                  // We have to wait a bit between seeds to give the other side a sec to answer if they want to pull
                                  // Note that the other side could be in the middle of sending up to 5 pushes
                                  // so it might take a while to get back to us


#define COUNTDOWN_SEED_MS    70UL  // Long enough that they can see our initial seed and get back to us with a PULL

#define COUNTDOWN_SEED_COUNT MS_TO_COUNTS( COUNTDOWN_SEED_MS )

#if (COUNTDOWN_SEED_COUNT > 255 )     // So ugly. There must be a way to get the max value of a type at compile time?

    #error COUNTDOWN_SEED_COUNT must fit into uint8_t
#endif

// ACTIVE countdown keep track if we have any downstream people from us
// (that is direct children PULLing from us, or grandchildren PULLing from our children, etc)
// We reset this each time we get any PULL- even a PULL for a packet we don't have.
// We also reset at the end of a PUSH since a PUSH takes a while.
// Children who are done downloading themselves but have people downloading from them will send us
// a PULL past the end of the image to tell us that they are still alive.
// That empty PULL is triggered by the retry timer, so we should wait for at least 2xretry timeout
// and (maybe?) 5x3xthe max time for a push to account for the case when child is PUSHing
// on all open faces?
// to allow for missed packets before assuming that there is no one active on a face.

volatile uint8_t countdown_active;


#define COUNTDOWN_ACTIVE_MS    1000UL // This should be long enough that we can make it all the way around
                                      // all faces sending SEED a couple of times and not getting any answer, so
                                      // SEED timeout * 6 * 3.
                                      // Must also be long enough that if one of those faces was in the middle of a PUSH
                                      // then it has time to finish it and then reply with ACTIVE.

                                      // Keep in mind that anytime we send a PUSH that means we got a PULL
                                      // so we are active.

// Actually needs to be move than 63ms to be more than 2 counts or else bad timing could maybe one count aslias too quick
#define COUNTDOWN_ACTIVE_COUNT MS_TO_COUNTS( COUNTDOWN_ACTIVE_MS )

#if (COUNTDOWN_ACTIVE_COUNT > 255 )     // So ugly. There must be a way to get the max value of a type at compile time?
    #error COUNTDOWN_ACTIVE_COUNT must fit into uint8_t
#endif



volatile uint8_t countdown_retry;       // This is reset every time we get something from our source if we are a node
                                        // (so practically speaking either a SEED or PUSH)
                                        // If it expires, then it has been a while since we got a push so
                                        // either PUSH or PULL timed out, or we sent a PULL that was for a block
                                        // the source did not have yet.
                                        // If we still need blocks, then we send another PULL to kick start things again and
                                        // this should not collide because the source never sends anything unsolicited after the SEED.
                                        // If we do not need blocks, but we are still active (we got a PULL from a child recently)
                                        // then we send a PULL to our souce with block==total_blocks which will not result in a PUSH
                                        // but just lets the source we are still active.



#define COUNTDOWN_RETRY_MS    200UL    // Shorter give faster recovery, and duped PULLs are benign except if they collide with PUSHes.

// all faces sending SEED a couple of times and not getting any answer, so
// SEED timeout * 6 * 3.
// Must also be long enough that if one of those faces was in the middle of a PUSH
// then it has time to finish it and then reply with ACTIVE.

// Keep in mind that anytime we send a PUSH that means we got a PULL
// so we are active.


// Actually needs to be move than 63ms to be more than 2 counts or else bad timing could maybe one count aslias too quick
#define COUNTDOWN_RETRY_COUNT MS_TO_COUNTS( COUNTDOWN_RETRY_MS )

#if (COUNTDOWN_RETRY_COUNT > 255 )     // So ugly. There must be a way to get the max value of a type at compile time?
    #error COUNTDOWN_RETRY_COUNT must fit into uint8_t
#endif



/*
typedef uint32_t millis_t;
static volatile millis_t millisCounter=1;           // How many milliseconds since startup?
*/

// Below are the callbacks we provide to blinkcore

// This is called by timer ISR about every 512us with interrupts on.

// This function is not reentrant unless we are doing something very wrong,
// so ok to test and then update these values without worrying about corruption.

void timer_256us_callback_sei(void) {

    static uint8_t tick_prescaller;     // Divide ticks /256
                                        // We need this extra divider so that our timeout counters fit into a single byte
                                        // otherwise we need to worry about interrupts coming in the middle of updates.

    if ( --tick_prescaller == 0 ) {      // Pre-decement so the flag will already be set for comparison

        // TODO: Make a custom function that decrements and then increments if past zero
        // to save a compare to zero?

        if (countdown_seed) {
            countdown_seed--;
        }

        if (countdown_startup) {
            countdown_startup--;
        }

        if (countdown_active) {
            countdown_active--;
        }

        if (countdown_retry) {
            countdown_retry--;
        }

        /*
        millisCounter+=1;      // 62ms per prescalled tick
        tick_prescaller=4;
        */
    }

}


// This is called by timer ISR about every 256us with interrupts on.

void timer_128us_callback_sei(void) {
    IrDataPeriodicUpdateComs();
}


// The FLASH_BOOTLOADER_BASE must be defined as the start of the .text segment when compiling the bootloader code.
// Note that the linker doubles the value passed, so the .text should start at 0x1c00 from the linker's args

// THIS MUST LIVE IN THE BOOTLOADER SECTION OF FLASH

static void __attribute__ ((noinline)) burn_page_to_flash( uint8_t page , const uint8_t *data ) {

    // TODO: We can probably do these better directly. Or maybe fill the buffer while we load the packet. Then we have to clear the buffer at the beginning.

    // First erase the flash page...

    // Fist set up zh to have the page in it....

    uint16_t address = (page * 128);        // This should hit the page at 0x0d00 which we currently define as testburn...


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


    // We write in words not bytes

    // TODO: We can do this much faster in ASM since we can
    // avoid reloading zero_reg every pass and use post increment for Z and stuff like that.

    // Anything that has an SPM must have cli/sei around it because the SPM much come
    // 4 cycles after the enable bit interlock before it.

    const uint16_t *ptr = (const uint16_t *) data;

    for(uint16_t i=0; i<128;i+=2 ){               // 64 words (128 bytes) per page
        cli();
        boot_page_fill(  i , *ptr++ );
        sei();
    }


    cli();
    boot_page_erase( address );
    sei();

    boot_spm_busy_wait();

    // NOte that the bottom bits must be 0 for the write command. :/

    cli();
    boot_page_write( address );
    sei();

    boot_spm_busy_wait( );

    cli();
    boot_rww_enable();       // Enable the normal memory
    sei();

    //sei();                  // Don't want to have an int going to normal memory until it works again.


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

static uint16_t checksum_flash_page( uint8_t page ) {

    uint16_t checksum=0;

    uint8_t *p = FLASH_ACTIVE_BASE + ( page *FLASH_PAGE_SIZE );

    for( uint8_t i=0; i < FLASH_PAGE_SIZE; i++ ) {

        checksum += pgm_read_byte( p++ );

    }

    checksum += page;

    return checksum;

}

static uint8_t checksum_128byte_RAM_buffer( const uint8_t *buffer ) {
    uint8_t data_computed_checksum=0;         // We have to keep this separately so we can fold it into the total program checksum

    for(uint8_t i=0; i<FLASH_PAGE_SIZE;i++ ) {

        data_computed_checksum += *(buffer++);

    }

    return data_computed_checksum;
}

//static uint8_t __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) push_packet_payload_checksum( const push_payload_t &push_payload );

static uint8_t push_packet_payload_checksum( const push_payload_t &push_payload ) {

    uint8_t checksum = checksum_128byte_RAM_buffer( push_payload.data );  // Start with the checksum of the data part

    checksum+=ir_packet_header_enum::PUSH; // Add in the header

    checksum+= push_payload.page;          // Add in the page number. This protects against out of order packets.

    return checksum ^ 0xff;                // Invert all bits to protect again all 0's


}


// Send response to a pull message that contains one page of flash memory
// Called from the IR packet handler when it sees the PULL header in an incoming message

/* Here is the push packet payload structure...

    struct push_payload_t {                 // Response to a pull with the flash block we asked for
        uint8_t data[DOWNLOAD_PAGE_SIZE];   // An actual page of the flash memory
        uint8_t page;                       // The block number in this packet
        uint8_t page_checksum;              // Simple longitudinal checksum + page#,  then inverted. This comes at the end so we can compute it on the fly.
    };

*/

//static void __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) send_push_packet( uint8_t face , uint8_t page );


static void send_push_packet( uint8_t face , uint8_t page ) {

    if (irSendBegin( face )) {

        uint8_t computed_checksum= ir_packet_header_enum::PUSH;

        irSendByte( ir_packet_header_enum::PUSH );

        // This feels wrong. There really should be a flash pointer type.
        uint8_t *flashptr = ((uint8_t *) 0) + (page * FLASH_PAGE_SIZE);

        for(uint8_t i=0; i<FLASH_PAGE_SIZE ; i++ ) {

            uint8_t b = pgm_read_byte( flashptr++ );

            irSendByte( b );

            computed_checksum+=b;

        }

        irSendByte( page );

        computed_checksum+=page;            // Add in the page number for some out of order protection

        irSendByte( computed_checksum ^ 0xff );            // We invert the checksum on both sides to avoid matching on a string of 0's

        irSendComplete();

    }

}



// TODO: Precompute this whole packet at compile time and send as one big chunk

//static void __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) send_seed_packet( uint8_t face , uint8_t pages , uint16_t program_checksum );

static void send_seed_packet( uint8_t face , uint8_t pages , uint16_t program_checksum ) {

   if (irSendBegin( face )) {

       irSendByte( ir_packet_header_enum::SEED );

       uint8_t computed_checksum=ir_packet_header_enum::SEED;

       irSendByte( pages );

       computed_checksum += pages;

       uint8_t program_checksum_low = (uint8_t) program_checksum;

       irSendByte( program_checksum_low );

       computed_checksum += program_checksum_low;

       uint8_t program_checksum_high = (uint8_t) program_checksum >> 8;     // TODO: Check that this compiles to just accessing the high byte

       irSendByte( program_checksum_high );

       computed_checksum += program_checksum_high;

       irSendByte( computed_checksum ^ 0xff );          // Invert bits to detect all 0 errors

       irSendComplete();

   }


}

//uint16_t __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) calculate_active_game_checksum();

uint16_t calculate_active_game_checksum() {

    uint16_t checksum=0;

    for( uint8_t page=0; page< DOWNLOAD_MAX_PAGES ; page++ ) {

        checksum += checksum_flash_page( page );
    }

    return checksum;

}


#define SOURCE_FACE_NONE    (FACE_COUNT)            // No source face, we are the root.

uint8_t download_source_face;         // The face that we saw the first seed on and we are now downloading from
                                      // or source_face_none if we are the root
                                      // Only valid if download_total_pages > 0

uint8_t download_total_pages;         // The length of the game we are downloading in pages. Init to 0. Set by 1st pull request.

uint8_t download_next_page;           // Next page we want to get
                                      // 0 means we have not gotten a good push yet


uint16_t active_program_checksum;     // The total program checksum for the loaded or loading active program
                                      // Valid after built in game copied, or after a seed received


uint8_t last_seed_face;               // The last face we sent a seed packet to. Note that we skip around so that adjacent tiles can maybe seed form each other.

// This is triggered by us getting a push or seed packet.
// If we get a seed then we know that the other side is waiting for us to pull
// so should not be a collision.

//static void __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) send_pull_packet( uint8_t face , uint8_t next_page);

static void send_pull_packet( uint8_t face , uint8_t next_page) {

    if (irSendBegin( face )) {

        irSendByte( ir_packet_header_enum::PULL);

        irSendByte( next_page);

        irSendComplete();

        } else {

        // Send failed.

    }

}

static uint8_t check_pull_packet(  const blinkboot_packet *pull_packet , uint8_t packet_len ) {

    if (pull_packet->header != ir_packet_header_enum::PULL) {
        return 0;
    }

    // Right packet length?

    if ( packet_len != sizeof( pull_payload_t) + 1   ) {        // header byte + inverted check byte
        return 0;
    }


    // Pull packet is simple, no checks needed


    return 1;
}



static void send_nopayload_packet( uint8_t face, uint8_t header_byte ) {

    if (irSendBegin( face )) {

        irSendByte(  header_byte );

        irSendByte( header_byte ^ 0xff );      // Invert header byte as checksum

        irSendComplete();

        } else {

        // Send failed.

    }

}

static uint8_t check_nopayload_packet( const blinkboot_packet *push_packet , uint8_t packet_len , uint8_t header_byte ) {

    // Check that the payload data is intact

    if ( push_packet->header != header_byte ) {
        return 0;
    }

    // Right packet length?


    if ( packet_len != 2 ) {        // header byte + inverted check byte
        return 0;
    }

    if ( push_packet->no_payload.inverted_check_byte != (header_byte ^ 0xff) ) {

        return 0;

    }

    // TODO: check the packet checksum that protects the page number and header

    return 1;
}

static uint8_t check_active_packet( const blinkboot_packet *push_packet , uint8_t packet_len ) {

    return check_nopayload_packet( push_packet , packet_len , ir_packet_header_enum::ACTIVE);

}

static void send_active_packet( uint8_t face ) {

    send_nopayload_packet( face , ir_packet_header_enum::ACTIVE );

}

static uint8_t check_letsgo_packet( const blinkboot_packet *push_packet , uint8_t packet_len ) {

    return check_nopayload_packet( push_packet , packet_len , ir_packet_header_enum::LETSGO);

}


static void send_letsgo_packet( uint8_t face ) {

    send_nopayload_packet( face , ir_packet_header_enum::LETSGO );

}

// Send repeatedly on all faces

//static void __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) send_letsgo_storm();

static void send_letsgo_storm() {

    for( uint8_t t=0; t< 3 ; t++ ) {

        // Give it 3 tries to account for lost packets and collisions
        // THis also has to last long enough that we don't end up hearing a
        // new seed from an adjacent blink after we come back up with the new game.

        for( uint8_t face=0; face < FACE_COUNT ; face++ ) {

            if (face != download_source_face) {

                send_letsgo_packet( face );

            }

        }
    }

}


// Returns 1 if this packet passes length and checksum tests for valid push packet

static uint8_t __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) check_push_packet();

static uint8_t check_push_packet( const blinkboot_packet *push_packet , uint8_t packet_len ) {

    if ( push_packet->header != ir_packet_header_enum::PUSH ) {

        return 0;

    }

    // Right packet length?

    if ( packet_len != sizeof( push_payload_t ) + 1  ) {        // +1 to include the header byte
        return 0;
    }

    // Check that the payload data is intact

    if (  push_packet_payload_checksum( push_packet->push_payload )  != push_packet->push_payload.packet_checksum ) {
        return 0;
    }

    // TODO: check the packet checksum that protects the page number and header

    return 1;
}

// Returns 1 if this packet passes length for valid active packet

//static uint8_t __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) check_seed_packet( const blinkboot_packet *seed_packet , uint8_t packet_len );

static uint8_t check_seed_packet( const blinkboot_packet *seed_packet , uint8_t packet_len ) {

    if ( seed_packet->header != ir_packet_header_enum::SEED ) {

        return 0;

    }

    if ( packet_len !=  (sizeof( seed_payload_t ) + 1 )  ) {        // +1 to include the header byte
        return 0;
    }

    // TODO: Check packet checksum here! A bad seed could mess things up.

    return 1;
}

uint8_t engagedbits;
                                    // bitmask to save space and checking for 0 is fast
                                    // Have we ever seen a pull on this face?
                                    // If so, then we do not need to send it seeds
                                    // it is responsible for its own pulls and retries
                                    // We also keep track of this so root can wait until it sees
                                    // at least one download before giving up to give time to
                                    // place the blink down into an arrangement.
                                    // The only unsolicited packet we will ever send is a LETSGO
/*

To be able to immediately detect when we are finished, we need some way to make sure
we just have not yet seen the other side yet. This could be a per-side timeout
that starts at an initial high value to allow for first contact and then is reset to a short value
once we know there is someone there. It could also be a fixed try strategy where were try a face X
times and if we get no answer then we assume no one there.

uint8_t finishedbits;               // bitmask if this side and all of its issue are finished downloading
                                    // You can tell if all of your sides are finished by XORing
                                    // engagedbits with finished bits

*/


uint8_t letsgo_flag;                // If we ever see a LETGO packet, we set this so
                                    // the main event loop can dump and send out own LETGO storm
                                    // This seems ineligant, but we can maybe fold this into
                                    // other state later

static void __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) processInboundIRPacketsOnFace( uint8_t f ) ;

static void processInboundIRPacketsOnFace( uint8_t f ) {

    if (irDataIsPacketReady(f)) {

        uint8_t packetLen = irDataPacketLen(f);

        // Note that IrDataPeriodicUpdateComs() will not save a 0-byte packet, so we know
        // the buffer has at least 1 byte in it

        // IR data packet received and at least 2 bytes long

        // TODO: Dig straight into the ir data structure and save these calls?

        const  blinkboot_packet *data = (const blinkboot_packet *) irDataPacketBuffer(f);

        Debug::tx('g');
        Debug::tx( data->header );

        // TODO: What packet type should be fastest check (doesn't really matter THAT much, only a few cycles)

        if ( check_push_packet( data , packetLen ) ) {

            // This is a packet of flash data

            // Note we do not check for program checksum while downloading. If we somehow start downloading a different game, it should result
            // in a bad total checksum when we get to the last block so We can do something other than jumping into the mangled flash image.

            // This is a valid push packet

            //TODO: Check that it came from the right face? Check that we are in download mode?

            uint8_t packet_page_number = data->push_payload.page;

            // Only accept a PUSH that...
            // 1. Comes from our source
            // 2. Is not past the end of active game memory (just to be 100% sure we never overwrite built in game)

            if ( f == download_source_face &&  packet_page_number < download_total_pages) {

                Debug::tx( 'h' );
                Debug::tx( packet_page_number );

                // Reply back quickly to keep things moving - even if this is not the one we are waiting for

                if (  packet_page_number == download_next_page) {        // Is this the one we are waiting for?

                    // We got a good packet, good checksum on data, and it was the one we needed!!!

                    // We do the actual burn. It can take ~3 milliseconds.

                    Debug::tx( 'B' );
                    burn_page_to_flash( packet_page_number , data->push_payload.data );

                    download_next_page++;

                    if (download_next_page == download_total_pages ) {

                        // We are done downloading!
                        // But we have to tell our source that people downstream from us are still active

                        setAllRawCorsePixels( COARSE_GREEN );

                        // If we are not active, then we send nothing so our upstream source will
                        // eventually active timeout as well

                        // We are now done, but to coordinate starting the downloaded game with everyone
                        // else, we will wait this long for a LETSGO from the root
                        countdown_startup = COUNTDOWN_STARTUP_LETSGOWAIT_COUNT;

                    } else {

                        // Blink with each packet that gets us closer (out of order packets will not change the color)
                        setRawPixelCoarse( f , download_next_page & 1 ? COARSE_GREEN1 : COARSE_GREEN2);

                        // we are still downloading, so don't time out as long as we are getting valid push packets that get us closer
                        // to being done


                        // Grease the wheels to get get next push quickly and continue the download

                    } // if (download_next_page == download_total_pages )

                }  else {  //   if (  packet_page_number != download_next_page)

                    // We got a PUSH, but not the page we were looking for, so ask again
                    // next time SEED comes around
                    // TODO: This is really an errant case, do we need to check it?

                     Debug::tx( 'X' );

                    setRawPixelCoarse( f , COARSE_RED );

                }

                Debug::tx( 'L' );
                Debug::tx( download_next_page );
                send_pull_packet( f , download_next_page );

                countdown_retry = COUNTDOWN_RETRY_COUNT;        // start counting down in case we don't get a reply
                countdown_active = COUNTDOWN_ACTIVE_COUNT;      // Stay alive as long as we keep getting PUSHes.
                // source will see this and get back to us on the next pass though.

                // Note that this pull will tell the source that we are active

                /*

                // As long as our source is sending us valid push packets, we know he is alive
                // and so we should not give up
                countdown_giveup = COUNTDOWN_GIVEUP_COUNT;

                */

            } //  if ( packet_page_number == download_next_page) {


        } else if (check_seed_packet( data , packetLen) ) {             // This is other side telling us to start pulling new game from him

            Debug::tx('e');
            Debug::tx( '0' + f );

            //setRawPixelCoarse( f , COARSE_CYAN );

            //setAllRawCorsePixels( COARSE_YELLOW );

            if ( download_total_pages==0 ) {           // Make sure we don't already have a source, and we are not root

                Debug::tx('-');

                // We found a good source - lock in and start downloading

                download_total_pages = data->seed_payload.pages;

                active_program_checksum = data->seed_payload.program_checksum;

                download_source_face = f;

                // This sets us up to send our first seed as far away from the source as possible
                // for better distribution.
                last_seed_face = f;

                Debug::tx('L');
                Debug::tx(download_next_page);

                setRawPixelCoarse( f , COARSE_GREEN1 );
                send_pull_packet( f , download_next_page );
                countdown_retry = COUNTDOWN_RETRY_COUNT;        // start counting down incase we don't get a reply

                // We just started SEEDing, so wait a minimum time to make sure we have a chance
                // to see all our neighbors before we give up
                countdown_startup = COUNTDOWN_STARTUP_CHILD_COUNT;

            }   //  if ( download_total_pages == 0 ) - we need a source?

            // We ignore SEEDs after the first one we see. The source should stop sending them once
            // he sees our PULL. Adjacent blinks will keep sending them.

        }  else if ( check_pull_packet( data , packetLen ) ) {

            uint8_t requested_page = data->pull_payload.page;

            Debug::tx('l');
            Debug::tx( requested_page );
            Debug::tx( '0' + f );


            if (requested_page<download_next_page) {            // Do we even have the next page ready?
                                                                // Will also fail if child is asking for page past the end
                                                                // which is a marker that says "I am done downloading, but
                                                                // there are still people downloading from my subtree so
                                                                // stay active".

                // Blink with each packet that gets us closer (out of order packets will not change the color)

                setRawPixelCoarse( f , requested_page & 1 ? COARSE_GREEN1 : COARSE_GREEN2);

                Debug::tx('H');
                Debug::tx( requested_page);

                send_push_packet( f , requested_page  );


            } else {

                Debug::tx('>');

            }

            // If we get a PULL then we know this is our child
            SBI( engagedbits , f );               // No need to ever send another SEED to this face

            if (last_seed_face==f) {    // Are we waiting for this initial PULL?
                countdown_seed=0;       // No need to wait - go on to next face.
            }

            countdown_active = COUNTDOWN_ACTIVE_COUNT;  // The blink on this face pulled so he is actively downloading himself so by definition it is active
                                                        // Even if we don't have the one he needs yet.
                                                        // He will try a PULL again next time we pass him with a SEED



        } else if (check_letsgo_packet( data , packetLen ) ) {

            Debug::tx('o');

            if ( f == download_source_face ) {      // Only expect (and accept) this from source

                letsgo_flag =1;             // This will cause the active loop to dump on next pass
                countdown_startup = 0;       // This will cause the giveup loop to dump on next pass

                // We will check if the download worked
                // If it did, then we will send a letsgo storm and jump to new game
                // if not, load built-in and jump to that
            }
        }

        irDataMarkPacketRead(f);

    }// irIsdataPacketReady(f)

}


// Does not keep interrupts on

void copy_built_in_game_to_active() {

    return;

    uint16_t *s_addr = (uint16_t *) FLASH_BUILTIN_BASE;
    uint16_t *d_addr = (uint16_t *) FLASH_ACTIVE_BASE;

    cli();      // Can't let interrupts come before SPM op

    for( uint8_t page = 0 ; page < DOWNLOAD_MAX_PAGES ; page++ ) {

        // We write in words not bytes

        // TODO: We can do this much faster in ASM since we can
        // avoid reloading zero_reg every pass and use post increment for Z and stuff like that.

        // Anything that has an SPM must have cli/sei around it becuase the SPM much come
        // 4 cycles after the enable bit interlock bnefore it.

        for(uint16_t i=0; i<128;i+=2 ){               // 64 words (128 bytes) per page
            boot_page_fill(  i , *s_addr++ );
        }


        boot_page_erase( d_addr );

        boot_spm_busy_wait();

        // Note that the bottom bits must be 0 for the write command. :/

        boot_page_write( d_addr );

        boot_spm_busy_wait( );

        d_addr+=64;         // 128 bytes, but this is a word *

    }

    boot_rww_enable();       // Enable the normal memory

    sei();

}


// Move the interrupts up to the bootloader area
// Normally the vector is at 0, this moves it up to 0x3400 in the bootloader area.

//void __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) move_interrupts_to_bootlader();

void move_interrupts_to_bootlader(void)
{
    uint8_t temp;
    /* GET MCUCR*/
    temp = MCUCR;

    // No need to cli(), ints already off and IVCE blocks them anyway

    /* Enable change of Interrupt Vectors */
    MCUCR = temp|(1<<IVCE);
    /* Move interrupts to Boot Flash section */
    MCUCR = temp|(1<<IVSEL);

}


// Move the interrupts back to 0x0000 (not in the bottom of the bootloader) 

//void __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) move_interrupts_to_bootlader();

void move_interrupts_to_base(void)
{
    uint8_t temp;
    /* GET MCUCR*/
    temp = MCUCR;

    // No need to cli(), ints already off and IVCE blocks them anyway

    /* Enable change of Interrupt Vectors */
    MCUCR = temp|(1<<IVCE);
    /* Move interrupts to Boot Flash section */
    MCUCR = temp & ~(1<<IVSEL);

}


// List of faces in order to seed. We stager them so that hopefully we can make gaps so that adjacent
// blinks can download from each other rather than from us.

// So, for example, if we are on face 0 then next we will jump to face 2. After 2 comes 4. See? Cute, right?

// For the seed where we are sending on all faces, this works well since we never hit two adjacent sides
// in a row. This gives the opportunity for the first blink we hit to then seed to it's sideways neighbor
// and thereby double throughput compared to us having to sequentially send to both of them.

// There is a huge hack here. Notice there are only 6 faces, but 7 entries in this skip table
// The last one happens to land at index SOURCE_FACE_NONE. When a tile is the root, it's source_face
// is set to SOURCE_FACE_NONE. When a tile starts a SEED sending round, it starts with its source_face
// and it starts sending SEEDs to faces in order based on the table, so the root will start at that last index
// and naturally jump to face 0 and then pick up. Just works and saves logic for the cost on one flash byte.

// If you start here, you go to the one below it                  0   1   2   3   4   5
static PROGMEM const uint8_t next_stagered_face[FACE_COUNT+1] = { 2 , 5 , 4 , 0 , 1 , 3  } ;

 // If we do not already have an active game (as indicated by download_next_page and download_total_pages), then
 // wait for a seed packet and start downloading. If we do already have an active game, just start seeding.

 // TODO: Jump into all these to save all that pushing and poping. We will never be returning, so all for naught.

 // Call with 1 to copy the built in game to active area

void __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) download_and_seed_mode( uint8_t we_are_root );

// We always exit by jumping to active game, so no need for a RET at the end or to save regs
void __attribute__((naked)) download_and_seed_mode( uint8_t we_are_root );

void download_and_seed_mode( uint8_t we_are_root ) {


    download_source_face = SOURCE_FACE_NONE;        // Set to the source face when we get initial seed
                                                    // or stays at SOURCE_FACE_NONE if we_are_root


    if (we_are_root) {

        active_program_checksum = calculate_active_game_checksum();

        download_total_pages = DOWNLOAD_MAX_PAGES;       // For now always send the whole thing. TODO: Base this on actual program length if shorter?
        download_next_page = download_total_pages;    // Special value of next > total signals totally downloaded, so just seed mode

        // last_seed_face = 0;                 // As root we have to start someplace, really doesn't matter where.
        // Don't need to explicitly set to 0 since all variables cleared at start

    } else {

        download_total_pages = 0;           // We need to download
        download_next_page = 0;             // We need to download

        // We don't need to init last_seed_face when we are downloading since it will
        // get set when we get our first SEEED packet.

        // Also do not need to init active_program_checksum which will get set on first SEED
    }


    setAllRawCorsePixels( COARSE_DIMBLUE );

    // The giveup timeout will keep root awake until he sees activity on one of the faces
    // The giveup timeout will keep child awake until he sees the first seed on one of the faces

    // Note that the give up on the root is never reset so he will give up as soon as giveup expires and
    // there are no active children. Cool right?


    // Start by sending 3 rounds of seed packets to everyone


    countdown_startup = COUNTDOWN_STARTUP_ROOT_COUNT;
    countdown_active = COUNTDOWN_ACTIVE_COUNT;      // Assume no active downstream blinks until they tell us so
    countdown_seed   = 0;                           // Seed initial SEED round immediately. Will retry at COUNTDOWN_SEED intervals

   // Keep going until either...
   // 1. we are root and none of our children are active anymore (giveup will expire while we are downloading to them and never get reset)
   // 2. we get a LETSGO which sets  giveup=0


    while ( ( countdown_startup || countdown_active ) && !letsgo_flag ) {

            // Scan all the faces for new incoming messages
            // Note that some incoming messages generate outgoing messages, like PULL making a PUSH

            for( uint8_t f=0; f<FACE_COUNT; f++ ) {

                // Always check if we got anything new on the source face so we have the most
                // to offer our requesters

                if ( download_source_face != SOURCE_FACE_NONE ) {       // We are not the root and we are downloading

                    // If we are the source, then no point checking the source, and we will never
                    // send a PULL so never need any kind of retry

                    // Note that checking the source face is always quick because the longest packet
                    // we can send is a PULL if we get a PUSH. If we do send a PULL then it will reset the counter_retry.
                    processInboundIRPacketsOnFace( download_source_face );

                    // Check if we need to send a retry on each pass because a single PUSH
                    // can take 300us so if we waited until all sides were serviced, we could
                    // expire our active time with our source.

                    // We checked above that download_source_face != SOURCE-FACE_NONE, so we know that
                    // countdown_retry was initialization when we sent first PULL.

                    // We only send a PULL if we need a PUSH or if have seen a PULL from out children lately
                    // This is how the root knows when everything is done because his active will time out

                    if (countdown_retry==0 && (download_next_page<download_total_pages || countdown_active )) {

                        // We haven't gotten a PUSH in a while so send a PULL. This does two things...
                        // 1. If we need more blocks, this will kickstart the transfer again
                        // 2. If we are done downloading, but we land here then we know that someone is
                        //    still downloading form us (we are active), so this PULL packet will tell the source
                        //    that we are still active and then he will stay active and his source will also
                        //    on down the line. Since the download_next_page==download_total_pages this will not
                        //    generate a PUSH reply.

                        Debug::tx('R');
                        Debug::tx('L');
                        Debug::tx(download_next_page);

                        send_pull_packet( download_source_face , download_next_page );
                        countdown_retry = COUNTDOWN_RETRY_COUNT;

                    } //  if (countdown_retry==0)

                } //   if ( download_source_face != SOURCE_FACE_NONE ) - we are not the root

                // Now check the face we are on for anything coming in on the faces
                // Sure this will redundantly hit the source face again when we are a child, but thats ok.
                processInboundIRPacketsOnFace( f );

            }   // for( uint8_t f=0; f<FACE_COUNT; f++ ) - checking non-soruce faces for incoming messages

            // Sending probe seeds are a low priority, so we only do it after we have checked (and possibly pushed to)
            // all the engaged faces

            if (countdown_seed==0 && download_next_page > 0 ) {      // Don't bother sending seeds until we have something to share

                // Visually clear the last blue seed indicator is there was one on the last face

                if ( last_seed_face != download_source_face && !TBI( engagedbits , last_seed_face ) ) {
                    setRawPixelCoarse( last_seed_face , COARSE_OFF );                          // Clear previous seed indicator if we are sending SEEDs there
                }


                // Advance to next face in the staggered probe sequence
                last_seed_face = pgm_read_byte( next_stagered_face + last_seed_face );      // Get next staggered face to send on
                                                                                            // Note that last_Seed_face will start at SEED_FACE_NOONE
                                                                                            // on the root and source_face on children.
                                                                                            // There is a hack in next_stagered_face to make SOURCE_FACE_NONE work.


                // If this next face in the sequence is not the source and not engaged, try probing it
                if ( last_seed_face != download_source_face && !TBI( engagedbits , last_seed_face ) ) {

                    // Once a face has sent us a PULL then we know that he knows
                    // about the download. To avoid collisions, we let him take control of the link
                    // for now on and only answer his PULL requests, and don't send any more SEEDs
                    // that could collide with his aync PULLs

                    // Visually indicate the probe with a blue pixel

                    if (countdown_startup && countdown_active ) {
                        setRawPixelCoarse( last_seed_face , COARSE_MAGENTA );                          // Show blue on the face we are sending seed packet
                    } else {
                        if (countdown_startup) {
                            setRawPixelCoarse( last_seed_face , COARSE_BLUE );                          // Show blue on the face we are sending seed packet
                        } else {
                            setRawPixelCoarse( last_seed_face , COARSE_RED );                          // Show blue on the face we are sending seed packet
                        }
                    }

                    Debug::tx( 'E' );
                    Debug::tx( '0'+ last_seed_face );

                    send_seed_packet( last_seed_face , download_total_pages , active_program_checksum );

                }

                // Wait a while before sending next probe seed
                countdown_seed =  COUNTDOWN_SEED_COUNT;

            }  // if (countdown_seed==0 && download_next_page > 0  )

    } // while (countdown_giveup && !letsgo_flag) - our source is still alive, so we are wating for him to give the LETSGO

    // We got here either because we timed out or got a LETSGO packet

    // Check if the download was a success
    // TODO: Check total checksum here too

    if ( download_next_page == download_total_pages ) {

        // Download was a success

        Debug::tx('O');
        send_letsgo_storm();        // Tell everyone to tell every to start

        // Do a little success flash
        for(uint8_t x=0; x< 5; x++) {
            setAllRawCorsePixels( COARSE_GREEN );
            _delay_ms(100);
            setAllRawCorsePixels( COARSE_OFF );
            _delay_ms(100);
        }

    } else {

        // Download failed

        // Do a little failure flash
        for(uint8_t x=0; x< 50; x++) {
            setAllRawCorsePixels( COARSE_RED );
            _delay_ms(100);
            setAllRawCorsePixels( COARSE_OFF );
            _delay_ms(100);
        }

        copy_built_in_game_to_active();

        // If there is still someone Seeding us, then we will start downloading.
        // Otherwise we just run our built in game.
        // Hard to know what best to do in this situation.

    }

    // OK PEOPLE, LETS GO START THE GAME EVERYONE !!!!!
    
    move_interrupts_to_base();   
                                    // Send interrupt back to the normal vector table. For now, Arduino programs expect this.         
                                    // NOTE: boot_spm_interrupt_disable() does NOT do this! That disables the interrupt when an SPM instruction completes!
                                    // TODO: we will keep them when we take over pixel and other stuff.

    CLEAR_STACK_AND_JMP( "0x0000" );
    //asm("jmp 0x0000");
}



// This is not really an ISR, is it the entry point to start downloading from another tile
// We mark it as naked because everything is already set up ok, we just want to jump straight in.


inline void load_built_in_game_and_seed() {

    #warning Implement copy_built_in_game_to_active()

    download_and_seed_mode(1);          // Be the root for this new game

}

/*

// Foreground saw a seed packet so we should start downloading a new game

extern "C" void __attribute__((naked)) BOOTLOADER_DOWNLOAD_MODE_VECTOR() {

    //register uint8_t face asm("r25");           // We get passed the face in r25

    // TODO: Know where that first seed came from and start there.

    download_and_seed_mode(0);       // Entering this Way makes us listen for the first seed and then start downloading

}

// User longpressed so Wants to start seeding this blink's built in game

extern "C" void __attribute__((naked)) BOOTLOADER_SEED_MODE_VECTOR() {

    //register uint8_t face asm("r25");           // We get passed the face in r25

    load_built_in_game_and_seed();

}

*/

// This is the entry point where the blinkcore platform will pass control to
// us after initial power-up is complete
//void  dummymain2() __attribute__((section("zerobase"))) __attribute__((used));

//void  run() __attribute__((section("zerobase"))) __attribute__((used ));

//void __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) run();

void run(void) {

    Debug::init();

    //power_init();       // Get sleep functions

    // We only bother enabling what we need to save space

    pixel_init();

    ir_init();

    //button_init();

    ir_enable();

    pixel_enable();

    //button_enable_pu();
    
    move_interrupts_to_bootlader();

    //setAllRawCorsePixels( COARSE_ORANGE );  // Set initial display


    // copy_built_in_game_to_active();

    active_program_checksum = calculate_active_game_checksum();     // We need this (1) to know if an incoming seed packet matters to use, and
                                                                    // (2) to generate an outoging seed packet if we end up seeding

    sei();					// Let interrupts happen. For now, this is the timer overflow that updates to next pixel.



/*
    for(uint8_t blink=0;blink<5;blink++) {

        if (GPIOR1 == 'S') {

            setAllRawCorsePixels( COARSE_GREEN );
            _delay_ms(100);
            setAllRawCorsePixels( COARSE_RED );
            _delay_ms(100);

        } else {

            setAllRawCorsePixels( COARSE_BLUE );
            _delay_ms(100);
            setAllRawCorsePixels( COARSE_RED );
            _delay_ms(200);


        }
    }

*/

    if ( GPIOR1 == 'S' ) {

        // Foreground wants us to just seed

        load_built_in_game_and_seed();

    } else {

        // Foreground got a seed packet and wants us to download

        download_and_seed_mode(0);

    }


}




// Below is just a little stub program to make it easier to work on the bootloader.
// This stub lands at 0x000 to runs at reset. All it does is check the button.
// If the button is down, then it runs the bootloader in seed mode.
// If the button is up, then it runs to bootloader in download mode.
// Assumes there is a section called "zerobase" that starts at 0x000

extern "C" void __vector_3 (void) __attribute__((section("zerobase")));
extern "C" void __vector_3 (void) __attribute__((naked));

extern "C" void __vector_3 (void) {

    // Clear zero reg

    asm("eor r1,r1");

    // BUmp CPU up to 8Mhz

    CLKPR = _BV( CLKPCE );                  // Enable changes
    CLKPR =				0;                  // DIV 1 (8Mhz clock with 8Mhz RC osc)

    // Init all SRAM variables to 0

    for( uint16_t r=0x100; r<0x500; r++ ){

        *((uint8_t *) r) = 0;

    }


    Debug::init();

    // We only bother enabling what we need to save space

    button_enable_pu();                     // do this first so it has time to overcome capacitance

    pixel_init();

    pixel_enable();
    
    move_interrupts_to_bootlader();
    
    // copy_built_in_game_to_active();

    sei();					// Let interrupts happen. For now, this is the timer overflow that updates to next pixel.

    uint8_t f=0;

    while ( ( BUTTON_PIN & _BV(BUTTON_BIT) ) ) {
         setRawPixelCoarse( f , COARSE_RED );
         _delay_ms(100);
         setRawPixelCoarse( f , COARSE_OFF );
         f++;
         if (f==6) f=0;

    }

    setAllRawCorsePixels( COARSE_ORANGE );  // Set initial display

    _delay_ms(250);

    if ( ! ( BUTTON_PIN & _BV(BUTTON_BIT) ) ) {       // Pin goes low when button pressed because of pull up

        // Green flash shows we are the SEED
        setAllRawCorsePixels( COARSE_GREEN );
        _delay_ms(250);
        GPIOR1 = 'S';

    } else {

        GPIOR1 = 'D';

    }
    CLEAR_STACK_AND_JMP( "0x3800" );
    //asm("jmp 0x3800");

    // Canary bytes that you can see in the HEX file

    asm(" .byte 'M' ");
    asm(" .byte 'M' ");


}

