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

#include "debug.h"

#include "blinkos.h"

#include "ir.h"
#include "utils.h"
#include "timers.h"          // get US_TO_CYCLES()

#if defined(IR_TX_DEBUG) || defined(IR_RX_DEBUG)
    #include "debug.h"                                     // We use the SP port for deugging Stuff
#endif

#include "blinkos_irdata.h"

#include <util/delay.h>         // Must come after F_CPU definition

#include <avr/sfr_defs.h>		// Gets us _BV()

#include <util/atomic.h>        // ATOMIC_BLOCK

// A bit cycle is one 2x timer tick, currently 128us

#define IR_WINDOW_US 128            // How long is each timing window? Based on timer programming and clock speed.

#define IR_CLOCK_SPREAD_PCT  5      // Max clock spread between TX and RX clocks in percent

#define IR_CHARGE_OVERHEAD 50       // How long does take to fully charge IR after it has triggered?

// Time between consecutive bit flashes
// Must be long enough that receiver can detect 2 distinct consecutive flashes even at maximum interrupt latency


#define SQUARE(n) ((n)*(n))

//#define IR_SPACE_TIME_US (IR_WINDOW_US + (( ((unsigned long) IR_WINDOW_US * IR_CLOCK_SPREAD_PCT) ) / 100UL ) - TX_PULSE_OVERHEAD )  // Used for sending flashes. Must be longer than one IR timer tick including if this clock is slow and RX is fast.

//#define IR_SPACE_TIME_US (IR_WINDOW_US *  SQUARE(1 + (IR_CLOCK_SPREAD_PCT/100.0) ) )  // Used for sending flashes.

//#define IR_SPACE_TIME_US (145) )   // Used for sending flashes.

// We calculate these transmit window sizes individually in the sending code.

                                // Calculated to be just long enough that TX window will be longer than RX sample window when clocks mismatch by 5%
                                // Must be longer than one IR timer tick including if this clock is slow and RX is fast
                                // Must be shorter than two IR timer ticks including if the sending pulse is delayed by maximum interrupt latency.

#define TICKS_PER_SECOND (F_CPU)

#define IR_SPACE_TIME_TICKS US_TO_CYCLES( IR_SPACE_TIME_US )

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

 } ;


 /*

    STATE TRANSITIONS WHILE READING AN IR PACKET
    ============================================

    Start with byteBuffer=0. The only way out of this state is to get a SYNC.

    When a SYNC is seen, then we load 0b10000000 into the byteBuffer and 0 into the packetBufferLen and then start looking for valid bits to stuff into the buffer.

    After the 8th valid bit, the 1 that was at the top will fall off the bottom, triggering us to save the first byte in the packetBuffer, increasing the len 1.

    When the next SYNC is received, the packetBufferReadyFlag is set, completing the packet reception.

    Note that the byteBuffer will never be 0 if we are currently reading in a packet. This is because we stuff a 1 bit in the top slot when we start a valid byte reception.
    This makes the byteBuffer a good flag to see if we are currently receiving, and setting byteBuffer to 0 will abort anything in progress and wait for next SYNC.

    Note that a packet must be at least 1 byte long, or else we would see two syncs in a row as a zero byte packet. This is valid, but would waste the buffer until the
    zero byte packet was marked as read. Byte requiring at least 1 good byte, we add the constraint that there must be at least 8 (or multipule of 8) will formed bits
    between the syncs, which greatly increases error rejection.

 */


// We keep these in together in a a struct to get faster access via
// Data Indirect with Displacement opcodes

static ir_rx_state_t ir_rx_states[IRLED_COUNT];

// Temp storage to stash the IR bits while interrupts are off.
// We will later process and decode once interrupts are back on.

// TODO: I don't think this needs to be volatile since always used in same context?

volatile uint8_t most_recent_ir_test;

  void timer_128us_callback_cli(void) {
	  // Interrupts are off, so get it done as quickly as possible
	  most_recent_ir_test = ir_sample_and_charge_LEDs();
  }

// The updateIRComs() function is probably the most optimized in this code base
// because it iterated 8 times every tick so must be fast. I also tried making it
// clearer by using a case-based state machine, but the compiler exploded that with
// code that ended up randomly blowing the stack. So here we are.


 void IrDataPeriodicUpdateComs(void) {

     // Grab which IR LEDs triggered in the last time window

    uint8_t bits = most_recent_ir_test;

    // Recharge the LEDs that were triggered on this last sample
    // Note we intentionally wait some time after the sample so that we do not see the same
    // pulse trigger use twice. Two pulses are always separated by more than 1 time window,
    // so as long as we keep up sampling we should never miss a pulse, and Actually charging the
    // LED closer to when it will get triggered by a pulse might make it less sensitive since it
    // will have more residual charge.

    // Loop though and process each IR LED (which corresponds to one bit in `bits`)
    // Start at IR5 and work out way down to IR0.
    // Going down is faster than up because we can test bitwalker == 0 for free

    // TODO: Probably more efficient to use ASM to walk bits down, and we get free zero test at the end.

    uint8_t bitwalker = _BV( IRLED_COUNT -1 );
    ir_rx_state_t *ptr = ir_rx_states + IRLED_COUNT -1;

    // Loop though each of the IR LED and see if anything happened on each...

    do {

        const uint8_t lastSample = bits & bitwalker;

        // We will directly update the master in cases where it is changed
        uint8_t windowsSinceLastTrigger = ptr->windowsSinceLastTrigger;

        // TODO: Make this more efficient.

        if (lastSample) {

            // The only interesting time to look at samples is after a pulse is received so only bother looking then

            if (windowsSinceLastTrigger<4) {

                // 0,1,2,3,4 windows are data bits....

                // If we get here, then we potentially received a data bit.

                if (ptr->byteBuffer) {      // Are we currently receiving a byte? (That is, did we ever get a sync?) If not, ignore possible bits

                    // If we get here, then we are currently receiving a byte (that is, we  got a good sync)

                    //TODO: Check ASM, might want to cache this

                    uint8_t dataBit = 0;        // Potential received data bit

                    // 0,1 windows are a '1' data bit

                    if (windowsSinceLastTrigger<=1) {

                        dataBit = 0b10000000;       // We will stuff a 1 in the highest bit of the input buffer
                                                    // because bits are sent lest significant bit first, so we must
                                                    // stuff at the top and shift downwards

                    }

                    // 2,3,4 windows are a '0' data bit - do nothing because we preloaded `dataBit` with `0`

                    #ifdef IR_RX_DEBUG
                        if (bitwalker==  _BV(IR_RX_DEBUG_LED) ) {
                            if (dataBit) {
                                    Debug::tx_now('1');
                            } else {
                                    Debug::tx_now('0');
                            }
                        }
                    #endif

                    // Here we know we got a data bit. It is either 0 or 1 (decided above)

                    uint8_t data =  ptr->byteBuffer>>1 ;    // make room at top for new 1 bit
                                                            // remember data bits are transmitted lest significant first

                    data |= dataBit;                        // data now holds the data byte in progress with the new bit in the top bit

                    // TODO: we could save some time here using an assembly shift followed by a carry bit test

                    if (ptr->byteBuffer & 0b00000001) {     // If the bottom it in the input buffer was high, then we just got the last bit of a full byte

                        // Save the fully received byte, prime for next one

                        if ( ptr->packetBufferLen < IR_RX_PACKET_SIZE ) { // Check that there is room in the buffer to store this byte

                            ptr->packetBuffer[ptr->packetBufferLen] = data;

                            ptr->packetBufferLen++;

                            ptr->byteBuffer = 0b10000000;            // prime buffer for next byte to come in. Remember this 1 will fall off the bottomm after 8 bits to indicate a full byte

                            #ifdef IR_RX_DEBUG
                                if (bitwalker== _BV(IR_RX_DEBUG_LED) ) {
                                    Debug::tx_now('B');          // Buffered byte
                                    Debug::tx( data );
                                }
                            #endif

                        } else {

                            // Packet received was too large for buffer so abort this reception.

                            ptr->byteBuffer = 0;                // this blocks any more bits from coming
                            ptr->packetBufferLen =0;            // We will need a new SYNC to start a new packet being received

                            #ifdef IR_RX_DEBUG
                                if (bitwalker== _BV(IR_RX_DEBUG_LED) ) {
                                    Debug::tx_now('V');      // Overflow
                                    Debug::tx( data );
                                }
                            #endif

                        }


                    } else {

                        // Incoming data byte still in progress, save with newly added bit

                        ptr->byteBuffer = data;         // Remember that we stuffed the new bit into `data` above

                    }

                } else { // if (!ptr->byteBuffer) - if we get something that looks like a bit but byteBuffer==0, then we never got a starting sync

                    #ifdef IR_RX_DEBUG
                        if (bitwalker == _BV(IR_RX_DEBUG_LED) ) {
                            Debug::tx_now('+');                  // got a bit outside of a frame
                        }
                    #endif

                }


            } else if (windowsSinceLastTrigger <=6 ) {

                // SYNC

                // We check that byteBuffer is 0b10000000 to make sure that we are not in the middle of Receiving a new byte, which would be an error
                // since packets should always contain full bytes between the SYNCs

                if (ptr->byteBuffer==0b10000000) {

                    // Ok, this new sync is a trailing sync since we are actively receiving and are on an 8-bit boundary

                    if (ptr->packetBufferLen) {

                        // Only save non-zero length packets because otherwise we would see two consecutive syncs
                        // as a zero byte packet and there is just not enough error rejection, so a waste to use up
                        // the buffer on these.
                        // Also ensures there is always a header byte in higher level protocols

                        ptr->packetBufferReady = 1;     // Signal to the foreground that there is data ready in the packet buffer
                        ptr->byteBuffer =0;             // stop reading until we get a new sync

                        #ifdef IR_RX_DEBUG
                            if (bitwalker==_BV(IR_RX_DEBUG_LED)) {
                                Debug::tx_now('R');      // Packet received and buffered successfully
                            }
                        #endif

                        // This is tricky - here we clear the way to send immediately after the trailing sync
                        // Otherwise the collisiosn avoidance in irDataTXinProgress() could see the last
                        // pulse of the trailing sync as a leading pulse of another sync or bit
                        // This lets use reply to this packet we just received immediately

                        ptr->windowsSinceLastTrigger = 7;

                    } else {

                        // Got a sync, but nothing in packet buffer so there were 2 consecutive syncs
                        // treat this new one as a starting sync
                        // We are already reading so don't need to reset anything

                        #ifdef IR_RX_DEBUG
                            if (bitwalker==_BV(IR_RX_DEBUG_LED)) {
                                Debug::tx_now('y');      // sYnc
                            }
                        #endif
                    }


                } else {            // if (ptr->byteBuffer!=0b10000000)

                    // If we get here, then we got a leading sync, so start reading bytes

                    if (!ptr->packetBufferReady) {               // Don't clobber data that has not been read out of the buffer yet

                        ptr->byteBuffer = 0b10000000;            // prime buffer for next byte to come in. Remember this 1 will fall off the bottom to indicate a full byte
                        ptr->packetBufferLen=0;                  // Ready to start filling the packet buffer with new packet


                        #ifdef IR_RX_DEBUG
                            if (bitwalker==_BV(IR_RX_DEBUG_LED)) Debug::tx_now('Y');      // sYnc
                        #endif

                    } else {

                        #ifdef IR_RX_DEBUG
                            if (bitwalker==_BV(IR_RX_DEBUG_LED)) Debug::tx_now('o');      // sYnc ignored becuase of buffer overflow
                        #endif

                    }


                }

            } else {  // if (windowsSinceLastTrigger > 6 )

                // Been too long since we saw pulse, so nothing happening
                // To get here, we already aborted anything in progress when the windowsSincelastTrigger was incremented to 7

                #ifdef IR_RX_DEBUG
                    if (bitwalker==_BV(IR_RX_DEBUG_LED)) Debug::tx_now('I');
                    //if (bitwalker==_BV(IR_RX_DEBUG_LED)) sp_serial_tx('0'+ptr->windowsSinceLastTrigger);      // Error
                #endif

            }

            ptr->windowsSinceLastTrigger= 0;          // We are here because we got a trigger

        } else {  // No trigger in this sample window

            // No trigger on this IR on this update cycle

            // No point in incrementing past 7 since 7 is already an error
            // and this keeps us form overflowing at 255 (although that is very unlikely)
            // This check also lets us immediately abort any RX in progress as soon as
            // it is clear that we can not make a good packet anymore.

            if (windowsSinceLastTrigger<=7) {

                windowsSinceLastTrigger++;
                ptr->windowsSinceLastTrigger=windowsSinceLastTrigger;

                // Note that we do not need to look for errors here, they will show up when the trigger finally happens.

                #ifdef IR_RX_DEBUG
                    if (bitwalker==_BV(IR_RX_DEBUG_LED)) Debug::tx_now('-');          // Idle sample
                #endif

            } else {

                // We have waited long enough that this can not be a sync pulse
                // We really only need to clear this so that irDataRXinProgress() will know that
                // any RX that ws in progress is now aborted

                #ifdef IR_RX_DEBUG
                    if (bitwalker==_BV(IR_RX_DEBUG_LED)) Debug::tx_now('A');          // Abort any RX in progress
                #endif

                ptr->byteBuffer = 0x00;            // Clear byte buffer. A 0 here means we are waiting for sync

            }


        }

        ptr--;
        bitwalker >>=1;


    } while (bitwalker);

}

// Is there potentially an RX in progress on this LED?
// Used to hold off sending while RX in progress to avoid a collisions that would clobber both transmissions

// Is there potentially an RX in progress on this LED?
// Used to hold off sending while RX in progress to avoid a collisions that would clobber both transmissions

inline uint8_t irDataRXinProgress( uint8_t led ) {

    ir_rx_state_t *ptr = ir_rx_states + led;

    // This uses the fact that anytime we are activel recieving a byte, the byte buffer will 
    // be nonzero. Even if we are getting an all-0 byte, the top bit will be marching up.
    // This is handy, but it does allow a collision during the leading sync. See below. 

    return ptr->byteBuffer;

/*

    // Below is a very conservative collision detection scheme that even detects a preamble.
    // Unfortunately with current RX code, it will see a collision for the first 7 windows
    // after a valid packet is received because the final pulse of the trailing sync
    // is in fact a pulse and could be seen as a leading pulse of a new sync. 
    
    // We could add an extra rx_in_progess flag or something like that, or find a way to force
    // the windowsSinceLastTrigger to be 7 at the end of a valid packet. 
    // Or just require there always be at least 7 windows between a valid TX and a follow up TX. 
    // We should do this, but not now.

    // Anytime we are actively receiving a message, all value windows are less than 7
    // This will even hold off in the case where we just triggered and are still waiting to
    // see if it was a valid preamble.

    if (ptr->windowsSinceLastTrigger <= 6 ) {

        Debug::tx('F');
        Debug::tx( ptr->windowsSinceLastTrigger );
                
        
        return 1;
    } else {
        return 0;
    }
    
*/    

}


// Is there a received data ready to be read on this face?

bool irDataIsPacketReady( uint8_t led ) {

    return( ir_rx_states[led].packetBufferReady);

}


// Returns 0 if no data packet ready, or the length of the  data if complete packet is ready

uint8_t irDataPacketLen( uint8_t led ) {

    if (ir_rx_states[led].packetBufferReady) {

        return ir_rx_states[led].packetBufferLen;

    }  else {
        return 0;
    }

}

const uint8_t *irDataPacketBuffer( uint8_t led ) {

    return ir_rx_states[led].packetBuffer;

}


void irDataMarkPacketRead( uint8_t led ) {

    ir_rx_states[led].packetBufferReady=0;

}


// So we need the TX window to always be longer than the sample window or else
// two TX pulses could happen in the same sample window and only one we be seen.

// Here we compute the longest sample window assuming worst case that the
// RX clock is slow. Note this is an actual real time us.

#define MINIUM_TX_WINDOW_RT_US  (IR_WINDOW_US * (1.0 + (IR_CLOCK_SPREAD_PCT/100.00) ))

// Next we compute how long each TX delay for each transmitted symbol in accurate us

#define IR_TX_1_BIT_DELAY_RT_US (( 1.0 * MINIUM_TX_WINDOW_RT_US ) + IR_CHARGE_OVERHEAD )
#define IR_TX_0_BIT_DELAY_RT_US (( 3.0 * MINIUM_TX_WINDOW_RT_US ) + IR_CHARGE_OVERHEAD )
#define IR_TX_S_BIT_DELAY_RT_US (( 5.0 * MINIUM_TX_WINDOW_RT_US ) + IR_CHARGE_OVERHEAD )     // Sync

// Next we have to determine how long each delay should be in local clock time in the worst case
// where the local clock is fast.

// MIN_DELAY_RT * 1 + ( (IR_CLOCK_SPREAD_PCT) ) = MIN_DELAY_LT

#define MIN_DELAY_LT( MIN_DELAY_RT , CLOCK_SPREAD_PCT ) ( MIN_DELAY_RT * ( 1.0 + ( CLOCK_SPREAD_PCT / 100.00) ) )

// Sends bits in least-significant-bit first order (RS232 style)

// Note that you must not dilly dally between begin, sending bytes, and ending. You don't have much time to think
// so get everything set up beforehand.

// Returns 1 if the send was successfully started, 0 if there was an RX in progroess (you should then try again)

uint8_t irSendBegin( uint8_t face ) {

    // Start things up, send initial pulses
    // We send two to cover the case where an ambient trigger happens *just* before the sync pulse
    // starts, which causes the real sync to start right in the middle of the charging, so now the
    // LED is partially discharged and predisposed to trigger again off ambient.
    // If we pre-trigger the RX led, then it will not be able to trigger on ambient in the
    // time span of the sync pulse. So this 1 bit is really meaningless, it just makes sure the
    // RX LED is starting up charged when the leading pulse of the sync comes in.
    // We don't need to worry about that 1 getting seen as a real bit since the sync comes after it
    // and a long idle window came before it.

    // TODO: We need a timeout here or else a continuous stream of SYNCs could lock us out here...
    

    if (irDataRXinProgress(face)) {
        return 0;
    }
    
    ir_tx_start( 1 << face , US_TO_CYCLES(  MIN_DELAY_LT( IR_TX_1_BIT_DELAY_RT_US , IR_CLOCK_SPREAD_PCT ))  );

    ir_tx_sendpulse( US_TO_CYCLES(  MIN_DELAY_LT( IR_TX_S_BIT_DELAY_RT_US , IR_CLOCK_SPREAD_PCT ))  ) ;


    return 1;

}

void irSendByte( uint8_t b ) {

    uint8_t bitwalker = 0b00000001;

    do {

        if ( b & bitwalker) {
            ir_tx_sendpulse( US_TO_CYCLES(  MIN_DELAY_LT( IR_TX_1_BIT_DELAY_RT_US , IR_CLOCK_SPREAD_PCT ))  ) ;
        } else {
            ir_tx_sendpulse( US_TO_CYCLES(  MIN_DELAY_LT( IR_TX_0_BIT_DELAY_RT_US , IR_CLOCK_SPREAD_PCT ))  ) ;
        }

        bitwalker<<=1;

        // TODO: Faster to do in ASM and check carry bit?

    } while (bitwalker);        // 1 bit overflows off top. Would be better if we could test for overflow bit

}

void irSendComplete() {

    // Send final SYNC
    ir_tx_sendpulse( US_TO_CYCLES(  MIN_DELAY_LT( IR_TX_S_BIT_DELAY_RT_US , IR_CLOCK_SPREAD_PCT ))  ) ;

    ir_tx_end();

}


void irDataInit() {
    #ifdef IR_RX_DEBUG
        Debug::init();
    #endif
}


// Convenience function since we send packets in a couple different places.
// This sends a user data packet. No error checking so you are responsible to do it yourself
// Returns 0 if it was not able to send because there was already an RX in progress on this face

// I bet you are wondering why the headerbyte comes at the end, right?
// Because if we put it at the beginning then chained functions (like ir_send_user_data)
// Would have to shuffle all the params around to set up the next call. At the end, the extra
// param can just get loaded and done is done!

uint8_t ir_send_data(   uint8_t face, const void *data , uint8_t len , uint8_t headerbyte  ) {

    // Ok, now we are ready to start sending!

    const uint8_t *ptr = (uint8_t *) data;      // Just type conversion so we can step though bytes.

    if (irSendBegin( face )) {

        irSendByte( headerbyte );

        while (len) {

            irSendByte( *ptr++ );
            len--;

        }

        irSendComplete();

        return 1;
    }

    return 0;

}

