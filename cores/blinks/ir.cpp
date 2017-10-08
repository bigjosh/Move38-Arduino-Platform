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

#include "ir_callback.h"        // Connects the IR periodic ticks callback to the timer ISR.

// A bit cycle is one timer tick, currently 512us

//TODO: Optimize these to be exact minimum for the distance in the real physical object    

#define IR_CHARGE_TIME_US 5        // How long to charge the LED though the pull-up

// Currently chosen empirically to work with some tile cases Jon made 7/28/17
 #define IR_PULSE_TIME_US 10

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

// For testing. 

// from http://www.microchip.com/forums/m587239.aspx

uint8_t oddParity(uint8_t p) {
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
     uint8_t bitcount;                // Valid data bits in the buffer +1. Sets to 1 when a valid sync is detected. When we get to 8, we have a good byte if the parity checks.
     
     uint8_t pulse_count;             // Number of pulses received in most recent tick
     uint8_t pulse_accumulator;       // Accumulate prior pulses here (at most contains 2 ticks for now)     
     uint8_t pulse_ticks;             // Number of ticks included in the accumulator (now max 2)
          
 } ir_rx_state_t;

 // We keep these in together in a a struct to get faster access via
 // Data Indirect with Displacement opcodes

 ir_rx_state_t ir_rx_states[IRLED_COUNT];

// This gets called anytime one of the IR LED cathodes has a level change drops. This typically happens because some light 
// hit it and discharged the capacitance, so the pin goes from high to low. We initialize each pin at high, and we charge it
// back to high everything it drops low, so we should in practice only ever see high to low transitions here.

// Note that there is also a background task that periodically recharges all the LEDs frequently enough that 
// that they should only ever go low from a very bright source - like an IR led pointing right down their barrel. 

ISR(IR_ISR)
{	

    //DEBUGA_1();                
    
    // ===Time critcal section start===
    
    // Find out which IR LED(s) went low to trigger this interrupt
    
    // TODO: Could save lots by redoing in ASM
    // TODO: Look into moving _zero_reg_ out of R! to save a push/pop and eor.
        
    asm("nop");
            
    uint8_t ir_LED_triggered_bits = (~IR_CATHODE_PIN) & IR_BITS;      // A 1 means that LED triggered


    // only debug on IR0    
    if ( ir_LED_triggered_bits & _BV(0) ) {        // IR1
        //DEBUGB_PULSE(1);
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
            
        }  //  if ( ir_LED_triggered_bits & bitmask ) 

        rx_ptr--;
        ledBitwalk >>=1;
        
    } while (ledBitwalk);

   //DEBUGA_0();   
            
}

 

// Called form timer.cpp every tick.
// Here we process the counts from the previous tick. 


void ir_tick_isr_callback(){

    //DEBUGA_PULSE(1);
    
    uint8_t ledBitwalk = _BV( IRLED_COUNT-1 );           // We will walk down though the 6 leds
    
    ir_rx_state_t *rx_ptr = ir_rx_states+(IRLED_COUNT-1); 
    
    do {
        
        uint8_t pulses= rx_ptr->pulse_count;                // This compiles nicely to Z with offset

        if (!pulses) {                                     // No pulses in this tick, at least means any pending symbol is complete
            
            
            if (ledBitwalk==_BV(0)) {    // Only debug on led #1
                
                if (rx_ptr->pulse_accumulator==2) {         // 0 symbol found
                   // DEBUGA_0();
                   // _delay_us(10);
                    DEBUGA_PULSE(50);
                } else if (rx_ptr->pulse_accumulator==3) {         // 0 symbol found
                // DEBUGA_0();
                // _delay_us(10);
                    DEBUGB_PULSE(50);
                }
                
            }                     
            
            rx_ptr->pulse_accumulator=0;
            rx_ptr->pulse_ticks=0;
            
        } else {            
                                    
            // We do have accumulated pulses in the last tick
                        
            // TODO: Bound these to not overflow, or does time do that for us implicitly?
            
            rx_ptr->pulse_accumulator+=pulses;
            rx_ptr->pulse_ticks++;
            
            rx_ptr->pulse_count = 0;        // zero out the counter that gets incremented every tick so we can see when there is a tick with zero pulses
            
        } 
        
/*        
        else {        // pulses==0, so this might be the sience after a symbol..
            
            if (rx_ptr->pulse_ticks) {          // Any pulses received that could be symbols? (ignores if no pulses happened since last time)
                
                uint8_t accumulator = rx_ptr->pulse_accumulator;
                
                if ( accumulator!= 2 && accumulator != 3) {      // Only valid symbols now are 2 or 3 pulses
                    
                    // Not a valid number of pulses, but be noise
                    
                    rx_ptr->bitcount=0;     // Start over again (we need now to see a full good byte before we sync again)
                    SBI( rx_ptr->stateBits , STATEBITS_NOISE );
                    
                    
                }  else {
                    
                    // Ok, got a valid symbol - decode it!
                    
                    uint8_t bit = ( accumulator == 3 );         // 2=0, 3=1
                    
                    uint8_t bitcount=rx_ptr->bitcount;
                    
                    if (bitcount==0) {          // Nothing in the buffer yet
                        
                        if (bit) {          // Did we get a start bit?
                            
                            // Start receiving next 8 bits of data + parity bit!
                            bitcount=1;
                            rx_ptr->buffer=0;
                            
                        }                            
                        
                        // Note that if we got a 0 here, then we are idle so just wait for the start bit
                        // This makes it possible to sync by sending 9 0's.                         
                        
                    } else if (bitcount==8) {          
                        // We have received a full byte, so this bit is the parity. Check it!
                        
                        if ( bit == oddParity( rx_ptr->buffer ) ) { 
                            
                            // Parity checks! We got a good full byte!
                            
                            if ( TBI(  rx_ptr->stateBits , STATEBITS_READY ) ) {  // Already a byte ready but not yet read?
                                
                                SBI( rx_ptr->stateBits , STATEBITS_OVERFLOW );    
                                
                            } 
                            
                            rx_ptr->lastValue = rx_ptr->buffer;
                            
                        } else {
                            
                            // Flag parity error
                            SBI( rx_ptr->stateBits , STATEBITS_PARITY );                     
                            
                        }
                        
                        // Start over looking for a new byte
                        bitcount=0;                            
                        
                    } else {        // bitcount < 8
                        
                        rx_ptr->buffer <<=1;
                        rx_ptr->buffer |=bit;
                        rx_ptr->bitcount++;
                        
                    }                                               
                    
                }                    
                
                // Start looking for a good symbol
                rx_ptr->pulse_accumulator=0;
                rx_ptr->pulse_ticks=0;
                
            }                
                
        }
*/


        rx_ptr--;
        ledBitwalk >>=1;

            
    }  while (ledBitwalk);
    
    //DEBUGA_0();
                                   
}    

/*

// Superseded by idiomatic Arduino functions

uint8_t irled_rx_overflowBits(void) {
	
	return irled_rx_overflow;
	
}

*/


/*

// Superseded by idiomatic Arduino functions

// Returns last received data for requested face 
// bit 2 is 1 if data found, 0 if not
// if bit 2 set, then bit 1 & 0 are the data

uint8_t ir_read( uint8_t led) {

    uint8_t data = irled_RX_value[ led ];

    // Look for the starting pattern of 01 at the beginning of the data

    if ( ( data  & 0b00001100 ) == 0b00000100 ) {

        cli();                          // Must be atomic
        irled_RX_value[ led ] = 0;      // Clear out for next byte
        irled_rx_overflow &= ~_BV(led); // Clear out overflow flag
        sei(); 
                
        return( data );
        
    } else {
        
        return(0);
    }        
    
}    

*/

// Returns true if data is available to be read on the requested face
// Always returns immediately
// Cleared by subseqent irReadDibit()
// `led` must be less than FACE_COUNT
/*
uint8_t irIsAvailable( uint8_t face ) {
    
    uint8_t data = irled_RX_value[ face ];

    // Look for the starting pattern of 01 at the beginning of the data

    if ( ( data  & 0b00001100 ) == 0b00000100 ) {
        
        return( 1 );
        
    } else {
        
        return (0 );
        
    }                
                  
}    

// Returns last received dibit (value 0-3) for requested face
// Blocks if no data ready
// `led` must be less than FACE_COUNT

uint8_t irReadDibit( uint8_t face) {
        
    uint8_t data = irled_RX_value[ face ];

    // Block until we see the starting pattern of 01 at the beginning of the data

    while ( ( data  & 0b00001100 ) != 0b00000100 );

    cli();                          // Must be atomic
    irled_RX_value[ face ] = 0;      // Clear out for next byte
    irled_rx_overflow &= ~_BV(face); // Clear out overflow flag
    sei();
        
    return( data  & 0b00000011 );
           
}    


// Returns true if data was lost because new data arrived before old data was read
// Next read will return the older data (new data does not over write old)
// Always returns immediately
// Cleared by subseqent irReadDibit()
// `led` must be less than FACE_COUNT

uint8_t irOverFlowFlag( uint8_t face ) {
    
    return ( _BV( face ) & irled_rx_overflow );    
}    


// Blocks if there is already a transmission in progress on this face
// Returns immediately if no transmit already in progress
// `led` must be less than FACE_COUNT

void irFlush( uint8_t face ) {

    // while ( ir_tx_data[face]);          // Wait until any currently in progress transmission is complete
    
}    

// Transmits the lower 2 bits (dibit) of data on requested face
// Blocks if there is already a transmission in progress on this face
// Returns immediately and continues transmission in background if no transmit already in progress
// `led` must be less than FACE_COUNT

void irSendDibit( uint8_t face , uint8_t data ) {
    
}    

*/
// IR comms uses the 16 bit timer1 
// We only want 8 bits so will set the TOP at 256.
// 
// We run this timer at /8 prescaller so at 4Mhz will hit (our simulated) TOP of 256 every ~0.5ms
// TODO: Coordinate with PIXEL timers so they do not step on each other. 


// Transmits the lower 2 bits (dibit) of data on all faces
// Blocks if there is already a transmission in progress on any face
// Returns immediately and continues transmission in background if no transmits are already in progress

void irSendAllDibit(  uint8_t data ) {
        
    for( uint8_t face =0; face< FACE_COUNT; face++) {
        
        irSendDibit( face, data );
        
    }        
    
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
