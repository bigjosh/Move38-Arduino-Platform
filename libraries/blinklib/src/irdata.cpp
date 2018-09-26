/*

    Talk to the 6 IR LEDs that are using for communication with adjacent tiles


    THEORY OF OPERATION
    ===================

    The IR LEDs detect pulses of light.

    A bit is represented by the space between two pulses of light. A short space is a '1' and a long space is a '0'.

    A transmission is sent as a series of 8 bits.

    We always send a preamble of "10" before the data bits. This allows the receiver to detect the
    incoming byte. This sequence is special because (1) the only way a '1' can end up in the top bit
    of the buffer is if we have received enough consecutive valid bits, and (2) the following '0'
    guards against the case where we send an opening '1' bit but it just happens to come right after the
    receiver had spontaneously fired. In this case, we might see these two fires (the spontaneous one
    and the intentional start of the preamble one) as a valid '1' bit. The extra '0' ensure that only the second
    intentional '1' bit will have a '0' after it and the phantom '1' will be pushed off the top of the buffer.

    There are 6 data bits after the preamble. MSB sent first.

    There must be an idle gap after the end of each transmission.

    Internally, we use an 8-bit buffer to store incoming bits. We check to see if the top two
    bit of the buffer match the '10' preamble sequence to know if we have received a good transmission.

    If we ever go too long between pulses, we reset the incoming buffer to '0' to avoid
    detecting a phantom message.

    TODO: When noise causes a face to wake us from sleep too much, we can turn off the mask bit for a while.

    TODO: MORE TO COME HERE - NEED PICTURES.


*/

#include "blinklib.h"

#include "ir.h"
#include "utils.h"
#include "timer.h"          // get US_TO_CYCLES()

#include "irdata.h"

#include <util/delay.h>         // Must come after F_CPU definition

#include <avr/sfr_defs.h>		// Gets us _BV()

// A bit cycle is one 2x timer tick, currently 256us

#define IR_WINDOW_US 256            // How long is each timing window? Based on timer programming and clock speed.

#define IR_CLOCK_SPREAD_PCT  10     // Max clock spread between TX and RX clocks in percent

#define TX_PULSE_OVERHEAD_US 25     // How long does ir_tx_pulse() take?

// Time between consecutive bit flashes
// Must be long enough that receiver can detect 2 distinct consecutive flashes even at maximum interrupt latency

 //#define IR_SPACE_TIME_US (IR_WINDOW_US + (( ((unsigned long) IR_WINDOW_US * IR_CLOCK_SPREAD_PCT) ) / 100UL ) - TX_PULSE_OVERHEAD )  // Used for sending flashes. Must be longer than one IR timer tick including if this clock is slow and RX is fast.

#define IR_SPACE_TIME_US (300)  // Used for sending flashes.
                                // Must be longer than one IR timer tick including if this clock is slow and RX is fast
                                // Must be shorter than two IR timer ticks including if the sending pulse is delayed by maximum interrupt latency.

#define TICKS_PER_SECOND (F_CPU)

#define IR_SPACE_TIME_TICKS US_TO_CYCLES( IR_SPACE_TIME_US )

/*

// from http://www.microchip.com/forums/m587239.aspx

static uint8_t oddParity(uint8_t p) {
      p = p ^ (p >> 4 | p << 4);
      p = p ^ (p >> 2);
      p = p ^ (p >> 1);
      return p & 1;
}

*/


// You really want sizeof( ir_rx_state_t) to be  a power of 2. It makes the emitted pointer calculations much smaller and faster.
// TODO: Do we need all these volatiles? Probably not...

 typedef struct {

     // These internal variables are only updated in ISR, so don't need to be volatile.

     // I think access to the  first element is slightly faster, so most frequently used should go here

    uint8_t volatile windowsSinceLastFlash;          // How many times windows since last trigger? Reset to 0 when we see a trigger

    uint8_t inputBuffer;                    // Buffer for RX in progress. Data bits step up until high bit set.
                                            // High bit will always be set for real data because the start bit is 1


    // Visible to outside world
     volatile uint8_t inValue;            // Last successfully decoded RX value. 1 in high bit means valid and unread. 0= empty.


    uint8_t dummy;                          // TODO: parity bit? for now just keep struct a power of 2

    // This struct should be even power of 2 long.

 } ir_rx_state_t;


 // We keep these in together in a a struct to get faster access via
 // Data Indirect with Displacement opcodes

 static volatile ir_rx_state_t ir_rx_states[IRLED_COUNT];

// Called once per timer tick
// Check all LEDs, decode any changes

 // Note this runs in callback context in timercallback.

 volatile uint8_t specialWindowCounter=0;

 // Temp storage to stash the IR bits while interrupts are off.
 // We will later process and decode once interrupts are back on.

 volatile uint8_t most_recent_ir_test;

 void timer_256us_callback_cli(void) {

    // Interrupts are off, so get it done as quickly as possible
    most_recent_ir_test = ir_test_and_charge_cli();

 }

 void updateIRComs(void) {

     // Grab which IR LEDs triggered in the last time window

    uint8_t bits = most_recent_ir_test;

    // Loop though and process each IR LED (which corresponds to one bit in `bits`)
    // Start at IR5 and work out way down to IR0.
    // Going down is faster than up because we can test bitwalker == 0 for free


    uint8_t bitwalker = _BV( IRLED_COUNT -1 );
    ir_rx_state_t volatile *ptr = ir_rx_states + IRLED_COUNT -1;

/*
    #warning only checking IR0!
    uint8_t bitwalker = _BV( 0 );
    ir_rx_state_t volatile *ptr = ir_rx_states;
*/
    // Loop though each of the IR LED and see if anything happened on each...

    do {

        uint8_t bit = bits & bitwalker;

        if (bit) {      // This LED triggered in the last time window

            uint8_t thisWindowsSinceLastFlash = ptr->windowsSinceLastFlash;

             ptr->windowsSinceLastFlash = 0;     // We just got a flash, so start counting over.

            if (thisWindowsSinceLastFlash<=3) {     // We got a valid bit

                uint8_t inputBuffer = ptr->inputBuffer;     // Compiler should do this optimization for us, but it don't

                inputBuffer <<= 1;      // Make room for new bit (fills with a 0)

                if (thisWindowsSinceLastFlash<=1) {     // Saw a 1 bit

                    inputBuffer |= 0b00000001;          // Save newly received 1 bit

                }

                // Here we look for a 1 followed by a 0 in the top two bits. We need this
                // because there can potentially be a leading 1 in the bit stream if the
                // first pulse of the 0 start bit happens to come right after an ambient
                // trigger - this could look like a 1. This can only happen at the first pulse
                // because after that we are pulsing often enough that there will never be
                // an ambient trigger.
                // So we use the pattern '10' as a start because if there is a leading '1'
                // then there will be '11' at the beginning and the 1st '1' will not have a '0'
                // after it.
                // TODO: Explain this better with pictures.


                if ( (inputBuffer & 0b11000000) == 0b10000000 ) {

                    // TODO: check for overrun in lastValue and either flag error or increase buffer size

                    ptr->inValue = inputBuffer;           // Save the received byte (clobbers old if not read yet)

                    inputBuffer =0;                    // Clear out the input buffer to look for next start bit


                }

                ptr->inputBuffer = inputBuffer;

            }  else {

                // Received an invalid bit (too long between last two detected flashes)

                ptr->inputBuffer = 0;                       // Start looking for start bit again.

            }

        } else {

            ptr->windowsSinceLastFlash++;           // Keep count of how many windows since last flash

            // Note there here were do not check for overflow on windowsSinceLastFlash.
            // We assume that an LED will spontaneously fire form ambient light in fewer
            // than 255 time windows

        }


        ptr--;
        bitwalker >>=1;


    } while (bitwalker);

}

// Is there a received data ready to be read on this face?

bool irIsReadyOnFace( uint8_t led ) {
    return( ir_rx_states[led].inValue != 0 );
}

// Read the most recently received data. Blocks if no data ready

uint8_t irGetData( uint8_t led ) {

    ir_rx_state_t volatile *ptr = ir_rx_states + led;        // This turns out to generate much more efficient code than array access. ptr saves 25 bytes. :/   Even so, the emitted ptr dereference code is awful.

    while (! ptr->inValue );      // Wait for high be to be set to indicate value waiting.

    // No need to atomic here since these accesses are lockstep, so the data can not be updated until we clear the ready bit

    uint8_t d = ptr->inValue;

    ptr->inValue=0;       // Clear to indicate we read the value. Doesn't need to be atomic.

    return d & 0b00111111;      // Don't show our internal preamble bits

}

// Simultaneously send data on all faces that have a `1` in bitmask

void irSendDataBitmask(uint8_t data, uint8_t bitmask) {

    uint8_t bitwalker = 0b00100000;

    // Start things up, send initial pulse and start bit (1)
    ir_tx_start( IR_SPACE_TIME_TICKS , bitmask , 1 );

    ir_tx_sendpulse( 3 );           // Guard 0 bit to ensure real start bit is detected and not extraneous leading pulse.

    do {

        if (data & bitwalker) {
            ir_tx_sendpulse( 1 ) ;
        } else {
            ir_tx_sendpulse( 3 ) ;
        }

        bitwalker>>=1;

    } while (bitwalker);

    // TODO: Send a stop bit or some parity bit for error checking? Necessary?

    ir_tx_end();

}

// Send data on specified face
// I put destination (face) first to mirror the stdio.h functions like fprintf().

void irSendData(  uint8_t face , uint8_t data  ) {
    irSendDataBitmask( data , 1 << face );
}


// Send data on all faces

void irBroadcastData( uint8_t data ) {

    irSendDataBitmask( data , IR_ALL_BITS );

}
