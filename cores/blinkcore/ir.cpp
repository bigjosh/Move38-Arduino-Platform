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


#include "hardware.h"

#include <avr/interrupt.h>
#include <util/delay.h>         // Must come after F_CPU definition

#include "debug.h"
#include "ir.h"
#include "utils.h"

#include "callbacks.h"

// A bit cycle is one timer tick, currently 512us

//TODO: Optimize these to be exact minimum for the distance in the real physical object    

#define IR_CHARGE_TIME_US 5        // How long to charge the LED though the pull-up

// Currently chosen empirically to work with some tile cases Jon made 7/28/17
 #define IR_PULSE_TIME_US 10    // Used for sending flashes
 
 // Must be long enough that receiver can detect distinct flashes even at maximum interrupt latency
 #define IR_SPACE_TIME_US 100   // Used for sending flashes
 
 // Must be long enough that a full window fits inside two timer ticks including the max clock differnece between sender and reciever. 
 #define IR_WINDOW_US 1100       // Used for sending idle windows. 
 
static inline void chargeLEDs( uint8_t bitmask ) {
    
     PCMSK1 &= ~bitmask;                                 // stop Triggering interrupts on these pins because they are going to change when we charge them
    
    // charge up receiver cathode pins while keeping other pins intact
           
    // This will enable the pull-ups on the LEDs we want to change without impacting other pins
    // The other pins will stay whatever they were.
    
    // NOTE: We are doing something tricky here. Writing a 1 to a PIN bit actually toggles the PORT bit. 
    // This saves about 10 instructions to manually load, or, and write back the bits to the PORT. 
    
    
    /*
        19.2.2. Toggling the Pin
        Writing a '1' to PINxn toggles the value of PORTxn, independent on the value of DDRxn. 
    */
    
    IR_CATHODE_PIN =  bitmask;
    
    // Empirically this is how long it takes to charge 
    // and avoid false positive triggers in 1ms window 12" from halogen desk lamp
    
    _delay_us( IR_CHARGE_TIME_US );
    
    
    // Only takes a tiny bit of time to charge up the cathode, even though the pull-up so no extra delay needed here...
    

    PCMSK1 |= bitmask;                  // Re-enable pin change on the pins we just charged up
                                        // Note that we must do this while we know the pins are still high
                                        // or there might be a *tiny* race condition if the pin changed in the cycle right after
                                        // we finished charging but before we enabled interrupts. This would latch
                                        // forever.
                                                                                    
    // Stop charging LED cathode pins (toggle the triggered bits back to what they were)
    
    IR_CATHODE_PIN = bitmask;     
                                                          
}    

// Send a pulse on all LEDs that have a 1 in bitmask
// bit 0= D1, bit 1= D2...
// This clobbers whatever charge was on the selected LEDs, so only call after you have checked it.

// TODO: Queue TX so they only happen after a successful RX or idle time. Unnecessary since TX time so short?


void ir_tx_pulse( uint8_t bitmask ) {
    
    // TODO: Check for input before sending and abort if found...

    // Remember that the normal state for IR LED pins is...
    // ANODE always driven. Typically DDR driven and PORT low when we are waiting to RX pulses.
    // CATHODE is input, so DDR not driven and PORT low. 
    
    uint8_t cathode_ddr_save = IR_CATHODE_DDR;
    
    PCMSK1 &= ~bitmask;                                 // stop Triggering interrupts on these cathode pins because they are going to change when we pulse
    
    IR_CATHODE_DDR |= bitmask ;   // Drive Cathode too (now driving low)
    
    // Right now both cathode and anode are driven and both are low - so LED off

    // Anode pins are driven output and low normally, so this will
    // make them be driven high output 
     
    IR_ANODE_PIN  = bitmask;    // Blink!       (Remember, a write to PIN actually toggles PORT)
    
    // Right now anode driven and high, so LED on!
      
    // TODO: Is this the right TX pulse with? Currently ~6us total width
    // Making too long wastes (a little?) battery and time
    // Making too short might not be enough light to trigger the RX on the other side
    // when TX voltage is low and RX voltage is high?
    // Also replace with a #define and _delay_us() so works when clock changes?

    _delay_us( IR_PULSE_TIME_US );
                
    IR_ANODE_PIN  = bitmask;    // Un-Blink! Sets anodes back to low (still output)      (Remember, a write to PIN actually toggles PORT)

    // Right now both cathode and anode are driven and both are low - so LED off

    IR_CATHODE_DDR = cathode_ddr_save;   // Cathode back to input too (now driving low) 
      
    // charge up receiver cathode pin while keeping other pins intact
           
    // This will enable the pull-ups on the LEDs we want to change without impacting other pins
    // The other pins will stay whatever they were.
    
    // NOTE: We are doing something tricky here. Writing a 1 to a PIN bit actually toggles the PORT bit. 
    // This saves about 10 instructions to manually load, or, and write back the bits to the PORT. 
        
    /*
    19.2.2. Toggling the Pin
    Writing a '1' to PINxn toggles the value of PORTxn, independent on the value of DDRxn. 
    */
    
    // TODO: Only recharge pins that we high when we started
    
    // TODO: These need to be asm because it sticks a load here.
               
    IR_CATHODE_PIN =  bitmask;
    
    _delay_us( IR_CHARGE_TIME_US ); 
        
    
    PCMSK1 |= bitmask;                  // Re-enable pin change on the pins we just charged up
                                        // Note that we must do this while we know the pins are still high
                                        // or there might be a *tiny* race condition if the pin changed in the cycle right after
                                        // we finished charging but before we enabled interrupts. This would latch until the next 
                                        // recharge timeout.
    
    
    // Stop charging LED cathode pins (toggle the triggered bits back o what they were)
    
    IR_CATHODE_PIN =  bitmask;                 
     
}

// from http://www.microchip.com/forums/m587239.aspx

static uint8_t oddParity(uint8_t p) {
      p = p ^ (p >> 4 | p << 4);
      p = p ^ (p >> 2);
      p = p ^ (p >> 1);
      return p & 1;
}


/*

// Superseded by idiomatic Arduino functions  

void ir_send( uint8_t face , uint8_t data ) {
    
    while ( ir_tx_data[face]);          // Wait until any currently in progress transmission is complete
    
    ir_tx_data[face] = to_tx_pattern( data & 0x03 ); 
    
}    

*/


// 2 pulse=0 bit
// 3 pulse=1 bit

// 1 pulse doesn't mean anything because a pulse can happen spontaneously just from he LEDs leakage. 

// TODO: More symbols to increase bandwidth

// A count of how many triggers in the last timer tick

 #define STATEBITS_READY        1    // Value in lastValue is valid
 #define STATEBITS_PARITY       2    // There was an RX parity error
 #define STATEBITS_OVERFLOW     3    // A received byte in lastValue was overwritten with a new value
 #define STATEBITS_NOISE        4    // We saw unexpected extra pulses inside data
 
 
 typedef struct {
     
     volatile uint8_t lastValue;      // Last successfully decoded RX value.
     volatile uint8_t stateBits;
     
     // These internal variables are only updated in ISR, so don't need to be volatile.
     uint8_t buffer;                  // Buffer for RX in progress.
     uint8_t nextbit;                 // Valid data bits in the buffer +1. 0= waiting for sync. Sets to 1 when a valid sync is detected. When we get to 8, we have a good byte if the parity checks. Reset to 0 anytime we see something invalid. 
     
     uint8_t pulse_count;             // Number of pulses received in most recent tick
     uint8_t pulse_accumulator;       // Accumulate the number of pulses in the tickCount previous ticks. 
     uint8_t tickCount;               // Number of ticks since current phase started (phase is either receiving a symbol or idle) 
     
     uint8_t validSymbol;             // Reset when we see an idle window. Set when we see more than 3 pulses in a window, more than 3 pulses in a symbol, more than 2 ticks in a symbol. 
          
 } ir_rx_state_t;

 // We keep these in together in a a struct to get faster access via
 // Data Indirect with Displacement opcodes

 ir_rx_state_t ir_rx_states[IRLED_COUNT];
 
 
// This gets called anytime one of the IR LED cathodes has a level change drops. This typically happens because some light 
// hit it and discharged the capacitance, so the pin goes from high to low. We initialize each pin at high, and we charge it
// back to high everything it drops low, so we should in practice only ever see high to low transitions here.

// For each LED that went low, we increment the pulse_count and then recharge the LED back to high to be ready for the next trigger.

ISR(IR_ISR)
{	

    //DEBUGA_1();                
    
    // ===Time critcal section start===
    
    // Find out which IR LED(s) went low to trigger this interrupt
    
    // TODO: Could save lots by redoing in ASM
    // TODO: Look into moving _zero_reg_ out of R! to save a push/pop and eor.
        
            
    uint8_t ir_LED_triggered_bits = (~IR_CATHODE_PIN) & IR_BITS;      // A 1 means that LED triggered


    // only debug on IR0    
    if ( ir_LED_triggered_bits & _BV(0) ) {        // IR1
        //DEBUGC_PULSE(1);
    }
            
    // If a pulse comes in after we sample but before we finish charging and enabling pin change, then we will miss it
    // so best to keep this short and straight
    
    // Note that protocol should make sure that real data pulses should have a header pulse that 
    // gets this receiver in sync so we only are recharging in the idle time after a pulse. 
    // real data pulses should come less than 1ms after the header pulse, and should always be less than 1ms apart. 
    // Recharge the ones that have fired

    chargeLEDs( ir_LED_triggered_bits ); 

    // TODO: Some LEDs seem to fire right after IR0 is charged when connected to programmer?

    // ===Time critcal section end===
            
    // This is more efficient than a loop on led_count since AVR only has one pointer register
    // and bit operations are fast 
    
    // TODO: Could be optimized more and pure asm
        
    uint8_t ledBitwalk = _BV( IRLED_COUNT-1 );           // We will walk down though the 6 leds
    
    ir_rx_state_t *rx_ptr = ir_rx_states+(IRLED_COUNT-1);     // This compiles nicely to Z with offset
        
    do {
           
        if ( ir_LED_triggered_bits & ledBitwalk )  {         // Did we just get a pulse on this LED?
            
            rx_ptr->pulse_count++;
            // ASSERT: There is not enough time in a tick to overflow this counter. TODO: Check that. 
            
        }  //  if ( ir_LED_triggered_bits & bitmask ) 

        rx_ptr--;
        ledBitwalk >>=1;
        
    } while (ledBitwalk);

   //DEBUGA_0();   
            
}

// Abort any frame that might be in progress. We call this when we see anything invalid. 

static void inline abortFrame(  ir_rx_state_t *ptr   ) {
    ptr->nextbit=0;                         // 0 here means no frame in progress. It gets set to 1 when we get a sync 

    
    if (ptr==ir_rx_states) DEBUGA_PULSE(20);
}    

// Start decoding the 8 data + 1 partiy bits  of a frame

static void inline startFrame(  ir_rx_state_t *ptr   ) {
	ptr->nextbit=1;                         // 0 here means no frame in progress. It gets set to 1 when we get a sync
}


// We just received a symbol on this LED as marked by a trailing idle window

static void inline decode_symbol(  ir_rx_state_t *ptr   , uint8_t pulses ) {


    // here accumulator can only be 1,2, or 3 (higher values are rejected below when the pulse are being accumulated)
           
    if (pulses==1) {       // Sync symbol, start reading data symbols into the buffer...
               
        startFrame(ptr);		// Ready to start receiving data bits		
               
    } else {        // if (accumulator ==2 || accumulator==3 )  - Got idle window, there was a valid data symbol
               
        // It is a data symbol

        //DEBUGB_BITS(ptr->nextbit);

        uint8_t bit = (pulses==3);    // 2=0-bit, 3=1-bit
        			               
        if (ptr->nextbit == 9 ) {     // Do we already have a full byte?

            // If so, then this is party
                   
            if ( oddParity( ptr-> buffer) == bit ) {     // Check parity
                       
                // Parity checks! We got a good full byte!
                       
                if ( TBI(  ptr->stateBits , STATEBITS_READY ) ) {  // Already a byte ready but not yet read?
                           
                    SBI( ptr->stateBits , STATEBITS_OVERFLOW );    // Signal the overflow (we overwrite existing on overflow)
                           
                }
                       
                ptr->lastValue = ptr->buffer;
                
                DEBUGB_BITS(ptr->buffer);                
                       
            } else {
                       
				// Flag parity error
				SBI( ptr->stateBits , STATEBITS_PARITY );
                       
			}
                   
			// Frame finished, start over looking for a new frame            
	        abortFrame(ptr);
                   
            // Note that buffer naturally shifts out old value, so we do not need to reset it. 

        } else {        // bitcount < 8
                   
                ptr->buffer <<=1;           
                ptr->buffer |=bit;
                ptr->nextbit++;

                //DEBUGB_BITS(ptr->buffer);
                   
        }
        
               
    }
          
}


// Called once per tick per IR LED
// Pull it out of the loop for clarity
 
static void inline led_tick(  ir_rx_state_t *ptr  ) {
    
    // Get the number of pulses that happened in the last tick
	
        
    uint8_t pulses= ptr->pulse_count;                // This compiles nicely to Z with offset		


		do {
			#warning debug code

			//** DEBUG

			if (ptr==ir_rx_states) {
		        uint8_t a= pulses;                // This compiles nicely to Z with offset
	 	 	 
		        while (a--) {
			        _delay_us(20);
			        DEBUGC_PULSE(20); 
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
               
            if ( ptr->tickCount > 2) {               // maximum number of consecutive idle windows in a frame is 1 - which is 3 ticks
                
                // There have been more than 2 consecutive idle windows. This is an error, so abort any frame that it is progress
            
                abortFrame(ptr);          // invalidate any frame in progress. It might already be 0, which is ok. 
                ptr->tickCount=0;        // Reset back down. At least keeps us from overflowing. 
                
            }                
        }

    } else {    // pulses > 0 - so we got pulses in the last tick

        // ERROR CHECK: More than 3 pulses in a window is too many, and is an error

        if (ptr->pulse_count>3) {
            abortFrame(ptr);
            /*   TODO: Set a noise bit? */
        } else {

            // TODO: This code could be more direct?

            if (ptr->pulse_accumulator==0) {       // the previous tick was idle

                ptr->tickCount=0;                   // Reset counter

            } 
                    
            // If there have been more than 2 consecutive ticks with pulses (including this one), then this is an error
		
		    ptr->tickCount++;
        
            // TODO: This should be 2
            if (ptr->tickCount > 3) {               // maximum number of windows with consecutive pulses in a frame is 1, which is 2 ticks (remember that a <tick is 1/2 a window)
                
                // There have been more than 2 consecutive ticks with pulses. This is an error, so abort any frame that it is progress
                
                abortFrame(ptr);         // invalidate any frame in progress. It might already be 0, which is ok.
                ptr->tickCount=0;        // Reset back down. At least keeps us from overflowing.
                
            } else {
            
                ptr->pulse_accumulator += pulses;

                // TODO: ERROR CHECK accumulator cant be more than 3 or abort
                /*
                if (ptr->pulse_accumulator>3) {
                    abortFrame(ptr);
                    //   TODO: Set a noise bit? 
                } 
                */     
            }      
        }
        
        ptr->pulse_count= 0;        
                    
    }            
	

           
}    

// Called form timer.cpp every tick.
// Here we process the counts from the previous tick. 

void ir_tick_isr_callback(){


	#warning debug
    DEBUGC_PULSE(1);
    
    uint8_t ledBitwalk = _BV( IRLED_COUNT-1 );           // We will walk down though the 6 leds
    
    ir_rx_state_t *ptr = ir_rx_states+(IRLED_COUNT-1);
    
    do {
        
        led_tick( ptr );

        ptr--;
        ledBitwalk >>=1;
            
    }  while (ledBitwalk);
    	
                                   
}    

// We use the general interrupt control register to gate interrupts on and off rather than the mask

void ir_enable(void) {
    
    // This must come before the charge or we could miss a change that happened between the charge and the enable and that would
    // loose the LED out of the cycle forever

    SBI( PCICR , IR_PCI );      // Enable the pin group to actual generate interrupts    
    
    // There is a race where an IR can get a pulse right here, but that is ok becuase it will just generate an int and be processed normally
    // and get recharged naturally before the next line.
    
    // Initial charge up of cathodes to get things going
    chargeLEDs( IR_BITS );      // Charge all the LEDS - this handles suppressing extra pin change INTs during charging
    
}

void ir_disable(void) {

    CBI( PCICR , IR_PCI );      // Disable the pin group to block interrupts
        
}        

void ir_init(void) {
    
    DEBUG_INIT();     // TODO: Get rid of all debug stuff
      
    
    IR_ANODE_DDR |= IR_BITS ;       // Set all ANODES to drive (and leave forever)
                                    // The PORT will be 0, so these will be driven low
                                    // until we actively send a pulse
                            
    // Leave cathodes DDR in input mode. When we write to PORT, then we will be enabling pull-up which is enough to charge the 
    // LEDs and saves having to switch DDR every charge. 

    
    // Pin change interrupt setup
    IR_MASK |= IR_PCINT;             // Enable pin in Pin Change Mask Register for all 6 cathode pins. Any change after this will set the pending interrupt flag.
                                     // TODO: Single LEDs can get masked here if they get noisy to avoid spurious wakes 
                                              
}
