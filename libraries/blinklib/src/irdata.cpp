/*

    Talk to the 6 IR LEDs that are using for communication with adjacent tiles


    THEORY OF OPERATION
    ===================
    
    All communication is 7 bits wide. This leaves us with an 8th bit to use as a canary bit to save needing any counters.
    
    Bytes are transmitted least significant bit first.
    
    IR_MASK by default allows interrupts on all faces. 
    TODO: When noise causes a face to wake us from sleep too much, we can turn off the mask bit for a while.
    
    MORE TO COME HERE NEED PICTURES. 


*/


#define DEBUG_MODE


#include "debug.h"
#include "blinklib.h"

const uint8_t x = ERRORBIT_DUMMY;




#include "ir.h"
#include "utils.h"




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

// Time between consecutive sync flashes
// A sync is 4 consecutive flashes that must be seen as the RX as at least 3

//#define IR_SYNC_TIME_US (IR_WINDOW_US - (( ((unsigned long) IR_WINDOW_US * IR_CLOCK_SPREAD_PCT) ) / 100UL ) - TX_PULSE_OVERHEAD  )  // Used for sending flashes. Must be longer than one IR timer tick including if this clock is slow and RX is fast.

 #define IR_SYNC_TIME_US (300)  // Used for sending flashes. Must be longer than one IR timer tick including if this clock is slow and RX is fast.

// from http://www.microchip.com/forums/m587239.aspx

static uint8_t oddParity(uint8_t p) {
      p = p ^ (p >> 4 | p << 4);
      p = p ^ (p >> 2);
      p = p ^ (p >> 1);
      return p & 1;
}



// TODO: Smaller window to increase bandwidth


// You really want sizeof( ir_rx_state_t) to be  a power of 2. It makes the emmited pointer calculations much smaller and faster.


 typedef struct {
     
     // These internal variables are only updated in ISR, so don't need to be volatile.
     
     // I think access to this first element is slightly faster, so most frequently used should go here
     
     uint8_t bitstream;               // The tail of the last sample window states. 
     
     uint8_t buffer;                  // Buffer for RX in progress. Bits step up until high bit set.           

    // Visible to outside world     
     volatile uint8_t lastValue;      // Last successfully decoded RX value. 1 in high bit means valid and unread. 0= empty. 
     
     volatile uint8_t errorBits;      // Error conditions seen 
     
     
          
     // This struct should be even power of 2 long. See assert below. 
              
 } ir_rx_state_t;


 // We keep these in together in a a struct to get faster access via
 // Data Indirect with Displacement opcodes

 ir_rx_state_t ir_rx_states[IRLED_COUNT];
  
// Got a valid bit. Deserialize it.

static void gotBit(ir_rx_state_t *ptr, bool bit ) {
    
    
    if (bit)    DEBUGB_PULSE(40);
    else DEBUGB_PULSE(20);
    
    uint8_t buffer=ptr->buffer;
    
    if (buffer) {           // There must be at least a leading 1-bit from the sync pulse or else we are not sync'ed
        

        if (buffer & 0b10000000) {     // We have already deserialized 7 bits, so this final bit is parity
        

        
            
            // TODO: Do we even need a parity check? Can any error really get though?

            
            if ( oddParity( buffer ) != bit ) {     // Parity checks (negate becuase of the leading non-data 1-bit

                DEBUGA_BITS( buffer );
                
                if (ptr->lastValue) {   // Already have a value?

                    // Signal the overflow happened if anyone cares to check
                    
                    SBI( ptr->errorBits , ERRORBIT_OVERFLOW );
                                                           
                }             
                
                
                ptr->lastValue = buffer;                    // Save new value (overwrites in case of overflow)
                
            }  else {

                // Signal the parity error happened if anyone cares to check
                
                SBI( ptr->errorBits , ERRORBIT_PARITY );
                    
            }                     
            
            buffer=0;   // Start searching for next sync. 
            
        } else {  // buffer not full yet
            
            buffer <<= 1;
            
            buffer |= bit;
            
        }            
        
        ptr->buffer=buffer;
        
    }        
    
}    

// Got a sync symbol. Start parsing bits.

static void sync(ir_rx_state_t  *ptr) {
    
    ptr->buffer=0b00000001;         // This will walk up to the 8th bit to single a full deserialization
    
}    


// Start searching for sync again

static void reset(ir_rx_state_t  *ptr, uint8_t errorReasonBit ) {
    
    ptr->buffer=0;         // Start searching for next sync. 
    SBI( ptr->errorBits , errorReasonBit );
    
    
}


// Called once per timer tick
// Check all LEDs, decode any changes
 
 
 void updateIRComs(void) {
     
    uint8_t bits = ir_test_and_charge();
    
    if ( TBI( bits , 0 ) ) DEBUGC_PULSE(50);
    else DEBUGC_PULSE(10);
    
//    if (ir_rx_states->buffer) DEBUGA_1();
//    else DEBUGA_0();
    

  
#warning only processing IR0    
//    ir_rx_state_t *ptr = ir_rx_states + IRLED_COUNT -1;
//    uint8_t bitwalker = 0b00100000;
    
    uint8_t bitwalker = 0b00000001;
    ir_rx_state_t *ptr = ir_rx_states;;

    while (bitwalker) {
        
        bool bit = bits & bitwalker;
        
        uint8_t bitstream = ptr->bitstream;
        
        bitstream <<= 1;                // Shift up 1 to make room for new bit
        
        bitstream |= bit;               // Save the new bit
        
        // This can be massively optimized once we get the algorithm right
        
        if ( (bitstream & 0b00011111) == 0b00000100 ) {     // Got a valid 0-bit symbol
            gotBit( ptr , 0 );
        } else if ( (bitstream & 0b00111111) == 0b00001100 ) {   // Got a valid 1-bit symbol (no gap between pulses)
            gotBit( ptr , 1 );
        } else if ( (bitstream & 0b01111111) == 0b00010100 ) {   // Got a valid 1-bit symbol (gap between pulses)
            gotBit( ptr , 1 );
        } else if ( (bitstream & 0b00000111) == 0b00000111 ) {   // Got a sync (can happen again on next sample when 4 consecutive pulses) 
            sync( ptr );
//        } else if ( (bitstream & 0b00001111) == 0b00000000 ) {   // TODO: Re-enstate this shorter window. Too long since last trigger. 
//            reset(ptr , ERRORBIT_DROPOUT );                      // This was triggering because we were taking too long. Relax for now. 
        } else if ( (bitstream & 0b00011111) == 0b00000000 ) {   // Too long since last trigger
            reset(ptr , ERRORBIT_DROPOUT );           
        } else if ( (bitstream & 0b00011111) == 0b00010101 ) {   // Not a valid pattern. Noise. 
            reset(ptr , ERRORBIT_NOISE);
        }            

        
                
        
        ptr->bitstream = bitstream;
                         
        ptr--;                    
        bitwalker >>=1;
    }        
    
//    if (ir_rx_states->buffer) DEBUGA_1();
//    else DEBUGA_0();
    
     
}     
 
void led_onTick(void) {
    
    
    /*
    
    // Get the number of pulses that happened in the last tick
    	        
    uint8_t pulses= ptr->pulse_count;                // This compiles nicely to Z with offset		


		do {
			#warning debug code

			// DEBUG

			if (ptr==ir_rx_states) {
		        uint8_t a= pulses;                // This compiles nicely to Z with offset
	 	 	 
		        while (a--) {
                    DEBUGC_1();
			        _delay_us(10);
                    DEBUGC_0();
			        _delay_us(10);
		        }
		    }

		} while (false); 


    if (pulses==0) {

	
        // No pulses in the last tick, so check to see if there is a symbol receipt in progress...

        uint8_t accumulator = ptr->pulse_accumulator;
       			    
        if (accumulator>0) {
            
            // We got a valid symbol. Process it. 
            
            decode_symbol( ptr ,  accumulator );
            
            ptr->pulse_accumulator=0;
            
            ptr->tickCount=1;           // One consecutive idle window so far (this one!)...
        
        } else {        // accumulator == 0
        
            // current tick idle, previous tick was also idle
			// Remember that a tick is only 1/2 of a window
			
			ptr->tickCount++;
               
            // TODO: Reduce this to 2 when we confirm max interrupt latency can't make us blow out into a next window.  Will improve error detection  when signal is very weak. Does it make a practical difference? Probably not becuase other filters will catch it. 
               
            if ( ptr->tickCount > 3) {               // maximum number of consecutive idle windows in a frame is 1 - which is 3 ticks. We give an extra here until we know max interrupt latency. 
                
                // There have been more than 2 consecutive idle windows. This is an error, so abort any frame that it is progress
            
                abortFrame(ptr);          // invalidate any frame in progress. It might already be 0, which is ok. 
                
                SBI( ptr->errorBits , ERRORBIT_NOISE );
                
                ptr->tickCount=0;        // Reset back down. At least keeps us from overflowing.
                                
            }                
        }

    } else {    // pulses > 0 - so we got pulses in the last tick

        // ERROR CHECK: More than 3 pulses in a window is too many, and is an error

        if (ptr->pulse_count>3) {
            abortFrame(ptr);
            SBI( ptr->errorBits , ERRORBIT_NOISE );
        } else {

            // TODO: This code could be more direct?

            if (ptr->pulse_accumulator==0) {       // the previous tick was idle

                ptr->tickCount=0;                   // Reset counter

            } 
                    
            // If there have been more than 2 consecutive ticks with pulses (including this one), then this is an error
		
		    ptr->tickCount++;
                 
            // Max number of frames a symbol can take is ( IR_PULSE_TIME_US + IR_SPACE_TIME_US ) * 3. 
         
            // TODO: reduce this when we know max interrupt latency. WIll improve noise rejection. 
            
            if (ptr->tickCount > 5) {               // maximum number of windows with consecutive pulses. way too high now until we nail down max interrupt latency. 
                
                // There have been more than 2 consecutive ticks with pulses. This is an error, so abort any frame that it is progress
                
                abortFrame(ptr);         // invalidate any frame in progress. It might already be 0, which is ok.
                SBI( ptr->errorBits , ERRORBIT_DROPOUT );
                                
                ptr->tickCount=0;        // Reset back down. At least keeps us from overflowing.
                
            } else {
            
                ptr->pulse_accumulator += pulses;

                // Our longest symbol (for now) is a 1-bit, which is 3 pulses long.
                // Check accumulator cant be more than 3 or abort
                
                if (ptr->pulse_accumulator>3) {
                    abortFrame(ptr);
                    SBI( ptr->errorBits , ERRORBIT_NOISE );
                } 
                     
            }      
        }
        
        ptr->pulse_count= 0;        
                    
    }            
    
    */
	

           
}    

// Read the error state of the indicated LED
// Clears the bits on read

uint8_t ir_getErrorBits( uint8_t led ) {
    
    uint8_t bits;

    ir_rx_state_t *ptr = ir_rx_states + led;        
    
    // Must do the snap-and-clear atomically because otherwise i bit might get set by the ISR 
    // after we snapped, but before we cleared, and then then clear could clobber it. 
    
    DO_ATOMICALLY {
    
        bits = ptr->errorBits;        // snapshot current value    
        ptr->errorBits=0;                      // Clear out the bits we are grabbing
        
    }     
    
    return bits; 
}    

// Is there a received data ready to be read?

uint8_t irIsReady( uint8_t led ) {
    return( ir_rx_states[led].lastValue );    
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



static void txsyncpulse( uint8_t ledBitmask ) {
    ir_tx_pulse( ledBitmask );
    _delay_us(IR_SYNC_TIME_US);
}


static void txpulse( uint8_t ledBitmask) {
    ir_tx_pulse( ledBitmask );                          
    _delay_us(IR_SPACE_TIME_US);                        
}    


static void txbit( uint8_t ledBitmask, uint8_t bit ) {
    
    // 1 blinks for a 0-bit
    
    txpulse( ledBitmask );
    
    if (bit) {
        
       // 2 blinks for a 1-bit        
        txpulse( ledBitmask );
        
    }

    // Two spaces at the end of each bit

    _delay_us(IR_SPACE_TIME_US);
    _delay_us(IR_SPACE_TIME_US);

    
    
}

void irSendData( uint8_t face , uint8_t data ) {
    
    #warning sending on all faces
    uint8_t ledBitmask = ALL_IR_BITS; // _BV( face );       // Only send on one LED
    
    // TODO: Check for RX in progress here?

    // RESET link
    
    // Three consecutive pulses will reset RX state
    // We need to send 4 to ensure the RX sees at least 3
   
    txsyncpulse( ledBitmask );
    txsyncpulse( ledBitmask );
    txsyncpulse( ledBitmask );
    txsyncpulse( ledBitmask );
  
   // Two spaces at the end of sync to load at least 1 zero into the bitstream

   _delay_us(IR_SPACE_TIME_US);
   _delay_us(IR_SPACE_TIME_US);    
      
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