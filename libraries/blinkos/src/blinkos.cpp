/*

    This library provides the operating system for the Move38 Blinks platform.
    More info at http://Move38.com

    This sits on top of the hardware abstraction layer and handles functions like

       *Startup and loading new games
       *Sleeping
       *Time keeping

    ...basically all things that a game can not do.

*/

// TODO: TEST RX in progress by having one side TX rapidly wiht short breaks and see if other side fits into the break.
// TODO: Maybe make 1st bit of IR high be a "short 1 byte user message" where next 7 are checksum of the 2nd byte"?
// TODO: Must check for a reboot command at ISR level. After reboot send an "I just rebooted" message" and wait a sec for a download firmware request.
//       This blocks allows downloads even when user code blocks in loop(). Also always give downlaod fresh slate for machine state.
//       Maybe must do something like this also for sleeping?

#include <avr/pgmspace.h>
#include <stddef.h>     // NULL

#include <util/crc16.h>     // For IR checksums
#include <util/atomic.h>         // ATOMIC_BLOCK

#include "blinkos.h"
#include "blinkos_button.h"
#include "blinkos_timer.h"          // Class Timer
#include "blinkos_pixel.h"

#include "blinkos_headertypes.h"    // Defines the first hearer byte types for IR packets

#include "blinkos_blinkboot.h"

#include "callbacks.h"              // From blinkcore, which will call into us via these

// TODO: Put this at a known fixed address to save registers
// Will require a new .section in the linker file. Argh.

loopstate_in_t loopstate_in;
loopstate_out_t loopstate_out;

#include "pixel.h"
#include "timers.h"
#include "utils.h"
#include "power.h"
#include "button.h"
#include "adc.h"

#include "debug.h"

#include "ir.h"
#include "blinkos_irdata.h"


#define SLEEP_TIMEOUT_SECONDS (10*60)          // If no button press in this long then goto sleep

#define SLEEP_TIMEOUT_MS ( (millis_t) SLEEP_TIMEOUT_SECONDS  * MILLIS_PER_SECOND) // Must cast up because otherwise will overflow

// When we should fall sleep from inactivity
OS_Timer sleepTimer;

// Turn off everything and goto to sleep

static void sleep(void) {

    pixel_disable();        // Turn off pixels so battery drain
    ir_disable();           // TODO: Wake on pixel
    button_ISR_on();        // Enable the button interrupt so it can wake us

    power_sleep();          // Go into low power sleep. Only a button change interrupt can wake us

    button_ISR_off();       // Set everything back to thew way it was before we slept
    ir_enable();

    pixel_enable();

    loopstate_in.woke_flag= 1;

}


void postponeSleep() {
    sleepTimer.set( SLEEP_TIMEOUT_MS );
}


buttonstate_t buttonstate;

void timer_1000us_callback_sei(void) {

    incrementMillis1ms();

    if (updateButtonState1ms( buttonstate )) {
        postponeSleep();
    }

}

// Atomically copy button state from source to destination, then clear flags in source.

void grabAndClearButtonState(  buttonstate_t &d ) {

    ATOMIC_BLOCK( ATOMIC_FORCEON ) {

        d.bitflags = buttonstate.bitflags;
        d.clickcount = buttonstate.clickcount;
        d.down = buttonstate.down;
        buttonstate.bitflags =0;                      // Clear the flags we just grabbed (this is a one shot deal)

    }

}

// Below are the callbacks we provide to blinkcore

// This is called by timer ISR about every 512us with interrupts on.

void timer_256us_callback_sei(void) {

    // Decrementing slightly more efficient because we save a compare.
    static unsigned step_us=0;

    step_us += 256;                     // 256us between calls

    if ( step_us>= 1000 ) {             // 1000us in a 1ms
        timer_1000us_callback_sei();
        step_us-=1000;                  // Will never underflow because we checked that it is >1000
    }
}

// This is called by timer ISR about every 256us with interrupts on.

void timer_128us_callback_sei(void) {
    IrDataPeriodicUpdateComs();
}


#define IR_CRC_INIT    0xff         // Initial value for CRC calculation for IR packets

uint8_t crcupdate( uint8_t const *data , uint8_t len , uint8_t crc) {

    while (len) {

        crc = _crc8_ccitt_update( *data , crc );

        data++;
        len--;


    }

    return crc;

}

// Test that a buffer full of data has the right CRC at the end

uint8_t crccheck(uint8_t const * data, uint8_t len )
{
    uint8_t c = IR_CRC_INIT;                          // Start CRC at 0xFF

    while (--len) {           // Count 1 less than total length

        c=_crc8_ccitt_update( c , *data );

        data++;

    }

    // d now points to final byte, which should be CRC

    return *data == c;

}



void processPendingIRPackets() {

    // First process any IR data that came in

    for( uint8_t f=0; f< IR_FACE_COUNT; f++ ) {

        if (irDataIsPacketReady(f)) {

            uint8_t packetLen = irDataPacketLen(f);

            // Note that IrDataPeriodicUpdateComs() will not save a 0-byte packet, so we know
            // the buffer has at least 1 byte in it

            // IR data packet received and at least 2 bytes long

            uint8_t const *data = irDataPacketBuffer(f);

            // TODO: What packet type should be fastest check (doesn;t really matter THAT much, only a few cycles)

            if (*data==  IR_PACKET_HEADER_PULLFLASH) {

                Debug::tx( 'L' );

                // This is a request for us to send some of our active game. It should only come when we are in seed mode.

                uint8_t requested_page = data[1];           // Read the requested page from the request packet

                irDataMarkPacketRead(f);

                // We are going to blindly send here. The coast should be clear since they just sent the request and will be waiting a timeout period before sending the next one.
                blinkos_blinkboot_sendPushFlash( f , requested_page );


            } else if (*data== IR_PACKET_HEADER_USERDATA ) {


                // Userland data
                loopstate_in.ir_data_buffers[f].len=  packetLen-1;      // Account for the header byte
                loopstate_in.ir_data_buffers[f].ready_flag = 1;

                // Note that these will get cleared after the loop call
                // We need this extra ready flag on top of the one in the irdata system because we share the
                // input buffer between system uses and userland, and so we only tell userland about packets
                // meant for them - and we hide the header byte from them too.
                // this is messy, but these buffers are the biggest thing in RAM.
                // so that the userland can read from the buffer without worrying about it getting
                // overwritten

            } else {

                // That's unexpected.
                // Could be a data error that messed up bits in the header byte,
                // which is why we picked interesting bit patterns for header bytes

                // Consume the mangled packet it so a new packet can come in

                irDataMarkPacketRead(f);

            }   // sort out packet types

        }  // irIsdataPacketReady(f)
    }    // for( uint8_t f=0; f< IR_FACE_COUNT; f++ )

}

// This sends a user data packet. No error checking so you are responsible to do it yourself
// Returns 0 if it was not able to send because there was already an RX in progress on this face

uint8_t ir_send_userdata( uint8_t face, const uint8_t *data , uint8_t len ) {

    return ir_send_data( face , data , len ,  IR_PACKET_HEADER_USERDATA  );
/*
    // Ok, now we are ready to start sending!

    if (irSendBegin( face )) {

        irSendByte( IR_PACKET_HEADER_USERDATA );

        while (len) {

            irSendByte( *data++ );
            len--;

        }

        irSendComplete();

        return 1;
    }

    return 0;

    */

}

void ir_mark_packet_read( uint8_t face ) {

    irDataMarkPacketRead( face );
    loopstate_in.ir_data_buffers[face].ready_flag=0;

}

// Returns 0 if could not send becuase RX already in progress on this face (try again later)

uint8_t sendUserDataCRC( uint8_t face, const uint8_t *data , uint8_t len ) {

    // We figure out the CRC first so there are no undue delays while transmitting the data

    // TODO: Do we have time between bits to calculate the CRC as we go? A 1 bit gives us like 150us, so probably. Even with interrupts?

    uint8_t crc = IR_CRC_INIT;

    crc = _crc8_ccitt_update( IR_PACKET_HEADER_USERDATA , crc );

    crc = crcupdate( data , len , crc );

    // Ok, now we are ready to start sending!

    if (irSendBegin( face ) ) {

        irSendByte( IR_PACKET_HEADER_USERDATA );

        while (len--) {

            irSendByte( *data++ );

        }

        irSendByte( crc );

        irSendComplete();

        return 1;

    }

    return 0;

}


// This is the entry point where the blinkcore platform will pass control to
// us after initial power-up is complete

// We make this weak so that a game can override and take over before we initialize all the higher level stuff

extern void setupEntry();

enum mode_t { RUNNING , SEEDING };        // Are we running the game or seeding downloads to nearby tiles?

mode_t mode = RUNNING;

void run(void) {

    power_init();

    button_init();

    //adc_init();			    // Init ADC to start measuring battery voltage

    pixel_init();

    ir_init();

    ir_enable();

    irDataInit();       // Really only called to init IR_RX_DEBUG

    pixel_enable();

    button_enable_pu();

    blinkos_blinkboot_setup();     // Get everything ready for seeding
                                        // TODO: We don't need to call this until we actually go into game transmissions mode.

    // Set the buffer pointers

    for( uint8_t f=0; f< IR_FACE_COUNT; f++ ) {

        loopstate_in.ir_data_buffers[f].data = irDataPacketBuffer( f ) +1 ;     // We use 1st byte for routing, so we give userland everything after that
        loopstate_in.ir_data_buffers[f].ready_flag = 0;

    }



    // Call user setup code
    setupEntry();

    postponeSleep();            // We just turned on, so restart sleep timer

    #warning
    mode = SEEDING;          // Force for now

    //blinkos_transmit_self();

    while (1) {

        updateMillisSnapshot();             // Use the snapshot do we don't have to turn off interrupts
                                            // every time we want to check this multibyte value


        // Let's get loopstate_in all set up for the call into the user process

        processPendingIRPackets();          // Sets the irdatabuffers in loopstate_in. Also processes any received IR blinkOS commands
                                            // I wish this didn't directly access the loopstate_in buffers, but the abstraction would
                                            // cost lots of unnecessary memory and coping

        grabAndClearButtonState( loopstate_in.buttonstate );     // Make a local copy of the instant button state to pass to userland. Also clears the flags for next time.

        loopstate_in.millis = millis_snapshot;

        if (mode==SEEDING) {

            blinkos_blinkboot_loop();

        } else if (mode==RUNNING) {

            loopEntry();

            for( uint8_t f = 0; f < PIXEL_FACE_COUNT ; f++ ) {

                pixelColor_t color = loopstate_out.colors[f];

                if (  color.reserved ) {          // Did the color change on the last pass? (We used the reserved bit to tell in the blnkOS API)

                    blinkos_pixel_bufferedSetPixel( f,  color  );           // TODO: Do we need to clear that top bit?

                }

            }

            blinkos_pixel_displayBufferedPixels();      // show all display updates that happened in last loop()
                                                            // Also currently blocks until new frame actually starts
        }

        for( uint8_t f = 0; f < IR_FACE_COUNT ; f++ ) {

            // Go though and mark any of the buffers we just passed into to the app as clear
            // Note that we don't have to worry about any races here because we set the ready flag
            // ourselves in this very loop

            // We have to do this or else an errant user program could leave a buffer full forever
            // and then we would never see any incoming control messages.

            if ( loopstate_in.ir_data_buffers[f].ready_flag ) {

                irDataMarkPacketRead( f ) ;

                // Also clear out the ready flag shown to userland so they don't keep seeing the same packet over and over again.

                loopstate_in.ir_data_buffers[f].ready_flag =0;

            }
        }

        if (sleepTimer.isExpired()) {
            sleep();
        }


        // TODO: Possible sleep until next timer tick?
    }

}
