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

#define IR_SPACE_TIME_US (150)  // Used for sending flashes.
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


enum IR_RX_STATE {
    
    IRS_WAIT,           // Waiting fir 1st flash or potenial preable
    
	IRS_PREAMBLE,	    // Reading preamble 
                        // Input buffer has most recently received bits
                        // When the sequence 0b10 is seen, then exit to DATA

	IRS_DATA,		    // bitcount is number of valid bits remaining
						// if count==0 -> save received byte to invalue, exit to partity
						// invalid bit-> exit to PREAMBLE

	//IRS_PARITY,			// if received valid bit AND even parity with byte -> exit to IDLE
						// anything else -> exit to PREAMBLE

	IRS_IDLE,			// Wait for guard idle time after byte 
                        // If 4 or more idle windows, signal received byte good and exist to PREAMBLE
                        // 4 is chosen because the maximum number of windows for a valid bit is 3
                        // If a flash is seen too soon, exit to preamble. 
};

const char *statechar    = "WPDI";
const char *statecharlow = "wpdi";

// TODO: Do we need all these volatiles? Probably not...

struct ir_rx_state_t {
    

     // These internal variables are only updated in ISR, so don't need to be volatile.

    uint8_t windowsSinceLastFlash;                  // How many times windows since last trigger? Reset to 0 when we see a trigger

	byte state;						                // Current read state

	uint8_t bitCount;							    // Number of bits left in the byte in progress

    uint8_t inputBuffer;                            // Buffer for RX in progress. Data bits shift up until full. 
    

    // Visible to outside world
     volatile uint8_t inValue;            // Last successfully decoded RX value. 1 in high bit means valid and unread. 0= empty.

     volatile uint8_t inValueReady:1;         // Newly decoded value in inValue


    // This struct should be even power of 2 long.

 } ;


 // We keep these in together in a a struct to get faster access via
 // Data Indirect with Displacement opcodes

 static volatile ir_rx_state_t ir_rx_states[IRLED_COUNT];

 // Temp storage to stash the IR bits while interrupts are off.
 // We will later process and decode once interrupts are back on.

// TODO: I don't think this needs to be volatile since always used in same context?

 volatile uint8_t most_recent_ir_test;

  void timer_256us_callback_cli(void) {
	  // Interrupts are off, so get it done as quickly as possible
	  most_recent_ir_test = ir_test_and_charge_cli();
  }

#warning
#include "sp.h"

// The updateIRComs() function is probably the most optimized in this code base
// because it iterated 8 times every tick so must be fast. I also tried making it 
// clearer by using a case-based state machine, but the compiler exploded that with
// code that ended up randomly blowing the stack. So here we are.


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

        const uint8_t bit = bits & bitwalker;
        
        // Keep a local read only copy of state
        // We will directly update the master in cases where it is changed
        const uint8_t state = ptr->state;
        

        if (bit) {      // This LED triggered in the last time window
            
            // Cache the value before we reset it
            const uint8_t thisWindowsSinceLastFlash = ptr->windowsSinceLastFlash;

            ptr->windowsSinceLastFlash = 0;     // We just got a flash, so start counting over.


            #warning Output current state for debuging
            if (bitwalker==0x1) {
                sp_serial_tx( statechar[ ptr->state ] );
            }
            
            
            uint8_t thisInputBuffer = ptr->inputBuffer;
                        
            if (state == IRS_WAIT ) {
                
                // We got the flash we have been waiting for!
                // This was the first flash so now we can start looking for a bit
                // that might be the first bit of a preamble. 
                
                thisInputBuffer=0;
                ptr->state = IRS_PREAMBLE;
                
            } else {
                
                // not in WAIT...                
            
                if (state==IRS_IDLE) {
                
                    // If we are in IDLE, then we did not see the needed 4 idle windows
                    // yet, so this flash means that the preceding byte was no good
                    // so we ignore it, but we register this flash because it might be the 
                    // start of a preamble bit

                    thisInputBuffer=0;                
                    ptr->state = IRS_PREAMBLE;
                                
                }                
            
                // Note that we do not need to check if the windows since last flash 
                // is less than 4 since it MUST be less than 4 if we are in DATA state since
                // the case that counts the idle windows will bump us back to PREABLE
                // anytime 4 empty windows are seen. 
            
                // Shift up the input buffer to make room for new bit
                thisInputBuffer >>= 1;
                                
		        if (thisWindowsSinceLastFlash <=1 ) {		    // 1-bit symbol received
			        thisInputBuffer |= 0b10000000;
                }
                
                // If not a 1-bit, then it was a 0-bit so the shift will leave the new 0 in the inputBuffer
                // Note that when we are waiting for the preamble, this will harmlessly
                // stuff a 0 in the inputBuffer in the first flash
                
                // So now inputBuffer has the new bit in it
                
                if (state == IRS_PREAMBLE) {
                    
                    // Remember that we reassemble bits high to low, so the preamble pattern
                    // of 10 will look like 01 in the inputBuffer
                    
                    if ( (thisInputBuffer & 0b11000000) == 0b01000000 ) {       // Check for preamble
                        ptr->state = IRS_DATA;
                        ptr->bitCount =8;                  // Number of bits to read
                        // No need to clear the input buffer here since it will all get shifted out anyway over the next 8 bits
                    }  
                                          
                } else if (state == IRS_DATA) {
                                        
                    ptr->bitCount--;
                    
                    // TODO: CHeck flag in ASM is faster
                    
                    if ( ptr->bitCount == 0  ) {
                        
                        // Got full byte 
                        
                        ptr->inValue = thisInputBuffer;         // Save the assembled byte
                        
                        ptr->state = IRS_IDLE;                  // Wait for the idle period before accepting it as valid
                        
                    } 
                    
                }             
            }                

            #warning Output current state for debuging
            if (bitwalker==0x1) {            
                sp_serial_tx( thisInputBuffer );
            }                
                                                                   
            ptr->inputBuffer = thisInputBuffer;         // Save the changes to the inputBuffer                      
            
        }  else {               // No flash in this last window
            
            #warning Output current state for debuging
            if (bitwalker==0x1) {
                sp_serial_tx( statecharlow[ ptr->state ] );
            }
                        
            // Check if it has been 4 windows since last flash 
            // Since any valid bit is only 3 windows, no need to increment past 
            // 4 anyway, and this is faster and keeps up from ever overflowing. 
            
            if (ptr->windowsSinceLastFlash == 3) {
            
                if (state == IRS_IDLE ) {
                                    
                    // Successfully waited it out, so the data we received is value
                    // pass it on up...
                    
                    #warning Output current state for debuging
                    if (bitwalker==0x1) {            
                        sp_serial_tx( ptr->inputBuffer );


                        //----
                        
                        #warning debug
                        uint8_t top = ptr->inputBuffer >> 4;
                        uint8_t bot = ptr->inputBuffer & 0x0f;
                    
                        if ( top != ( bot ^ 0x0f ) ) {
                            SP_PIN_R_SET_1();
                        }                        
                    
                        //---- test for errors in encoded data
                        
                        
                    }                
                    
                    
                    
                    ptr->inValueReady = 1; 
                    
                }                    
                
                ptr->state = IRS_WAIT;              // Yes, this will get benignly repeated after the 4th idle window. It is ok. It is cheaper than the test to skip it. 
                
            } else {
                
                ptr->windowsSinceLastFlash++;
                
            }
            
        }

        #warning debug
        SP_PIN_A_SET_0();
        SP_PIN_R_SET_0();

        ptr--;
        bitwalker >>=1;


    } while (bitwalker);

}

// Is there a received data ready to be read on this face?

bool irIsReadyOnFace( uint8_t led ) {
    return( ir_rx_states[led].inValueReady );
}

// Read the most recently received data. (Defaults to 0 on power-up)

uint8_t irGetData( uint8_t led ) {

    ir_rx_state_t volatile *ptr = ir_rx_states + led;        // This turns out to generate much more efficient code than array access. ptr saves 25 bytes. :/   Even so, the emitted ptr dereference code is awful.

    uint8_t d = ptr->inValue;

    ptr->inValueReady=0;        // Clear to indicate we read the value. Doesn't need to be atomic.

    return d ;

}

// Simultaneously send data on all faces that have a `1` in bitmask
// Sends bits in least-significant-bit first order (RS232 style)

void irSendDataBitmask(uint8_t data, uint8_t bitmask) {

    uint8_t bitwalker = 0b00000001;

    // Start things up, send initial pulse and start bit (1)
    ir_tx_start( IR_SPACE_TIME_TICKS , bitmask , 1 );

    ir_tx_sendpulse( 3 );           // Guard 0 bit to ensure real start bit is detected and not extraneous leading pulse.

    do {

        if (data & bitwalker) {
            ir_tx_sendpulse( 1 ) ;
        } else {
            ir_tx_sendpulse( 3 ) ;
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
