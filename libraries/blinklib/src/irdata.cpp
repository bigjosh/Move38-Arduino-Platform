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

#define IR_RX_PACKET_SIZE     34  // Maximum IR packet size. Larger values more efficient for large block transfers, but use more memory. 

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


    uint8_t paketBuffer[ IR_RX_PACKET_SIZE];        // Assemble incoming packet here
    // TODO: Deeper data buffer here?

    // Visible to outside world
     volatile uint8_t inValue;              // Last successfully decoded RX value. 1 in high bit means valid and unread. 0= empty.

     volatile uint8_t inValueState;         // Newly decoded value in inValue. Could be a bitflag - that would save a byte but cost a cycle.
     //volatile uint8_t parity;               // Top bit is received parity for value in inValue. Up to foreground to test. Could be a bitflag - that would save a byte but cost a cycle.

    // This struct should be even power of 2 long.

 } ;
 
 #define INVALUESTATE_NONE      0           // Waiting for sync and 1st byte
  
// We keep these in together in a a struct to get faster access via
// Data Indirect with Displacement opcodes

static volatile ir_rx_state_t ir_rx_states[IRLED_COUNT];

// Temp storage to stash the IR bits while interrupts are off.
// We will later process and decode once interrupts are back on.

// TODO: I don't think this needs to be volatile since always used in same context?

volatile uint8_t most_recent_ir_test;

  void timer_128us_callback_cli(void) {
	  // Interrupts are off, so get it done as quickly as possible
	  most_recent_ir_test = ir_sample_and_charge_LEDs();
  }


#if defined( TX_DEBUG ) || defined( RX_DEBUG )

    #include "sp.h"

    namespace debug {
        const uint8_t myState_count = 5;

        byte decode( byte v ) {

            return( v % myState_count );

        }


        byte test( byte v ) {

            byte orginal = decode( v ) ;

            byte inverted =  ( myState_count -1 - orginal ) ;

            byte calculatedInvertedTruncated = inverted % 8;


            byte recoveredInvertedTruncated = v / myState_count ;


            return calculatedInvertedTruncated == recoveredInvertedTruncated;

        }
    }
#endif

// The updateIRComs() function is probably the most optimized in this code base
// because it iterated 8 times every tick so must be fast. I also tried making it
// clearer by using a case-based state machine, but the compiler exploded that with
// code that ended up randomly blowing the stack. So here we are.


 void updateIRComs(void) {

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
    ir_rx_state_t volatile *ptr = ir_rx_states + IRLED_COUNT -1;
    
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
                
                if (ptr->byteBuffer) {      // Are we currently receiving a byte? (That is, did we get a sync?) If not, ignore everything
                    
                    //TODO: Check ASM, might want to cache this
                
                    uint8_t dataBit = 0;        // Potential received data bit
                
                    // 0,1 windows are a '1' data bit
                
                    if (windowsSinceLastTrigger<=1) {

                        dataBit = 0b10000000;       // We will stuff a 1 in the highest bit of the input buffer
                                                    // because bits are sent lest significant bit first, so we must
                                                    // stuff at the top and shift downwards
                                                
                    }
                
                    // 2,3,4 windows are a '0' data bit
                                                                
                    #ifdef RX_DEBUG
                        if (bitwalker==0x01) {
                            if (dataBit) {
                                    SP_SERIAL_TX_NOW('1');
                            } else {
                                    SP_SERIAL_TX_NOW('0');
                            }
                        }
                    #endif

                    // Here we know we got a data bit. It is either 0 or 1 (decided above) 

                    uint8_t data =  ptr->byteBuffer>>1 ;    // make room at top for new 1 bit
                                                                // remember data bits are transmitted lest significant first 

                    data |= dataBit;                         // data now holds the data byte in progress with the new bit in the top bit 

                    if (ptr->byteBuffer & 0b00000001) {     // If the bottom it in the input buffer was high, then we just got the last bit of a full byte

                        // Save the fully received byte, prime for next one

                        ptr->inValue = data;
                        ptr->inValueState=1;            // Signal new value received.

                        ptr->byteBuffer = 0x00;        // Clear input buffer so we need another SYNC to receive next byte
                                    
                        // TODO: Continuous back to back bytes with no sync in between?
                        //ptr->inputBuffer = 0b10000000;  // prime buffer for next byte to come in. Remember this 1 will fall off the bottom to indicate a full byte


                        #ifdef RX_DEBUG
                            if (bitwalker==0x01) {
                                SP_SERIAL_TX_NOW('C');      // Complete byte
                                sp_serial_tx( data );

                                if ( !debug::test( data ) ) {
                                    SP_PIN_A_SET_1();               // SP pin A goes high any time IR0 is triggered. We clear it when we later process in the polling code.                                               
                                    SP_PIN_A_SET_0();                                    
                                }
                            }                                            

                        #endif

                    } else {

                        // Incoming data byte still in progress, save with newly added bit

                        ptr->byteBuffer = data;

                        #ifdef RX_DEBUG
                            if (bitwalker==0x01) {
                                SP_SERIAL_TX_NOW('B');  // Buffered bit
                                sp_serial_tx( data );
                            }                                            
                        #endif

                    }
                    
                }                    
                
            } else if (windowsSinceLastTrigger <=6 ) {
                
                // SYNC 

                ptr->byteBuffer = 0b10000000;            // prime buffer for next byte to come in. Remember this 1 will fall off the bottom to indicate a full byte

                #ifdef RX_DEBUG
                    if (bitwalker==0x01) SP_SERIAL_TX_NOW('Y');      // sYnc
                #endif
                
            } else {                 
                
                // Been too long since we saw pulse, so reset everything and wait for next sync

                ptr->byteBuffer = 0x00;            // Clear buffer. A 0 here means we are waiting for sync

                #ifdef RX_DEBUG
                    if (bitwalker==0x01) SP_SERIAL_TX_NOW('E');                               // Error
                    if (bitwalker==0x01) sp_serial_tx('0'+ptr->windowsSinceLastTrigger);      // Error                                
                #endif


            }

            ptr->windowsSinceLastTrigger= 0;          // We are here because we got a trigger

        } else {  // No trigger in this sample window

            // No trigger on this IR on this update cycle
            
            // No point in incrementing past 9 since 9 is already an error
            // and this keeps us form overflowing (although that is very unlikely)
            if (windowsSinceLastTrigger<9) {
                
                windowsSinceLastTrigger++;

                ptr->windowsSinceLastTrigger=windowsSinceLastTrigger;
                
            }                

            #ifdef RX_DEBUG
                if (bitwalker==0x01) SP_SERIAL_TX_NOW('-');          // Idle sample
            #endif

        }

        ptr--;
        bitwalker >>=1;


    } while (bitwalker);

}

// Is there a received data ready to be read on this face?

bool irIsReadyOnFace( uint8_t led ) {
    return( ir_rx_states[led].inValueState );
}

// Read the most recently received data. (Defaults to 0 on power-up)
// Requires interrupts to be on

uint8_t irGetData( uint8_t led ) {

    uint8_t data;
    //uint8_t parity;

    ir_rx_state_t volatile *ptr = ir_rx_states + led;        // This turns out to generate much more efficient code than array access. ptr saves 25 bytes. :/   Even so, the emitted ptr dereference code is awful.

    // We need this to be atomic otherwise would could potentially miss an incoming value that arrives
    // exactly between when we collect the data and clear the inValueReady

    ATOMIC_BLOCK( ATOMIC_FORCEON ) {
        data = ptr->inValue;
        //parity = ptr->parity;
        ptr->inValueState=0;        // Clear to indicate we read the value. Doesn't need to be atomic.
    };


    return data ;

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

// Simultaneously send data on all faces that have a `1` in bitmask
// Sends bits in least-significant-bit first order (RS232 style)

void irSendDataBitmask(uint8_t data, uint8_t bitmask) {
    
    uint8_t bitwalker = 0b00000001;

    // Start things up, send initial pulses
    // We send two to cover the case where an ambient trigger happens *just* before the sync pulse
    // starts, which causes the real sync to start right in the middle of the charging, so now the 
    // LED is partially discharged and predisposed to trigger again off ambient.
    // If we pre-trigger the RX led, then it will not be able ot trigger on ambient in the
    // time span of the sync pulse. So this 1 bit is really meaningless, it just makes sure the
    // RX LED is starting up charged when the leading pulse of the sync comes in.
    // We don't need to worry about that 1 getting seen as a real bit since the sync comes after it 
    // and a long idle window came before it. 
    ir_tx_start( bitmask , US_TO_CYCLES(  MIN_DELAY_LT( IR_TX_S_BIT_DELAY_RT_US , IR_CLOCK_SPREAD_PCT ))  );
    ir_tx_sendpulse( US_TO_CYCLES(  MIN_DELAY_LT( IR_TX_S_BIT_DELAY_RT_US , IR_CLOCK_SPREAD_PCT ))  ) ;

    do {

        if (data & bitwalker) {
            ir_tx_sendpulse( US_TO_CYCLES(  MIN_DELAY_LT( IR_TX_1_BIT_DELAY_RT_US , IR_CLOCK_SPREAD_PCT ))  ) ;
        } else {
            ir_tx_sendpulse( US_TO_CYCLES(  MIN_DELAY_LT( IR_TX_0_BIT_DELAY_RT_US , IR_CLOCK_SPREAD_PCT ))  ) ;
        }

        bitwalker<<=1;

    } while (bitwalker);        // 1 bit overflows off top. Would be better if we could test for overflow bit

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
