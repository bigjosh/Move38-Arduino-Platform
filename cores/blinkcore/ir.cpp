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

#include "callbacks.h"

// A bit cycle is one timer tick, currently 512us

//TODO: Optimize these to be exact minimum for the distance in the real physical object
//TODO: This can likely be reduced or eliminated when we increase sampling rate

#define IR_CHARGE_TIME_US 2        // How long to charge the LED though the pull-up

// Currently chosen empirically to work with some tile cases Jon made 7/28/17

#define IR_PULSE_TIME_US 10         // Used for sending flashes


#if IR_ALL_BITS != IR_BITS

    #error Code assumes IR_ALL_BITS  and IR_BITS are equivalant. If not, you need to map them manually.

#endif

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

    // TODO: IR interrupts totally disabled for now. We will need them for wake on data.

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

        PCMSK1 &= ~bitmask;                                 // stop Triggering interrupts on these cathode pins because they are going to change when we pulse

        // TODO: Current blinklib does not use IR interrupts, so we could get rid of this but would only save a couple instructions/cycles

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

        // charge up receiver cathode pins while keeping other pins intact

        // TODO: Only recharge pins that we high when we started

        // TODO: These need to be asm because it sticks a load here.

        // Set cathode high for any bit masked pins that were high when we started

        IR_CATHODE_PIN =  savedCathodeBits;

        // Cathodes are now being driven high

        _delay_us( IR_CHARGE_TIME_US );

        PCMSK1 |= bitmask;                  // Re-enable pin change on the pins we just charged up
                                            // Note that we must do this while we know the pins are still high
                                            // or there might be a *tiny* race condition if the pin changed in the cycle right after
                                            // we finished charging but before we enabled interrupts. This would latch until the next
                                            // recharge timeout.



        IR_CATHODE_DDR = cathode_ddr_save;   // Cathode back to input too (pull-ups still on)

        IR_CATHODE_PIN =  savedCathodeBits;  // Disable pull-ups. Everything back to pure input.

}


static inline void chargeLEDs( uint8_t bitmask ) {

    uint8_t ir_cathode_ddr_cache = IR_CATHODE_DDR;       // This will not change unless we change it, so no need to reload it every access

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

    IR_CATHODE_PIN =  bitmask;       // This enables pull-ups on charge pins

    IR_CATHODE_DDR = ir_cathode_ddr_cache | bitmask;       // This will drive the charge pins high

    // Empirically this is how long it takes to charge
    // and avoid false positive triggers in 1ms window 12" from halogen desk lamp

    _delay_us( IR_CHARGE_TIME_US );


    // Only takes a tiny bit of time to charge up the cathode, even though the pull-up so no extra delay needed here...


    PCMSK1 |= bitmask;                  // Re-enable pin change on the pins we just charged up
                                        // Note that we must do this while we know the pins are still high
                                        // or there might be a *tiny* race condition if the pin changed in the cycle right after
                                        // we finished charging but before we enabled interrupts. This would latch
                                        // forever.

    // Stop charging LED cathode pins


    IR_CATHODE_DDR = ir_cathode_ddr_cache;          // Back to the way we started, charge pins now only pulled-up

    IR_CATHODE_PIN = bitmask;                       // toggle pull-ups off, now cathodes pure inputs

}

// Measure the IR LEDs to to see if they have been triggered.
// Must be called when interrupts are off.
// Returns a 1 in each bit for each LED that was fired.
// Fired LEDs are recharged.

uint8_t ir_test_and_charge_cli( void ) {

   // ===Time critcal section start===

   // Find out which IR LED(s) went low to trigger this interrupt

   // TODO: Could save lots by redoing in ASM
   // TODO: Look into moving _zero_reg_ out of R! to save a push/pop and eor.

   uint8_t ir_LED_triggered_bits;

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

    return ir_LED_triggered_bits;

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

static volatile uint16_t sendpulse_bitmask;      // Which IR LEDs to send on
static volatile uint16_t sendpulse_spaces;       // Time to delay until next pulse. 0=pulse sent
static volatile uint16_t sendpulse_spaces_next;  // A one entry deep buffer for spaces so we can try to keep it primed.

// Currently clocks at 23us @ 4Mhz

ISR(TIMER1_CAPT_vect) {

    if (sendpulse_spaces) {

        sendpulse_spaces--;

        if (sendpulse_spaces==0) {

            ir_tx_pulse_internal( sendpulse_bitmask );     // Flash

            sendpulse_spaces = sendpulse_spaces_next;

            sendpulse_spaces_next = 0;

        }
    }

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
// Each pulse will have an integer number of delays between it and the previous pulse.
// Each of those delay windows is spacing_ticks wide.
// This sets things up and sends the initial pulse.
// Then continue to call ir_tx_sendpulse() to send subsequent pulses
// Call ir_tx_end() after last pulse to turn off the ISR (optional but saves CPU and power)

void ir_tx_start(uint16_t spacing_ticks , uint8_t bitmask , uint16_t initialSpaces ) {

    sendpulse_bitmask = bitmask &  IR_ALL_BITS; // Protect the non-IR LED bits from invalid input

    ICR1 = spacing_ticks;               // We will fire ISR when we hit this, and also roll back to 0.

    TCNT1 = spacing_ticks-1;            // Grease the wheels. This will make the 1st pulse go out as soon as we turn on the timer

    SBI( TIFR1 , ICF1 );                // Clear TOP match flag in case it is set.
                                        // "ICF can be cleared by writing a logic one to its bit location."


    TIMSK1 |= _BV(ICIE1);                // Enable the ISR when we hit ICR1
                                         // TODO: Do this only once at startup

    sendpulse_spaces_next=initialSpaces; // Get our first space into the buffer so it is ready before we start
    sendpulse_spaces=1;                  // Will send as soon as called (immediately after next line).

    // Start clock in mode 12 with /1 prescaller (one timer tick per clock tick)
    TCCR1B = _BV( WGM12) | _BV( WGM13) | _BV( CS10);            // clk/1

    // ISR will trigger immediately and send the 1st pulse

}

// leadingSpaces is the number of spaces to wait between the previous pulse and this pulse.
// 0 doesn't really make any sense

// TODO: single buffer this in case sender has a hiccup or is too slow to keep up?

void ir_tx_sendpulse( uint8_t leadingSpaces ) {

    while (sendpulse_spaces_next);           // Wait for previous entry to buffer to pulse to get sent
    sendpulse_spaces_next=leadingSpaces;     // ISR trigger will end a pulse after specified number of spaces

}

// Turn off the pulse sending ISR
// TODO: This should return any bit that had to be terminated because of collision

void ir_tx_end(void) {

    while (sendpulse_spaces);       // Wait for all previous pulses to get sent (buffered and immedeate complete)

    // stop timer (not more ISR)
    TCCR1B = 0;             // Sets prescaler to 0 which stops the timer.

}

