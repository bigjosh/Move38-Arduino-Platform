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
#include <stdint.h>     // uintX_t

#include <util/crc16.h>     // For IR checksums
#include <util/atomic.h>         // ATOMIC_BLOCK

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

#define US_PER_TICK 250

uint16_t tickcounter;

// Below are the callbacks we provide to blinkcore

// This is called by timer ISR about every 512us with interrupts on.

void timer_256us_callback_sei(void) {
    
    tickcounter++;

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

// These header bytes are chosen to try and give some error robustness.
// So, for example, a header with a repeating pattern would be less robust
// because it is possible something blinking in the environment might replicate it

#define IR_PACKET_HEADER_OOB      0b01001110      // Internal blinkOS messaging
#define IR_PACKET_HEADER_USERDATA 0b11010011      // Pass up to userland

void processPendingIRPackets() {

    // First process any IR data that came in

    for( uint8_t f=0; f< IRLED_COUNT ; f++ ) {

        if (irDataIsPacketReady(f)) {

            uint8_t packetLen = irDataPacketLen(f);

            // Note that IrDataPeriodicUpdateComs() will not save a 0-byte packet, so we know
            // the buffer has at least 1 byte in it

            // IR data packet received and at least 2 bytes long

            uint8_t const *data = irDataPacketBuffer(f);

            // TODO: What packet type should be fastest check (doesn;t really matter THAT much, only a few cycles)

            if (*data== IR_PACKET_HEADER_OOB ) {

                // blinkOS packet
                irDataMarkPacketRead(f);


            } else if (*data== IR_PACKET_HEADER_USERDATA ) {


                // Userland data
                //loopstate_in.ir_data_buffers[f].len=  packetLen-1;      // Account for the header byte
                //loopstate_in.ir_data_buffers[f].ready_flag = 1;

                // Note that these will get cleared after the loop call
                // We need this extra ready flag on top of the one in the irdata system because we share the
                // input buffer between system uses and userland, and so we only tell userland about packets
                // meant for them - and we hide the header byte from them too.
                // this is messy, but these buffers are the biggest thing in RAM.
                // so that the userland can read from the buffer without worrying about it getting
                // overwritten

            } else {

                // Thats's unexpected.
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

}

// Returns 0 if could not send because RX already in progress on this face (try again later)

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

void run(void) {

    // Set the buffer pointers

    ir_enable();

    pixel_enable();

    while (1) {

        // Let's get loopstate_in all set up for the call into the user process

        processPendingIRPackets();          // Sets the irdatabuffers in loopstate_in. Also processes any received IR blinkOS commands
                                            // I wish this didn't directly access the loopstate_in buffers, but the abstraction would
                                            // cost lots of unnecessary memory and coping


    }

}
