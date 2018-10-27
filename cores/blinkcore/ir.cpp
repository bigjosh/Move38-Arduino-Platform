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
#include "shared.h"		// Get F_CPU and FACE_COUNT
#include "bitfun.h"

#include <avr/interrupt.h>
#include <util/delay.h>         // Must come after F_CPU definition

#include "ir.h"
#include "utils.h"


//TODO: Optimize these to be exact minimum for the distance in the real physical object
//TODO: This can likely be reduced or eliminated when we increase sampling rate


// You want the pulse time to be long enough to always trigger the RX LED. Longer pulses can get more energy across the link.
// You need the pulse time to be short enough that a single pulse does not trigger two adjacent RX windows. One pulse should trigger one window.

// You want the charge time to be long enough to fully charge the capacitance of the LED. It is possible that using shorter times could make the RX more sensitive
// because there would be less charge to bleed off, but I have not played with that much.
// Having the charge time too long is not a big concern since one it is fully charged that's it. Maybe the only concern is that it can not receive
// a new pulse while charging, so you might mask an RX if the total of the charge time and everything else is longer than the time between pulses (including clock skew).

#define IR_CHARGE_TIME_US 10         // How long to charge the LED though the pull-up

#define IR_PULSE_TIME_US 10         // How long to turn IR LED on to send a pulse


// Currently chosen empirically to work with some tile cases Jon made 7/28/17


#if IR_ALL_BITS != IR_BITS

    #error Code assumes IR_ALL_BITS  and IR_BITS are equivalant. If not, you need to map them manually.

#endif

// Debug flags can be uncommented in ir.h

#if defined ( TX_DEBUG )  || defined ( RX_DEBUG ) || defined (IR_DEBUG)

    #include "sp.h"

#endif


// This gets called anytime one of the IR LED cathodes has a level change drops. This typically happens because some light
// hit it and discharged the capacitance, so the pin goes from high to low. We initialize each pin at high, and we charge it
// back to high everything it drops low, so we should in practice only ever see high to low transitions here.

// Normally we leave this ISR off so only used for debugging. The timer ISR actually checks the IR LEDs at regular intervals. 
// This is better than interrupting on change since bright light could cause lots of ISRs and lock us out. 

#ifdef RX_DEBUG

    ISR(IR_ISR,ISR_NAKED) {

        // We make this NAKED so we can deal with it efficiently
        // otherwise the compiler adds an R0 and R1 preamble and postamble even though these are not used at all. Arg.
        // It compiles down to a single SBS with no side effects, so no need to save any registers.

        // Be careful if you put anything here that changes flags or registers!

           if ( ! TBI(PINC,0) ) {              // We test here so we don't go high on spurious INTs
               SP_PIN_A_SET_1();               // SP pin A goes high any time IR0 is triggered. We clear it when we later process in the polling code.
           }

        asm("RETI");

    }

#endif


// We use the general interrupt control register to gate interrupts on and off rather than the mask

void ir_enable(void) {

    // TODO: Call this interrupt enable()

    // This must come before the charge or we could miss a change that happened between the charge and the enable and that would
    // loose the LED out of the cycle forever

    // TODO: IR interrupts totally disabled for now. We will need them for wake on data.


    // There is a race where an IR can get a pulse right here, but that is ok becuase it will just generate an int and be processed normally
    // and get recharged naturally before the next line.

    // Initial charge up of cathodes to get things going
    //chargeLEDs( IR_BITS );      // Charge all the LEDS - this handles suppressing extra pin change INTs during charging


}

void ir_disable(void) {

    //CBI( PCICR , IR_PCI_BIT );      // Disable the pin group to block interrupts. Leave the bits for individual pins intact.

}

void ir_init(void) {


    IR_ANODE_DDR |= IR_BITS ;       // Set all ANODES to drive (and leave forever)
                                    // The PORT will be 0, so these will be driven low
                                    // until we actively send a pulse

    // Leave cathodes DDR in input mode. When we write to PORT, then we will be enabling pull-up which is enough to charge the
    // LEDs and saves having to switch DDR every charge.


    IR_CATHODE_DDR |= ~IR_BITS;     // Drive the unused (and really not connected) upper PC6 and PC7 pins low so we can
                                    // avoid having to mask these bits out and treat the cathode as a full byte

    // Pin change interrupt setup

    #ifdef RX_DEBUG


        // These lines enable the IRS that will make SP A pin go high when IR0 is triggered
        //IR_INT_MASK_REG |= _BV(0);      // Enable pin change on IR0 cathode
        //SBI( PCICR , IR_PCI_BIT );      // Enable the pin group to actually generate interrupts


        sp_serial_init();

        SP_PIN_R_MODE_OUT();
        sp_serial_tx('I');

        sp_serial_disable_rx();             // Free up the R pin for signaling

        SP_PIN_A_MODE_OUT();
        SP_PIN_R_MODE_OUT();

        SP_PIN_R_SET_0();

    #endif

    #ifdef TX_DEBUG
        SP_PIN_A_MODE_OUT();
    #endif

}


// Send a pulse on all LEDs that have a 1 in bitmask
// bit 0= D1, bit 1= D2...
// This clobbers whatever charge was on the selected LEDs, so only call after you have checked it.

// Must be atomic so that...
// 1) the IR ISR doesn't show up and see our weird registers, and
// 2) The flashes don't get interrupted and stretched out long enough to cause 2 triggers

// TODO: Queue TX so they only happen after a successful RX or idle time. Unnecessary since TX time so short?

// ASSUMES INTERRUPTS OFF!!!!
// TODO: Make a public facing version that brackets with ATOMIC

// TODO: Incorporate this into the tick handler so we will be charging anyway.

static inline void ir_tx_pulse_internal( uint8_t bitmask ) {


        // TODO: Check for input before sending and abort if found...
        // TODO: Maybe as easy as saving the cathode values and then only charging again if they were high?

        // Remember that the normal state for IR LED pins is...
        // ANODE always driven. PORT low when we are waiting to RX pulses or charging. PORT driven high when transmitting a pulse.
        // CATHODE is input when waiting for RX pulses, so DDR not driven and PORT low. CATHODE is driven high when charging and driven low when sending a pulse.

        // See ir.MD in this repo for more explanations

        uint8_t cathode_ddr_save = IR_CATHODE_DDR;          // We don't want to mess with the upper bits not used for IR LEDs

        //PCMSK1 &= ~bitmask;                                 // stop Triggering interrupts on these cathode pins because they are going to change when we pulse

        // Now we don't have to worry about...
        // (1) a received pulse on this LED interfering with our transmit and
        // (2) Our fiddling the LED bits causing an interrupt on this LED

        uint8_t savedCathodeBits = IR_CATHODE_PIN & bitmask;         // Read the current IR bit states so we can put them back the way they were and preserve any pending received bits

        IR_CATHODE_DDR |= bitmask ;   // Drive Cathode too (now driving low)

        // Right now both cathode and anode are driven and both are low - so LED off

        // if we got interrupted here, then the pulse could get long enough to look like 2 pulses.

        // Anode pins are driven output and low normally, so this will
        // make them be driven high output

        // NOTE: We are doing something tricky here. Writing a 1 to a PIN bit actually toggles the PORT bit.
        // This saves about 10 instructions to manually load, or, and write back the bits to the PORT.

        /*
        19.2.2. Toggling the Pin
        Writing a '1' to PINxn toggles the value of PORTxn, independent on the value of DDRxn.
        */

        #ifdef TX_DEBUG
            if (bitmask&0x01) {
                SP_PIN_A_SET_1();               // SP pin A goes when we are transmitting on IR0. Note that you can also tap the IR anode directly.
            }
        #endif

        IR_ANODE_PIN  = bitmask;    // Blink!       (Remember, a write to PIN actually toggles PORT)

        // Right now anode driven and high, so LED on!

        // TODO: Is this the right TX pulse with? Currently ~6us total width
        // Making too long wastes (a little?) battery and time
        // Making too short might not be enough light to trigger the RX on the other side
        // when TX voltage is low and RX voltage is high?
        // Also replace with a #define and _delay_us() so works when clock changes?

        _delay_us( IR_PULSE_TIME_US );

        IR_ANODE_PIN  = bitmask;    // Un-Blink! Sets anodes back to low (still output)      (Remember, a write to PIN actually toggles PORT)

        #ifdef TX_DEBUG
            SP_PIN_A_SET_0();               // Turn off SP A pin
        #endif

        // Right now both cathode and anode are driven and both are low - so LED off

        // charge up receiver cathode pins while keeping other pins intact

        // TODO: Only recharge pins that we high when we started

        // TODO: These need to be asm because it sticks a load here.

        // Set cathode high for any bit masked pins that were high when we started

        IR_CATHODE_PIN =  savedCathodeBits;

        // Cathodes are now being driven high

        _delay_us( IR_CHARGE_TIME_US );

        //PCMSK1 |= bitmask;                // Re-enable pin change on the pins we just charged up
                                            // Note that we must do this while we know the pins are still high
                                            // or there might be a *tiny* race condition if the pin changed in the cycle right after
                                            // we finished charging but before we enabled interrupts. This would latch until the next
                                            // recharge timeout.



        IR_CATHODE_DDR = cathode_ddr_save;   // Cathode back to input too (pull-ups still on)

        IR_CATHODE_PIN =  savedCathodeBits;  // Disable pull-ups. Everything back to pure input.

}


// Measure the IR LEDs to to see if they have been triggered.
// Must be called when interrupts are off.
// Returns a 1 in each bit for each LED that was fired.
// We break this out into a routing that runs with interrupts off so we can
// get consistent timing and not have the window size change frome to frame.

// Charge the bits set to 1 in 'chargeBits'
// Probably best to call with ints off so doesnt get interrupted
// Probably best to call some time after ir_sample_bits() so that a long pulse will not be seen twice.

uint8_t ir_sample_and_charge_LEDs() {

   // ===Time critcal section start===

   // Grab the IR LEDs that have triggered since the last time we checked.
   // These will be 0 on the cathode since it got discharged by the flash

   uint8_t ir_LED_triggered_bits;

   #ifdef RX_DEBUG
       SP_PIN_R_SET_1();               // SP pin R goes high during sample window
   #endif

   ir_LED_triggered_bits = IR_CATHODE_PIN;      // A 0 means that the IR LED drained, probably becuase of a flash

   // Note that we are capturing 8 bits here even though there are only 6 IR LEDs connected.
   // We purposely put the cathodes on PORTC since it really only has 6 working pins (PC6 is only connected when RESET is disabled, and PC7 does not have a pin).
   // This saves some ANDing to mask out upper bits.

    // If a pulse comes in after we sample but before we finish charging, then we will miss it
    // so best to keep this section short and straight.

    // Note that protocol should make sure that real data pulses should have a header pulse that
    // gets this receiver in sync so we only are recharging in the idle time after a pulse.
    // real data pulses should come less than 1ms after the header pulse, and should always be less than 1ms apart.


    // Recharge the ones that have fired

    // charge up receiver cathode pins while keeping other pins intact

    // This will enable the pull-ups on the LEDs we want to change without impacting other pins
    // The other pins will stay whatever they were.

    // NOTE: We are doing something tricky here. Writing a 1 to a PIN bit actually toggles the PORT bit.
    // This saves about 10 instructions to manually load, or, and write back the bits to the PORT.


    /*
        19.2.2. Toggling the Pin
        Writing a '1' to PINxn toggles the value of PORTxn, independent on the value of DDRxn.
    */
    
    uint8_t chargeBits = ~ir_LED_triggered_bits;

    IR_CATHODE_PIN =  chargeBits;       // This enables pull-ups on charge pins. If we set the DDR first, then we would drive the low pins to ground.
                                        // REMEBER: Writing a 1 to a PIN register actually toggles the PORT bit!

    IR_CATHODE_DDR =  chargeBits;       // This will drive the charge pins high

    // Empirically this is how long it takes to charge
    // and avoid false positive triggers in 1ms window 12" from halogen desk lamp

    _delay_us( IR_CHARGE_TIME_US );     // We charge for as long as the pulse time
                                        // It actually does not take this long to charge up the LED
                                        // (it has very low capacitance)
                                        // This is to avoid seeing the same pulse twice because it started sending
                                        // right before we sampled and finished right after in the next window.



    // Stop charging LED cathode pins


    IR_CATHODE_DDR = 0;                 // Back to the way we started, charge pins now input, but still pulled-up

    #ifdef RX_DEBUG
        SP_PIN_R_SET_0();               // We set the A output when we see a pin change interrupt on a cathode, so we clear it here to be ready to show the next one.
    #endif

    IR_CATHODE_PIN = chargeBits;        // toggle pull-ups off, now cathodes pure inputs
                                        // REMEBER: Writing a 1 to a PIN register actually toggles the PORT bit!

    // TODO: Some LEDs seem to fire right after IR0 is charged when connected to programmer?

   return chargeBits;               // Flip so a 1 in ir_sample_bits() means that LED triggered


}


/*

    We need a way to send pulses with precise spacing between them, otherwise
    an interrupt could happen between pulses and spread them out enough
    that they are no longer recognized.

    We will use Timer1 dedicated to this task for now. We will run in CTC mode
    and send our pulses on each TOP. This way we will never have to modify the
    counter while the timer is running- possibly loosing counts.

    We will use ICR1 to define the top.

    Mode 12: CTC TOP=ICR1 WGM13, WGM12

*/

static volatile uint8_t sendpulse_bitmask;      // Which IR LEDs to send on

// Currently clocks at 23us @ 4Mhz

ISR(TIMER1_COMPA_vect) {

    #ifdef TX_DEBUG
        if (sendpulse_bitmask & _BV(IR_RX_DEBUG_LED) ) SP_PIN_A_SET_1(); 
    #endif

    ir_tx_pulse_internal( sendpulse_bitmask );     // Flash
           
    #ifdef TX_DEBUG
        SP_PIN_A_SET_0();
    #endif

}

// Send a series of pulses with spacing_ticks clock ticks between each pulse (or as quickly as possible if spacing too short)
// If count=0 then 256 pulses will be sent.
// If spacing_ticks==0, then the time between pulses will be 65536 ticks

// Assumes timer1 stopped on entry, leaves timer1 stopped on exit
// Assumes timer1 overflow flag cleared on entry, leaves clear on exit

// TODO: Add a version that doesn't block and lets you specify a margin at the end before the next TX
// can start? Use overflow bit to see if past. Easy?

// TODO: Use fast PWM mode so OCR1 is buffered. We can then load the margin value
// on the last pass and it will get automatically swapped in after the final pulse.
// We can then quickly test the compare bit to see if the margin is over.

// TODO:  Check cathode and buffer first and don't send if in progress. Return aborted bits
/*

 uint8_t ir_LED_triggered_bits;

 ir_LED_triggered_bits = (~IR_CATHODE_PIN) & IR_BITS;

  */

// Sends starting a pulse train.
// Will send first pulse and then wait initialTicks before sending second pulse.
// Then continue to call ir_tx_sendpulse() to send subsequent pulses
// Call ir_tx_end() after last pulse to turn off the ISR (optional but saves CPU and power)

void ir_tx_start(uint8_t bitmask , uint16_t initialTicks ) {
      
    TCCR1B = _BV( WGM13) | _BV( WGM12);      // clk/0. Timer is now stopped.
    TCCR1A = 0;                               // Mode 12. THis is a CTC mode, we pick it just so we can directly access OCRA without the buffer.

    sendpulse_bitmask = bitmask &  IR_ALL_BITS; // Protect the non-IR LED bits from invalid input

    TCNT1 = 0;
    OCR1A = 1;                            // Prime the gears, send the first pulse right after we start the timer up.
    TCCR1A = _BV( WGM11 ) | _BV( WGM10);     // Mode 15. Count up to TOP=OCRA. Load new OCRA at BOTTOM.

    // Now that we are in a buffered mode, we can go ahead and update OCR1A to initial delay value

    OCR1A = initialTicks;               // We will fire ISR when we hit this, and also roll back to 0.

    SBI( TIFR1 , OCF1A );               // Clear match flag in case it is set.
                                        // After this, it will automatically be cleared by calling the ISR
                                        // TODO: We probably never need to do this manually


    SBI( TIFR1 , TOV1 );                // Clear Top OVerflow flag in case it is set.
                                        // We will use this to see when the timer fired and the last pulse is away
                                        // so we know when to buffer up the next delay value into OCRA 
                                        // We will manually clear this each time we buffer a new value


    TIMSK1 |= _BV(OCIE1A);                // Enable the ISR when we hit OCR1 as TOP
                                          // TODO: Do this only once at startup

    TCCR1B = _BV( WGM13) | _BV( WGM12) |_BV( CS10);   // clk/1. Starts timer.
        
    // ISR will trigger immediately and send the 1st pulse
    
}

// leadingSpaces is the number of spaces to wait between the previous pulse and this pulse.
// 0 doesn't really make any sense

// TODO: single buffer this in case sender has a hiccup or is too slow to keep up?

#warning
#include"debug.h"

void ir_tx_sendpulse( uint16_t delay_cycles) {    
    while ( !TBI( TIFR1 , TOV1) );            // Wait for current cycle to finish
    OCR1A = delay_cycles;                     // Buffer the next delay value
    SBI( TIFR1 , TOV1 );                      // Writing a 1 to the bit clears it       
}

// Turn off the pulse sending ISR
// TODO: This should return any bit that had to be terminated because of collision

void ir_tx_end(void) {

    while ( !TBI( TIFR1 , TOV1) );             // Wait for current cycle to finish
    SBI( TIFR1 , TOV1 );                      // Writing a 1 to the bit clears it
    while ( !TBI( TIFR1 , TOV1) );             // Wait final cycle to finish
    
    // stop timer (not more ISR)
    TCCR1B = 0;             // Sets prescaler to 0 which stops the timer.

}

