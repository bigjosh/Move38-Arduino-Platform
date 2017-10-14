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
#include "blinkcore.h"

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


#if ALL_IR_BITS != IR_BITS

    #error Code assumes ALL_IR_BITS  and IR_BITS are equivalant. If not, you need to map them manually. 

#endif

  
 
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



// This gets called anytime one of the IR LED cathodes has a level change drops. This typically happens because some light
// hit it and discharged the capacitance, so the pin goes from high to low. We initialize each pin at high, and we charge it
// back to high everything it drops low, so we should in practice only ever see high to low transitions here.

// TOOD: We will use this for waking from nap.

ISR(IR_ISR) {
    
    // EMPTY
    
}


// We use the general interrupt control register to gate interrupts on and off rather than the mask

void ir_enable(void) {
    
    // TODO: Call this interrupt enable()
    
    // This must come before the charge or we could miss a change that happened between the charge and the enable and that would
    // loose the LED out of the cycle forever
    
    #warning IR interrupts totally disabled for now. We will need them for wake

    //SBI( PCICR , IR_PCI );      // Enable the pin group to actual generate interrupts    
    
    // There is a race where an IR can get a pulse right here, but that is ok becuase it will just generate an int and be processed normally
    // and get recharged naturally before the next line.
    
    // Initial charge up of cathodes to get things going
    //chargeLEDs( IR_BITS );      // Charge all the LEDS - this handles suppressing extra pin change INTs during charging
    
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


// Send a pulse on all LEDs that have a 1 in bitmask
// bit 0= D1, bit 1= D2...
// This clobbers whatever charge was on the selected LEDs, so only call after you have checked it.

// Must be atomic so that...
// 1) the IR ISR doesnt show up and see our wierd registers, and 
// 2) The flashes don't get interrupted and streched out long enough to cause 2 triggers

// TODO: Queue TX so they only happen after a successful RX or idle time. Unnecessary since TX time so short?

// ASSUMES INTERRUPTS ON!!!! DONT CALL INTERNALLY!
// TODO: Make a public facing version that brackets with ATOMIC

// TODO: INcorporate this into the tick handle so we will be charging anyway. 

void ir_tx_pulse( uint8_t bitmask ) {
    
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        
        // TODO: Check for input before sending and abort if found...

        // Remember that the normal state for IR LED pins is...
        // ANODE always driven. Typically DDR driven and PORT low when we are waiting to RX pulses.
        // CATHODE is input, so DDR not driven and PORT low. 
    
                         
        uint8_t cathode_ddr_save = IR_CATHODE_DDR;          // We don't want to mess with the upper bits not used for IR LEDs
    
        PCMSK1 &= ~bitmask;                                 // stop Triggering interrupts on these cathode pins because they are going to change when we pulse
        
        // Now we don't have to worry about...
        // (1) a received pulse on this LED interfering with our transmit and 
        // (2) Our fiddling the LED bits causing an interrupt on this LED and fireing the receive code
    
        IR_CATHODE_DDR |= bitmask ;   // Drive Cathode too (now driving low)
    
        // Right now both cathode and anode are driven and both are low - so LED off
    
        
        // if we got interrupted here, then the pulse could get long enough to look like 2 pulses. 

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
}


// Measure the IR LEDs to to see if they have been triggered.
// Returns a 1 in each bit for each LED that was fired.
// Fired LEDs are recharged. 

uint8_t ir_test_and_charge( void ) {
   //DEBUGB_1();
   
   // ===Time critcal section start===
   
   // Find out which IR LED(s) went low to trigger this interrupt
   
   // TODO: Could save lots by redoing in ASM
   // TODO: Look into moving _zero_reg_ out of R! to save a push/pop and eor.
   
   uint8_t ir_LED_triggered_bits;

    DO_ATOMICALLY {   
        
       ir_LED_triggered_bits = (~IR_CATHODE_PIN) & IR_BITS;      // A 1 means that LED triggered

   
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
    }


    // only debug on IR0
    if ( ir_LED_triggered_bits & _BV(0) ) {        // IR1
        //DEBUGC_PULSE(1);
    }

    
    return ir_LED_triggered_bits;                      

}

