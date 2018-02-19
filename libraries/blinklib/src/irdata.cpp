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

// Note that POST_ILDE is the only state that is not exited with a trigger


enum IR_RX_STATE {
	PRE_IDLE,			// Count is number of consecutive idle windows
						// If non-idle window seen -> exit to PRE_IDLE count=0
						// If trigger and count>=4 -> exit to START_1

	START_1_0,			// Count is number of consecutive idle windows
						// if 0-bit received AND count>0 -> exit to START_0
						// if count>2 -> exit to PRE_IDLE

	START_1_1,

	START_0,			// count is number of consecutive 0-bit symbols
						// if count==1 -> exit to BIT
						// if 1-bit received or anything else -> exit to PRE_IDLE

	BIT0,				// count is number of consecutive consecutive idle windows
						// if count==8 -> exist to partity
						// invalid bit-> exit to PRE_IDLE

	BIT1,
	BIT2,
	BIT3,
	BIT4,
	BIT5,
	BIT6,
	BIT7,

	PARITY,				// Count number of valid bits received
						// if received valid bit AND even parity with byte -> exist to POST_IDLE
						// anything else -> exit to PRE_IDLE

	POST_IDLE,			// Count is consecutive idle guard windows received
						// If count==4 -> return byte, exit to START_1
};

const char *statechar    = "SABC01234567PE";
const char *statecharlow = "sabc01234567pe";

// You really want sizeof( ir_rx_state_t) to be  a power of 2. It makes the emitted pointer calculations much smaller and faster.
// TODO: Do we need all these volatiles? Probably not...

 typedef struct {

     // These internal variables are only updated in ISR, so don't need to be volatile.

     // I think access to the  first element is slightly faster, so most frequently used should go here

    uint8_t volatile windowsSinceLastFlash;          // How many times windows since last trigger? Reset to 0 when we see a trigger

	IR_RX_STATE state;						// Current read state

	uint8_t count;							// Different meaning in different states

    uint8_t inputBuffer;                    // Buffer for RX in progress. Data bits step up until high bit set.
                                            // High bit will always be set for real data because the start bit is 1


    // Visible to outside world
     volatile uint8_t inValue;            // Last successfully decoded RX value. 1 in high bit means valid and unread. 0= empty.

     volatile uint8_t inValueReady:1;         // Newly decoded value in inValue
          

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

#warning
#include "sp.h"   

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

             if (bitwalker==0x1) {
                 sp_serial_tx( statechar[ ptr->state ] );
                 sp_serial_tx( thisWindowsSinceLastFlash + '0' );
             }

			 switch (ptr->state) {

				 case PRE_IDLE:

						if (thisWindowsSinceLastFlash >= 4) {
							ptr->state = START_1_0;
						}

					break;

				case START_1_0:

						if (thisWindowsSinceLastFlash <=1 ) {		// 1-bit symbol received
							ptr->state = START_1_1;
						} else {
							ptr->state = PRE_IDLE;
						}

					break;

				case START_1_1:

						if (thisWindowsSinceLastFlash <=1 ) {		// Extra 1-bit symbol received (this can happen if the TX start bit happens right after a spontainious trigger)
							ptr->state = START_0;
						} else if ( thisWindowsSinceLastFlash <=3 ) {	// 0-bit symbol received
							ptr->state = BIT7;
						} else {
							ptr->state = PRE_IDLE;
						}

					break;

				case START_0:

						if (thisWindowsSinceLastFlash <=1 ) {		// 1-bit symbol received
							ptr->state = PRE_IDLE;
						} else if ( thisWindowsSinceLastFlash <=3 ) {	// 0-bit symbol received
							ptr->state = BIT7;
						} else {
							ptr->state = PRE_IDLE;
						}

					break;

				case BIT7:
                
    				    ptr->inputBuffer = 0;           // Clear out any old values
                
						if ( thisWindowsSinceLastFlash <=3 ) {		// valid symbol received

							if (thisWindowsSinceLastFlash <=1 ) {		// 1-bit symbol received
								ptr->inputBuffer |= _BV(7);
							} 
                            
							ptr->state = BIT6;

						} else {

							// Start searching again from scratch
							ptr->state = PRE_IDLE;
						}

					break;

				case BIT6:

						if ( thisWindowsSinceLastFlash <=3 ) {		// valid symbol received

							if (thisWindowsSinceLastFlash <=1 ) {		// 1-bit symbol received
								ptr->inputBuffer |= _BV(6);
							}

							ptr->state = BIT5;

						} else {

							// Start searching again from scratch
							ptr->state = PRE_IDLE;
						}

				break;


				case BIT5:

                        #warning only for 5 bit
    				    ptr->inputBuffer = 0;           // Clear out any old values

						if ( thisWindowsSinceLastFlash <=3 ) {		// valid symbol received

							if (thisWindowsSinceLastFlash <=1 ) {		// 1-bit symbol received
								ptr->inputBuffer |= _BV(5);
							} 
                            
							ptr->state = BIT4;

						} else {

							// Start searching again from scratch
							ptr->state = PRE_IDLE;
						}

				break;

				case BIT4:

						if ( thisWindowsSinceLastFlash <=3 ) {		// valid symbol received

							if (thisWindowsSinceLastFlash <=1 ) {		// 1-bit symbol received
								ptr->inputBuffer |= _BV(4);
							}

							ptr->state = BIT3;

						} else {

							// Start searching again from scratch
							ptr->state = PRE_IDLE;
						}

				break;

				case BIT3:

						if ( thisWindowsSinceLastFlash <=3 ) {		// valid symbol received

							if (thisWindowsSinceLastFlash <=1 ) {		// 1-bit symbol received
								ptr->inputBuffer |= _BV(3);
							}

							ptr->state = BIT2;

						} else {

							// Start searching again from scratch
							ptr->state = PRE_IDLE;
						}

				break;



				case BIT2:

						if ( thisWindowsSinceLastFlash <=3 ) {		// valid symbol received

							if (thisWindowsSinceLastFlash <=1 ) {		// 1-bit symbol received
								ptr->inputBuffer |= _BV(2);
							}

							//ptr->state = PARITY;

                            ptr->inValue = ptr->inputBuffer;
                            ptr->state = BIT1;

						} else {

							// Start searching again from scratch
							ptr->state = PRE_IDLE;
						}

				break;




				case BIT1:

						if ( thisWindowsSinceLastFlash <=3 ) {		// valid symbol received

							if (thisWindowsSinceLastFlash <=1 ) {		// 1-bit symbol received
								ptr->inputBuffer |= _BV(1);
							}

                            ptr->state = BIT0;

						} else {

							// Start searching again from scratch
							ptr->state = PRE_IDLE;
						}

				break;


				case BIT0:

						if ( thisWindowsSinceLastFlash <=3 ) {		// valid symbol received

							if (thisWindowsSinceLastFlash <=1 ) {		// 1-bit symbol received
								ptr->inputBuffer |= _BV(0);
							}

							//ptr->state = PARITY;
                            ptr->state = POST_IDLE;

						} else {

							// Start searching again from scratch
							ptr->state = PRE_IDLE;
						}

				break;

				case PARITY:

						if ( thisWindowsSinceLastFlash <=3 ) {		// valid symbol received

							if (thisWindowsSinceLastFlash <=1 ) {		// 1-bit symbol received
								ptr->inputBuffer = _BV(1);
							} else {
								ptr->inputBuffer = 0;
							}

							ptr->state = BIT2;

						} else {

							// Start searching again from scratch
							ptr->state = PRE_IDLE;
						}

				break;
                
				case POST_IDLE:
                
                        // Error condition: we got a flash while waiting for the idle 
                        // guard time after a received byte 
                                              
                        // Give up and start again

						// Start searching again from scratch
						ptr->state = PRE_IDLE;
						

				break;
                

            }      //  switch (ptr->state)


            if (bitwalker==0x1) {
                sp_serial_tx( statechar[ ptr->state ] );
            }
             
            SP_PIN_R_SET_0();

        } else {        // if (!bit)
            

            if (bitwalker==0x01) {
                sp_serial_tx( statecharlow[ ptr->state ] );                
                sp_serial_tx( ptr->windowsSinceLastFlash + '0' );
            }                       

            if (ptr->windowsSinceLastFlash<4) {     // Higher than 4 doesn't add any meaning,  so don't overflow
                
                ptr->windowsSinceLastFlash++;       
            
            
			    // The most number of windows for a Valid bit is 3
			    // so if we are at 4 now, then we already know we have gone too long for a valid bit so no point in
			    // counting higher (and maybe overflowing)

			    // When we see an idle time longer than the longest possible valid bit, then we check to
			    // see if a valid byte has been received.

			    // By waiting for an idle, we protect against extraneous triggers from ambient light
			    // since they will cause us to have too many bits.
			    // The idle at the end also makes it less like to see a long string if extraneous
			    // triggers as a valid byte because it would just happen to have a break at just the right moment.

                if (ptr->windowsSinceLastFlash==4) {

                    if (ptr->state==POST_IDLE) {


                        if (bitwalker==0x1) {
                            sp_serial_tx( 'R' );
                            sp_serial_tx( ptr->inputBuffer );
                        }            
                    
                    
                        // Did we have a full received byte just waiting for the idle window?

                        ptr->inValue = ptr->inputBuffer;            // Valid! Pass it on!
                        ptr->inValueReady  = 1;                          // Signal new value ready

                    }
                
               
                     ptr->state = PRE_IDLE;          // Any time we see 4 idle window, then we are reset and ready for new sync                
                                                    // Note that we leave the counter at 4 since this idle counts for the begining of the next byte as well
                }                                                    
                
            }                                                        
            
        }
        
        SP_PIN_A_SET_0();     

        ptr--;
        bitwalker >>=1;


    } while (bitwalker);

}

// Is there a received data ready to be read on this face?

bool irIsReadyOnFace( uint8_t led ) {
    return( ir_rx_states[led].inValueReady );
}

// Read the most recently received data. (Defaults to 0 on powerup)

uint8_t irGetData( uint8_t led ) {

    ir_rx_state_t volatile *ptr = ir_rx_states + led;        // This turns out to generate much more efficient code than array access. ptr saves 25 bytes. :/   Even so, the emitted ptr dereference code is awful.

    uint8_t d = ptr->inValue;

    ptr->inValueReady=0;        // Clear to indicate we read the value. Doesn't need to be atomic.

    return d ;      

}

// Simultaneously send data on all faces that have a `1` in bitmask

void irSendDataBitmask(uint8_t data, uint8_t bitmask) {

    uint8_t bitwalker = 0b10000000;

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
