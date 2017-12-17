/*

    Talk to the 6 IR LEDs that are using for communication with adjacent tiles


    THEORY OF OPERATION
    ===================
    
    All communication is currently 8 bits wide, 7 data bits followed by 1 parity bid (odd). MSB sent first. 
    
    Internally, we use an 8-bit buffer and always lead with a 1-bit, so the fully loaded value "0" has the high bit set. 
    This lets us tell the difference between "nothing" and the loaded value "0". It also lets us walk that leading 1-bit up the byte so that when
    it finally hits the MSB we know that the shifting is complete. This saves a counter. 
        
    IR_MASK by default allows interrupts on all faces. 
    TODO: When noise causes a face to wake us from sleep too much, we can turn off the mask bit for a while.
    
    MORE TO COME HERE NEED PICTURES. 


*/


#define DEBUG_MODE

#include "blinklib.h"


#include "ir.h"
#include "utils.h"
#include "timer.h"          // get US_TO_CYCLES()

#include "irdata.h"

#include <util/delay.h>         // Must come after F_CPU definition

// A bit cycle is one timer tick, currently 512us

#define IR_WINDOW_US 512            // How long is each timing window? Based on timer programming and clock speed. 
 
#define IR_CLOCK_SPREAD_PCT  10     // Max clock spread between TX and RX clocks in percent

#define TX_PULSE_OVERHEAD_US 25     // How long does ir_tx_pulse() take?

// Time between consecutive bit flashes
// Must be long enough that receiver can detect 2 distinct consecutive flashes even at maximum interrupt latency

 //#define IR_SPACE_TIME_US (IR_WINDOW_US + (( ((unsigned long) IR_WINDOW_US * IR_CLOCK_SPREAD_PCT) ) / 100UL ) - TX_PULSE_OVERHEAD )  // Used for sending flashes. Must be longer than one IR timer tick including if this clock is slow and RX is fast. 
 
  #define IR_SPACE_TIME_US (600)  // Used for sending flashes. Must be longer than one IR timer tick including if this clock is slow and RX is fast. 

// from http://www.microchip.com/forums/m587239.aspx

static uint8_t oddParity(uint8_t p) {
      p = p ^ (p >> 4 | p << 4);
      p = p ^ (p >> 2);
      p = p ^ (p >> 1);
      return p & 1;
}




// You really want sizeof( ir_rx_state_t) to be  a power of 2. It makes the emitted pointer calculations much smaller and faster.

 typedef struct {
     
     // These internal variables are only updated in ISR, so don't need to be volatile.
     
     // I think access to the  first element is slightly faster, so most frequently used should go here
     
    uint8_t windowsSinceLastFlash;          // How many times windows since last trigger? Reset to 0 when we see a trigger
          
    uint8_t inputBuffer;                    // Buffer for RX in progress. Data bits step up until high bit set.           
                                            // High bit will always be set for real data because the start bit is 1
                                           

    // Visible to outside world     
     volatile uint8_t lastValue;            // Last successfully decoded RX value. 1 in high bit means valid and unread. 0= empty. 

    
    uint8_t dummy;                          // TODO: parity bit? for now just keep struct a power of 2
                                                           
    // This struct should be even power of 2 long. 
              
 } ir_rx_state_t;
 
 
 // We keep these in together in a a struct to get faster access via
 // Data Indirect with Displacement opcodes

 ir_rx_state_t ir_rx_states[IRLED_COUNT];


// Called once per timer tick
// Check all LEDs, decode any changes
 
 // Note this runs in callback context in timercallback. 
 
 void updateIRComs(void) {
          
    uint8_t bits = ir_test_and_charge();

    /*
    
    // DEBUGC is the sample clock. A long pulse means a trigger on led IR0 durring prior window
    if ( TBI( bits , 0 ) ) SP_AUX_PULSE(50);
    else SP_AUX_PULSE(10);		 
    
    */
      
    ir_rx_state_t *ptr = ir_rx_states + IRLED_COUNT -1;
    uint8_t bitwalker = 0b00100000;


    //    #warning only processing IR0    
    //uint8_t bitwalker = 0b00000001;
    //ir_rx_state_t *ptr = ir_rx_states;;

    while (bitwalker) {
        
        uint8_t bit = bits & bitwalker;
        
        if (bit) {      // This LED triggered in the last time window
            
            uint8_t thisWindowsSinceLastFlash = ptr->windowsSinceLastFlash;
            
            ptr->windowsSinceLastFlash = 0;     // We just got a flash, so start counting over.
            
            if (thisWindowsSinceLastFlash<=3) {     // We got a valid bit
                                                
                uint8_t inputBuffer = ptr->inputBuffer;     // Compiler should do this optrimization for us, but it don't 
                
                inputBuffer <<= 1;      // Make room for new bit (fills with a 0)
                            
                if (thisWindowsSinceLastFlash<=1) {     // Saw a 1 bit
                    
                    inputBuffer |= 0b00000001;          // Save newly received 1 bit 
                    
                }
                                
                if ( inputBuffer & 0b10000000 ) {       // Do we already have a full 7-bit (plus start bit) byte in the buffer?
                    
                    // TODO: parity check would go here, but not for now. 
                    
                    // TODO: check for overrun in lastValue and either flag error or increase buffer size
                    
                    ptr->lastValue = inputBuffer;           // Save the received byte (clobbers old if not read yet)
                    
                    inputBuffer =0;                    // Clear out the input buffer to look for next start bit
                    
                } 
                
                ptr->inputBuffer = inputBuffer;
                
            }  else {
                
                // Received an invalid bit
                
                ptr->inputBuffer = 0;                       // Start looking for start bit again. 
                                                    
            }                
                
        }                
                         
        ptr--;                    
        bitwalker >>=1;
    }        
         
}     
 
// Is there a received data ready to be read on this face?

bool irIsReadyOnFace( uint8_t led ) {
    return( ir_rx_states[led].lastValue != 0 );    
}    

// Read the most recently received data. Blocks if no data ready

uint8_t irGetData( uint8_t led ) {
        
    ir_rx_state_t *ptr = ir_rx_states + led;        // This turns out to generate much more efficient code than array access. ptr saves 25 bytes. :/   Even so, the emitted ptr dereference code is awful.
    
    while (! ptr->lastValue );      // Wait for high be to be set to indicate value waiting. 
        
    // No need to atomic here since these accesses are lockstep, so the data can not be updated until we clear the ready bit        
    
    uint8_t d = ptr->lastValue;
    
    ptr->lastValue=0;       // Clear to indicate we read the value. Doesn't need to be atomic.

    return d & 0b01111111;      // Don't show our internal flag. 
    
}



static void txbit( uint8_t ledBitmask, uint8_t bit ) {
    
    // 1 blinks for a 0-bit
    
    if (bit) {
    
        ir_tx_pulses( 2 , US_TO_CYCLES( IR_SPACE_TIME_US ), ledBitmask );
        
    } else {

        ir_tx_pulses( 1 , US_TO_CYCLES( IR_SPACE_TIME_US ) , ledBitmask );
        
    }                    
    
    // Two spaces at the end of each bit

    _delay_us(IR_SPACE_TIME_US*2);

    
    
}

// Internal send to all faces with a 1 in the bitmask. 

static void irBitmaskSendData( uint8_t ledBitmask , uint8_t data ) {
    
    // Note that we do not need to mask out the top bit of data because the bitwalker does it automatically. 
    
    // TODO: Check for RX in progress here?

    // RESET link
    
    // Three consecutive pulses will reset RX state
    // We need to send 4 to ensure the RX sees at least 3
    
    ir_tx_pulses( 4 , US_TO_CYCLES( IR_SYNC_TIME_US ) , ledBitmask );
    
    // >2 spaces at the end of sync to load at least 1 zero into the bitstream


    // TODO: Maybe do a timer based delay that does
    // not accumulate additional time when interrupted?
    
    _delay_us(IR_SPACE_TIME_US*2);
    
    uint8_t bitwalker=0b01000000;                       // Send 7 bits of data
    bool parityBit = 0;

    do {
        
        bool bit = data & bitwalker;

        txbit(  ledBitmask , bit );
        
        // TODO: Faster parity calculation
        parityBit ^= bit;
        
        bitwalker >>=1;
        
    } while (bitwalker);
    
    txbit( ledBitmask, parityBit );	           // parity bit, 1=odd number of 1's in data    
}    

void irSendData( uint8_t face , uint8_t data ) {
    
    irBitmaskSendData( _BV( face ) , data );
    
}

void irBroadcastData( uint8_t data ) {
    
    
    // Find all the IR LEDs that do not currently have an RX in progress...
    
    uint8_t bitmask=0;
    
    uint8_t bitwalker = _BV( IRLED_COUNT-1 ); 
    
    ir_rx_state_t *ptr = ir_rx_states + IRLED_COUNT - 1;      
    
    while (bitwalker) {
        
        if (ptr->inputBuffer) {
            bitmask |= bitwalker;
        }            
        
        ptr--;
        
        bitwalker >>=1;
        
    }        
        
    // Send our first transmission only to those that do not have RX in progress
            
    irBitmaskSendData( bitmask , data ); 
    
    // Ok, by the time the above TX completes, whatever RX's that were in progress should be complete (each RX is a TX from the other side!)
    // so should be clear to send to those now...
    
    irBitmaskSendData( (~bitmask) &  ALL_IR_BITS , data );    
    
}