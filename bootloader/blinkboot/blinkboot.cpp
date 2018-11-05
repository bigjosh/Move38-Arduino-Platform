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

#warning
#include "debug.h"

#include "callbacks.h"      // blinkcore calling into us - the timer callbacks & run()

#include "ir.h"
#include "blinkboot_irdata.h"

#include "coarsepixelcolor.h"

#include "blinkos_headertypes.h"

#define US_PER_TICK        256UL
#define US_PER_MS         1000UL
#define TICK_PER_MS       ( US_PER_MS / US_PER_TICK)
#define MS_TO_TICKS( milliseconds ) ( milliseconds / MS_PER_TICK )

#define TICKS_PER_COUNT    256UL                                // Prescaller
#define US_PER_COUNT     (US_PER_TICK * TICKS_PER_COUNT)        // ~65ms per count
#define MS_PER_COUNT     (US_PER_COUNT/US_PER_MS)

#define MS_TO_COUNTS( ms )  ((ms / MS_PER_COUNT )+1)      // Always round up to longer delay

// Here lives all the bootloader state


volatile uint8_t countdown_until_done;       // If we have not seen a PUSH or PULL or an ACTIVE in a while then we timeout.
                                     // If we are fully downloaded, then we send a few DONEs and then reboot into the active game
                                     // If now, then we copy built-in game and jump to to

                                     // Should be >~12x the time it takes to send the longest packet to make sure that just missing a couple
                                     // will not cause a false alarm.

#warning make this shorter in production
#define COUNTDOWN_UNTIL_NEXT_DONE_MS     16000UL   // Wait long enough that if the neighbor had to service 5 PULL packets to get back to use we would still have time.
#define COUNTDOWN_UNTIL_NEXT_DONE_COUNT  MS_TO_COUNTS( COUNTDOWN_UNTIL_NEXT_DONE_MS )

#if (COUNTDOWN_UNTIL_DONE_COUNT > 255 )     // So ugly. There must be a way to get the max value of a type at compile time?
    #error must fit into uint8_t
#endif


volatile uint8_t countdown_until_next_seed;  // When do we try and send a seed again?
                                    // We have to wait a bit between seeds to give the other side a sec to answer if they want to pull

#warning make this shorter
#define COUNTDOWN_UNTIL_NEXT_SEED_MS    100UL     // Long enough that they can see out seed and get back to use with a PULL
#define COUNTDOWN_UNTIL_NEXT_SEED_COUNT MS_TO_COUNTS( COUNTDOWN_UNTIL_NEXT_SEED_MS )

#if (COUNTDOWN_UNTIL_NEXT_SEED_COUNT > 255 )     // So ugly. There must be a way to get the max value of a type at compile time?
    #error must fit into uint8_t
#endif


// Make next seed go out immediately

static void trigger_countdown_until_next_seed() {
    countdown_until_next_seed=0;
}

static void reset_countdown_until_next_seed() {

#warning
    countdown_until_next_seed= COUNTDOWN_UNTIL_NEXT_SEED_COUNT;
}


static void reset_countdown_until_done() {
    countdown_until_done=COUNTDOWN_UNTIL_NEXT_DONE_COUNT;
}



// Below are the callbacks we provide to blinkcore

// This is called by timer ISR about every 512us with interrupts on.

// This function is not reentrant unless we are doing something very wrong,
// so ok to test and then update these values without worrying about corruption.

void timer_256us_callback_sei(void) {

    static uint8_t tick_prescaller;     // Divide ticks /256
                                        // We need this extra divider so that our timeout counters fit into a single byte
                                        // otherwise we need to worry about interrupts coming in the middle of updates.

    if ( --tick_prescaller == 0 ) {      // Predecement so the flag will already be set for comparison

        // Do this stuff once every 256 ticks

        if (countdown_until_next_seed) {
            countdown_until_next_seed--;
        }

        if (countdown_until_done) {
            countdown_until_done--;
        }
    }

}

// This is called by timer ISR about every 256us with interrupts on.

void timer_128us_callback_sei(void) {
    IrDataPeriodicUpdateComs();
}


#define DOWNLOAD_MAX_PAGES 56       // The maximum length of a downloaded game
                                    // Currently set to use ~7KB. This saves 7KB for built in game and 2KB for bootloader


#define FLASH_PAGE_SIZE         SPM_PAGESIZE                     // Fixed by hardware

#define FLASH_ACTIVE_BASE       ((uint8_t *)0x0000)     // Flash starting address of the active area
#define FLASH_BUILTIN_BASE      ((uint8_t *)0x1c00)     // flash starting address of the built-in game
#define FLASH_BOOTLOADER_BASE   ((uint8_t *)0x3800)     // Flash starting address of the bootloader. Fixed by hardware.

// The FLASH_BOOTLOADER_BASE must be defined as the start of the .text segment when compiling the bootloader code.
// Note that the linker doubles the value passed, so the .text should start at 0x1c00 from the linker's args


void __attribute__ ((noinline)) burn_page_to_flash( uint8_t page , const uint8_t *data ) {

    // TODO: We can probably do these better directly. Or maybe fill the buffer while we load the packet. Then we have to clear the buffer at the beginning.

    // First erase the flash page...

    // Fist set up zh to have the page in it....

    uint16_t address = (page * 128);        // This should hit the page at 0x0d00 which we currently define as testburn...

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


    // We write in words not bytes

    uint16_t *ptr = (uint16_t *) data;

    for(uint16_t i=0; i<128;i+=2 ){               // 64 words (128 bytes) per page
        cli();
        boot_page_fill(  i , *ptr++ );
        sei();
    }

    cli();

    // We have to leave ints off for the full erase/write cycle here
    // until we get all the int handlers up into the bootloader section.

    boot_page_erase( address );

    boot_spm_busy_wait();

    // NOte that the bottom bits must be 0 for the write command. :/

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

uint16_t checksum_flash_page( uint8_t page ) {

    uint16_t checksum=0;

    uint8_t *p = FLASH_ACTIVE_BASE + ( page *FLASH_PAGE_SIZE );

    for( uint8_t i=0; i < FLASH_PAGE_SIZE; i++ ) {

        checksum += pgm_read_byte( p++ );

    }

    checksum += page;

    return checksum;

}

uint8_t checksum_128byte_RAM_buffer( const uint8_t *buffer ) {
    uint8_t data_computed_checksum=0;         // We have to keep this separately so we can fold it into the total program checksum

    for(uint8_t i=0; i<FLASH_PAGE_SIZE;i++ ) {

        data_computed_checksum += *(buffer++);

    }

    return data_computed_checksum;
}



uint8_t push_packet_payload_checksum( const push_payload_t &push_payload ) {

    uint8_t checksum = checksum_128byte_RAM_buffer( push_payload.data );  // Start with the checksum of the data part

    checksum+=IR_PACKET_HEADER_PUSH;       // Add in the header

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

void send_push_packet( uint8_t face , uint8_t page ) {

    if (irSendBegin( face )) {

        if (page & 1 ) {

            displayedRawPixelSet.rawpixels[face] = rawpixel_t( 255 , 0 , 255 );
        } else {

            displayedRawPixelSet.rawpixels[face] = rawpixel_t( 255 ,  160 ,  255 );

        }

        uint8_t computed_checksum= IR_PACKET_HEADER_PUSH;

        irSendByte( IR_PACKET_HEADER_PUSH );

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

        Debug::tx('l');


    }    else {

        displayedRawPixelSet.rawpixels[face] = rawpixel_t( 0 , 255 , 255 );

    }

}



// TODO: Precompute this whole packet at compile time and send as one big chunk

void send_seed_packet( uint8_t face , uint8_t pages , uint16_t program_checksum ) {

   if (irSendBegin( face )) {

       irSendByte( IR_PACKET_HEADER_SEED );

       uint8_t computed_checksum=IR_PACKET_HEADER_SEED;

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



uint16_t calculate_active_game_checksum() {

    uint16_t checksum=0;

    for( uint8_t page=0; page< DOWNLOAD_MAX_PAGES ; page++ ) {

        checksum += checksum_flash_page( page );
    }

    return checksum;

}


#define SOURCE_FACE_NONE    (FACE_COUNT)            // No source face, we are the root.

uint8_t download_source_face;         // The face that we saw the first seed on and we are now downloading from
                                      // Only valid if download_total_pages > 0

uint8_t download_total_pages;         // The length of the game we are downloading in pages. Init to 0. Set by 1st pull request.
uint8_t download_next_page;           // Next page we want to get
                                      // 0 means we have not gotten a good push yet
                                      // > total_pages means that download is done (hack)


uint16_t active_program_checksum;     // The total program checksum for the loaded or loading active program
                                      // Valid after built in game copied, or after a seed received


uint8_t next_seed_face;               // The next face we will send a seed packet to. Note that we skip around so that adjacent tiles can maybe seed form each other.


// This is triggered by use getting a seed packet. If we get a seed then we know that the other side is waiting for us to pull
// so should not be a collision.

inline void sendNextPullPacket() {

    if (irSendBegin( download_source_face )) {

        irSendByte( IR_PACKET_HEADER_PULL );

        irSendByte( download_next_page );

        irSendComplete();

        } else {

        // Send failed.

    }

}


// Returns 1 if this packet passes length and checksum tests for valid push packet

uint8_t inline push_packet_valid( const blinkboot_packet *push_packet , uint8_t packet_len ) {

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



// List of faces in order to seed. We stager them so that hopefully we can make gaps so that adjacent
// blinks can download from each other rather than from us.

// So, for example, if we are on face 0 then next we will jump to face 2. After 2 comes 4. See? Cute, right?

static PROGMEM const uint8_t next_stagered_face[FACE_COUNT] = { 2 , 3 , 4 , 5 , 1 , 0 } ;

// Read in any pending packets and react to to them
// Pull packet triggers a push if we have the page
// Seed packet triggers pull if we need more pages
// Push packet writes new page to flash

// Use this to move a function out of the bootloader to make room.
// We are able to move this out of the bootloader because it never runs in an interrupt context.
// It is only called from he main event loop.

void __attribute__((section("subbls"))) __attribute__((used)) __attribute__ ((noinline)) processInboundIRPackets();

void processInboundIRPackets() {

    for( uint8_t f=0; f< IRLED_COUNT ; f++ ) {

        if (irDataIsPacketReady(f)) {

            uint8_t packetLen = irDataPacketLen(f);

            // Note that IrDataPeriodicUpdateComs() will not save a 0-byte packet, so we know
            // the buffer has at least 1 byte in it

            // IR data packet received and at least 2 bytes long

            // TODO: Dig straight into the ir data structure and save these calls?

            const  blinkboot_packet *data = (const blinkboot_packet *) irDataPacketBuffer(f);

            // TODO: What packet type should be fastest check (doesn;t really matter THAT much, only a few cycles)

            if (data->header == IR_PACKET_HEADER_PUSH ) {

                // This is a packet of flash data

                if ( push_packet_valid( data , packetLen ) )  {        // Push packet payload plus header byte. Just an extra check that this is a valid packet.

                    // Note we do not check for program checksum while downloading. If we somehow start downloading a different game, it should result
                    // in a bad total checksum when we get to the last block so We can do something other than jumping into the mangled flash image.

                    // This is a valid push packet

                    //TODO: Check that it came from the right face? Check that we are in download mode?

                    uint8_t packet_page_number = data->push_payload.page;

                    if ( packet_page_number == download_next_page) {        // Is this the one we are waiting for?

                        Debug::tx( 'G' );
                        Debug::tx( packet_page_number );

                        // We got a good packet, good checksum on data, and it was the one we needed, and it is the right game!!!

                        burn_page_to_flash( packet_page_number , data->push_payload.data );

                        download_next_page++;

                        // Blink GREEN with each packet that gets us closer (out of order packets will not change the color)
                        setRawPixelCoarse( f , download_next_page & 1 ? COARSE_GREEN : COARSE_DIMGREEN );

                        if (download_next_page == download_total_pages ) {

                            download_next_page++;           // This hack makes download_next_page > download_total_pages when the download is complete

                            // We are done downloading! Anything else we should do here?

                            setAllRawCorsePixels( COARSE_BLUE );

                        } else {

                            // we are still downloading, so dont time out as long as we are getting valid push packets that get us closer
                            // to being done

                            reset_countdown_until_done();
                            
                            // Grease the wheels to get get next push quickly and continue the download
                            sendNextPullPacket();
                            // The other side should be waiting for this and ready to send the next push right away
                            // but if anything goes wrong, the source will step to the next face and try to download to them 
                            // and eventually come back to us with a seed to get things started again. 
                            
                            

                            // We should have another seed waiting for us to trigger next pull so no need to grease that wheel here.

                        }


                    } else { //  if ( ! packet_page_number == download_next_page)

                            setRawPixelCoarse( f , COARSE_ORANGE );

                    }


                    // We will pull next page when the source gets back around to sending us another seed packet to show it
                    // is our turn.

                } else {     // if ( push_packet_valid( data ) ) )

                    // invalid push packet
                    setRawPixelCoarse( f , COARSE_RED );

                }

            } else if (data->header == IR_PACKET_HEADER_SEED ) {             // This is other side telling us to start pulling new game from him

                if ( download_next_page <= download_total_pages ) {                // Do we need any more pages? (at start both will be 0, at end next will be total+1

                    if ( packetLen== (sizeof( seed_payload_t ) + 1 ) ) {         // Seed request plus header byte

                        if ( download_total_pages == 0 ) {      /// we are waiting for a good seed?

                            // We found a good source - lock in and start downloading

                            #warning check seeed packet checksum!

                            download_total_pages = data->seed_payload.pages;

                            active_program_checksum = data->seed_payload.program_checksum;

                            download_source_face = f;

                            next_seed_face = download_source_face + 2;  // Start seeding up and to the left.
                                                                        // This always skips the next next counter clockwise face to give some room for distributed downloading.
                                                                        // TODO: Is this the best simple strategy?

                            if (next_seed_face>=6) next_seed_face-=6;   // In case we wrapped around past face 5

                            // We will not start seeding until we actually have something to send

                            //download_next_page=0;                 // It inits to zero so we don't need this explicit assignment

                            setAllRawCorsePixels( COARSE_OFF );

                            setRawPixelCoarse( f , COARSE_BLUE );

                        }   //  if ( download_total_pages == 0 ) - we need a source

                        // We got a seed packet, so source is ready for our pull

                        sendNextPullPacket();

                    }  else { // Packet length wrong for pull request

                        setRawPixelCoarse( f , COARSE_RED );

                    } //packetLen== (sizeof( pull_request_payload_t ) + 1 )

                } //if ( download_next_page <= download_total_pages )  (ignore seed packets if we are already downloaded)

            }  else if (data->header == IR_PACKET_HEADER_PULL ) {

                if ( packetLen== (sizeof( pull_payload_t) + 1 ) ) { // // Pull request plus header byte

                    uint8_t requested_page = data->pull_payload.page;

                    if (requested_page<download_next_page) {            // Do we even have the next page?

                        send_push_packet( f , requested_page  );
                        
                        reset_countdown_until_next_seed();              // Give the recipient time to do a pull when they are ready for it

                        reset_countdown_until_done();                   // As long as we are getting pulls, someone still needs us

                    } // if (requested_page<download_next_page)

                } else { // if ( packetLen== (sizeof( pull_request_payload_t) + 1 ) )

                    setRawPixelCoarse( f , COARSE_RED );

                }

            }// if (data->header == IR_PACKET_HEADER_PULLREQUEST )

            irDataMarkPacketRead(f);

        }// irIsdataPacketReady(f)

    }    // for( uint8_t f=0; f< IR_FACE_COUNT; f++ )

}

// Move the interrupts up to the bootloader area
// Normally the vector is at 0, this moves it up to 0x3400 in the bootloader area.

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

 // If we do not already have an active game (as indicated by download_next_page and download_total_pages), then
 // wait for a seed packet and start downloading. If we do already have an active game, just start seeding.

 // TODO: Jump into all these to save all that pushing and poping. We will never be returing, so all for naught.

inline void download_and_seed_mode() {

    setAllRawCorsePixels( COARSE_DIMBLUE );

    while (1) {

        processInboundIRPackets();       // Read any incoming packets looking for pulls & pull requests

        if (countdown_until_next_seed==0 && download_next_page > 0  ) {      // Don't bother sending a seed until we have something to share

            setRawPixelCoarse( next_seed_face , COARSE_OFF );

            do {

                next_seed_face = pgm_read_byte( next_stagered_face + next_seed_face );      // Get next staggered face to send on

            } while ( next_seed_face == download_source_face );         // No need to seed the source face, he already has everything we have
                                                                        // Thie while should only trigger at most one time

            setRawPixelCoarse( next_seed_face , COARSE_BLUE );                          // SHow blue on the face we are sending seed packet

            send_seed_packet( next_seed_face , download_total_pages , active_program_checksum );

            reset_countdown_until_next_seed();

        }

        #warning
        if (0 && countdown_until_done==0) {

            // We timed out waiting for something to happen, so reboot.
            // For now we should lock up, but eventually we can use the chain of ACTIVE packets to trigger the root to send
            // a START NOW packet and everyone can reboot at once and no chance of accidentally getting an seed loop

            setAllRawCorsePixels( COARSE_GREEN );

            // All read means download failed.
            // red/blue alternating means it worked

            if ( download_next_page > download_total_pages ) {
                setRawPixelCoarse( 0 , COARSE_BLUE );               // If download worked, show alternating blue/green
                setRawPixelCoarse( 2 , COARSE_BLUE );
                setRawPixelCoarse( 4 , COARSE_BLUE );
            }
        }

    }

}

void copy_built_in_game_to_active() {

    #warning Implement copy_built_in_game_to_active()

    // For now we will use whatever game happens to end up in the active area

}

// This is not really an ISR, is it the entry point to start downloading from another tile
// We mark it as naked because everything is already set up ok, we just want to jump straight in.


inline void load_built_in_game_and_seed() {

    // TODO: implement
    // Copy the built in game to the active area in flash

    // TODO: Calc and return checksum

    active_program_checksum = calculate_active_game_checksum();

    download_total_pages = DOWNLOAD_MAX_PAGES;       // For now always send the whole thing. TODO: Base this on actual program length if shorter?
    download_next_page = download_total_pages+1;    // Special value of next > total signals totally downloaded, so just seed mode
    download_source_face = SOURCE_FACE_NONE;        // We are the root


    download_and_seed_mode();       // because we already set up the download variables to be done downloading, we will seed until timeout

}


// Foreground saw a seed packet so we should start downloading a new game

extern "C" void __attribute__((naked)) BOOTLOADER_DOWNLOAD_MODE_VECTOR() {

    //register uint8_t face asm("r25");           // We get passed the face in r25

    // TODO: Know where that first seed came from and start there.

    download_and_seed_mode();       // Entering this Way makes us listen for the first seed and then start downloading

}

// User longpressed so Wants to start seeding this blink's built in game

extern "C" void __attribute__((naked)) BOOTLOADER_SEED_MODE_VECTOR() {

    //register uint8_t face asm("r25");           // We get passed the face in r25

    load_built_in_game_and_seed();

}


// This is the entry point where the blinkcore platform will pass control to
// us after initial power-up is complete
//void  dummymain2() __attribute__((section("zerobase"))) __attribute__((used));

//void  run() __attribute__((section("zerobase"))) __attribute__((used ));
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

    move_interrupts_to_bootlader();      // Send interrupt up to the bootloader.

    setAllRawCorsePixels( COARSE_ORANGE );  // Set initial display

    // copy_built_in_game_to_active();

    sei();					// Let interrupts happen. For now, this is the timer overflow that updates to next pixel.

    for(uint8_t blink=0;blink<5;blink++) {

        if (GPIOR1 == 'S') {

            setAllRawCorsePixels( COARSE_GREEN );
            _delay_ms(200);
            setAllRawCorsePixels( COARSE_RED );
            _delay_ms(200);

        } else {

            setAllRawCorsePixels( COARSE_BLUE );
            _delay_ms(200);
            setAllRawCorsePixels( COARSE_RED );
            _delay_ms(200);


        }
    }

    if ( GPIOR1 == 'S' ) {

        // Foreground wants us to just seed

        load_built_in_game_and_seed();

    } else {

        // Foreground got a seed packet and wants us to download

        download_and_seed_mode();

    }


}




extern "C" void __vector_3 (void) __attribute__((section("zerobase")));
extern "C" void __vector_3 (void) __attribute__((naked));

extern "C" void __vector_3 (void) {

    asm("eor r1,r1");

    BUTTON_PORT |= _BV(BUTTON_BIT);

    _delay_ms( 100 );           // Give pull up a sec to act

    if ( ! ( BUTTON_PIN & _BV(BUTTON_BIT) ) ) {       // Pin goes low when button pressed because of pull up

        GPIOR1 = 'S';

    } else {

        GPIOR1 = 'D';

    }

    asm("jmp 0x3800");

    GPIOR1='J';
    GPIOR1='O';
    GPIOR1='S';
    GPIOR1='H';

    asm(" .byte 'M' ");
    asm(" .byte 'M' ");


}
