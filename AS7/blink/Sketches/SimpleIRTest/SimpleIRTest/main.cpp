/*
 * SimpleIRTest.cpp
 *
 * This is the simplest possible IR test program.
 * Is accesses the blink hardware directly with no interrupts and requires no library code (runs bare metal).
 *
 * It has three modes: TX, RX, and sleep
 *
 * Assumes factory default fuses (runs at 1Mhz).
 *
 * TX mode
 * =======
 * Blink sends an IR pulse every 750us on the active face
 *
 * The 'A' pin on the service port connector goes high while the IR LED is on.
 *
 * The RGB LED on the active face shows green to indicate TX mode
 *
 * Long button press (>0.5s) cycles though pulse transmitted pulse width
 * The pulse width is indicated by the number of BLUE LEDS lit:
 *
 *    1 blue = 5us
 *    2 blue = 7us
 *    3 blue = 10us
 *    4 blue = 12us
 *    5 blue = 15us
 *
 * Short button press (<0.5s) the button advances to the next face
 * Short button press after the last face switches to RX mode
 *
 * RX mode
 * =======
 * Blink listens for IR triggers on the active face
 *
 * The 'A' pin on the service port connector pulses high when the active IR LED is triggered. This
 * pulse width is fixed and only indicates that a trigger happened.
 *
 * The RGB LED on the active face shows RED to indicate RX
 * The time since the last trigger is indicated by which BLUE LEDS lit:
 *
 *    P2  Cyan    = <650us              (Too short)
 *    P3  Blue    = >650us and <950us   (Just right)
 *    P4  Magenta = >950us              (Too long)
 *
 * The two error cases are latching, so if even a single error is detected then the pixel will stay on.
 *
 * Short button press clears any errors.
 * Long button press advances to the next face
 * Long button press after the last face goes to sleep mode
 *
 * SLEEP MODE
 * ==========
 *
 * Blink goes into low power sleep.
 * Pressing the button in sleep mode wakes the blink and goes to TX mode
 *
 * Usage
 * =====
 * 1. Put one blink in TX mode (one face green) and select a medium pulse width (3 blue LEDs).
 * 2. Put another blink in RX mode (one face red).
 * 3. Aim the green face from the TX mode blink and the red face from the RX mode blink at each other.
 *
 * For the blink in RTX mode...
 * Cyan indicates that the IR LED is triggering by ambient light before the pulse arrives, or something is touching the LED
 * Blue indicates that pulses with expected timing are being received
 * Magenta in Rx mode indicates that some TX pulses are being missed
 * Each of these indications will stay lit for about 0.1 second after the event so you can see it.
 *
 * If only blue on RX lights then this is a good sign that good communication is possible.
 *
 * Try different TX pulse widths by long pressing the button. Longer widths (more blue LEDs) have more energy so
 * are better able to trigger though poor communications paths, but take longer and add noise.
 *
 * Connect an oscilloscope to the A pin on both blinks. Trigger on the A pin from the TX blink and watch to
 * see how long it takes for the RX to trigger after the TX pulse is sent.
 * Watch for the min and max of the timing between the RX and TX going high on pin A of the two blinks
 * Short and consistent times are good.
 *
 * Note that service port pin A is connect to ATMEGA pin 19 (PE2).
 *
 */

#include <avr/io.h>

#define F_CPU 1000000
#include <util/delay.h>
#include <avr/sleep.h>

#include "hardware.h"

// Bit manipulation macros
#define SBI(x,b) (x|= (1<<b))           // Set bit in IO reg
#define CBI(x,b) (x&=~(1<<b))           // Clear bit in IO reg
#define TBI(x,b) (x&(1<<b))             // Test bit in IO reg

#define BUTTON_DEBOUNCE_TIME_MS 100      // Debounce button presses
#define BUTTON_LONGPRESSS_TIME_MS   500

#define IR_LED_CHARGE_TIME_US  5         // How long to charge up the IR LED in RX mode
#define IR_LED_REST_TIME_US   15         // How long to wait after trigger detected before recharging

#define TX_PULSE_SPACING_US 750          // 750us is the nominal width of a SYNC symbol - which is the longest symbol we send. Good for testing ambient.

#define RX_TOO_SHORT_TIME_US ((TX_PULSE_SPACING_US * 8) / 10 )     // For detecting errors in RX mode
#define RX_TOO_LONG_TIME_US  ((TX_PULSE_SPACING_US* 12) / 10 )

// Set up a 750us timer.
// By default we are running at 1Mhz now
// We are just using the boring default mode where TCNT1 counts up.
// Just access TCNT1 directly for timing. It clicks at 1Mhz

void init_timer1() {
    TCCR1B = _BV( CS10 );           // clk/1, starts timer at 1Mhz
}

void init_rgb_leds() {

    // common cathodes

    SBI( LED_R_DDR , LED_R_BIT);        // Set output mode
    SBI( LED_R_PORT , LED_R_BIT);       // Turn OFF (this is the cathode)

    SBI( LED_G_DDR , LED_G_BIT);        // Set output mode
    SBI( LED_G_PORT , LED_G_BIT);       // Turn OFF (this is the cathode)

    SBI( LED_B_DDR , LED_B_BIT);        // Set output mode
    SBI( LED_B_PORT , LED_B_BIT);       // Turn OFF (this is the cathode)

    // pixel anodes

    SBI( PIXEL0_DDR , PIXEL0_BIT );     // Set output mode
    SBI( PIXEL1_DDR , PIXEL1_BIT );     // Set output mode
    SBI( PIXEL2_DDR , PIXEL2_BIT );     // Set output mode
    SBI( PIXEL3_DDR , PIXEL3_BIT );     // Set output mode
    SBI( PIXEL4_DDR , PIXEL4_BIT );     // Set output mode
    SBI( PIXEL5_DDR , PIXEL5_BIT );     // Set output mode

    SBI( BLUE_SINK_DDR , BLUE_SINK_BIT );       // Enable the sink for the blue booster
    SBI( BLUE_SINK_PORT, BLUE_SINK_BIT );       // Leave high so no leakage though blue.

}


static void activateAnode( uint8_t led ) {

    // TODO: These could probably be compressed with some bit hacking

    switch (led) {

        case 0:
        SBI( PIXEL0_PORT , PIXEL0_BIT );
        break;

        case 1:
        SBI( PIXEL1_PORT , PIXEL1_BIT );
        break;

        case 2:
        SBI( PIXEL2_PORT , PIXEL2_BIT );
        break;

        case 3:
        SBI( PIXEL3_PORT , PIXEL3_BIT );
        break;

        case 4:
        SBI( PIXEL4_PORT , PIXEL4_BIT );
        break;

        case 5:
        SBI( PIXEL5_PORT , PIXEL5_BIT );
        break;

    }

}


// Deactivate all anodes. Faster to blindly do all of them than to figure out which is
// is currently on and just do that one.

static void deactivateAnodes(void) {

    // Each of these compiles to a single instruction
    CBI( PIXEL0_PORT , PIXEL0_BIT );
    CBI( PIXEL1_PORT , PIXEL1_BIT );
    CBI( PIXEL2_PORT , PIXEL2_BIT );
    CBI( PIXEL3_PORT , PIXEL3_BIT );
    CBI( PIXEL4_PORT , PIXEL4_BIT );
    CBI( PIXEL5_PORT , PIXEL5_BIT );

}

// How long to flash each color when pixel_flash() called

#define PIXEL_ON_TIME_US 10

// Turn on the selected LED. Color values are binary (0=off, anything else on)
// longer on time is more brightness
// Takes about 50us for Each color that is turned on

void pixel_flash( uint8_t face , uint8_t r , uint8_t g , uint8_t b  ) {

    // Turn on the anode for the requested pixel

    activateAnode( face );

    // Set the color cathodes

    if ( r ) {
        CBI( LED_R_PORT , LED_R_BIT );
        _delay_us( PIXEL_ON_TIME_US );
        SBI( LED_R_PORT , LED_R_BIT );
    }

    if ( g ) {
        CBI( LED_G_PORT , LED_G_BIT );
        _delay_us( PIXEL_ON_TIME_US );
        SBI( LED_G_PORT , LED_G_BIT );
    }

    if ( b ) {
        CBI(BLUE_SINK_PORT , BLUE_SINK_BIT );       // Charge the blue booster
        _delay_us( PIXEL_ON_TIME_US );

        SBI( BLUE_SINK_PORT , BLUE_SINK_BIT );     // Stop charging
        CBI( LED_B_PORT , LED_B_BIT );             // Pull boost down
        _delay_us( PIXEL_ON_TIME_US );
        SBI( LED_B_PORT , LED_B_BIT );             // blue off
    }

    // Turn off anode

    deactivateAnodes();


}


// You can connect pin A on the service port to an oscilloscope
// Note that the pin A is always behind by 2us, but at least it is consistent
#define PIN_A_HIGH() SBI( SP_A_PORT , SP_A_BIT )
#define PIN_A_LOW()  CBI( SP_A_PORT , SP_A_BIT )

void init() {


    // Set up the A pin to be OUTPUT
    SBI( SP_A_DDR , SP_A_BIT );

    // Start in TX mode where we blink IR0.
    // Set cathode and anode to output mode

    // IR LED is currently OFF because both pins init to 0
    // We will turn it on by driving the anode high

    SBI( BUTTON_PORT , BUTTON_BIT );            // Enable the pull up on the button

    init_rgb_leds();

    init_timer1();

}


template <uint8_t pulse_width, uint8_t pixel_count, uint8_t tx_led>
void send_pulses_until_button_down() {

    // Pre comute where to put the blue LEDs to show the pulse width becuase
    // we dont have time in the loop

    uint8_t start_blue = ( tx_led + 1 ) % 6 ;
    uint8_t end_blue = ( start_blue + pixel_count ) % 6;

    SBI( IR_CATHODE_DDR ,  tx_led );
    CBI( IR_CATHODE_PIN , tx_led );

    SBI( IR_ANODE_DDR   ,  tx_led );

    TCNT1 =0;                                   // Reset the stopwatch
    do {

        //PIN_A_HIGH();
        while (TCNT1 < TX_PULSE_SPACING_US );     // Wait 750us
        //PIN_A_LOW();

        TCNT1 =0;                                 // Reset inter-pulse stopwatch

        SBI( IR_ANODE_PORT , tx_led );         // LED ON!
        PIN_A_HIGH();
        _delay_us(pulse_width-4);                             // Account for 4 cycle overhead of actually changing the port
        CBI( IR_ANODE_PORT , tx_led );         // LED OFF!
        PIN_A_LOW();

        pixel_flash( tx_led , 0 , 1 , 0 );               // Green on transmitting face



        // Use the blue LEDs to show current pulse width
        uint8_t p=start_blue;

        while (p!=end_blue) {

            pixel_flash(   p , 0 , 0 , 1 );               // blue
            p++;
            if (p==6) {           // Wrap around
                p=0;
            }

        }

    } while (!BUTTON_DOWN());

}

// Cycles though the different pulse widths on button press and then returns

// We have to unroll this because there is no way to do variable timing as short as 5us (thats only 5 instructions)


static uint8_t widthstep = 2;       // This is the current width setting
                                    // it stays set even between face and mode switches
                                    // We default to the middle


template <uint8_t tx_led>
void tx_cycle_wdiths_on_led() {


    while (1) {

        switch (widthstep) {

            case 0: send_pulses_until_button_down< 5,1,tx_led>(); break;
            case 1: send_pulses_until_button_down< 7,2,tx_led>(); break;
            case 2: send_pulses_until_button_down<10,3,tx_led>(); break;
            case 3: send_pulses_until_button_down<12,4,tx_led>(); break;
            case 4: send_pulses_until_button_down<15,5,tx_led>(); break;

        }

        // When we get here, the send_pulses_until_button_down() just retruned becuase the button went down

        _delay_ms(BUTTON_DEBOUNCE_TIME_MS);             // Debounce

        // COunt how many milliseconds the button is help down for

        TCNT1 = 0;
        uint16_t ms_count =0;
        while (BUTTON_DOWN()) {

            if (TCNT1>=1000) {
                ms_count++;
                TCNT1=0;
            }

        };                          // Wait for button up again.


        if ( ms_count< BUTTON_LONGPRESSS_TIME_MS ) {

            return;     // Will step to next LED

        }


        widthstep++;
        if (widthstep==5) widthstep=0;

    }
}

void tx_mode() {

    tx_cycle_wdiths_on_led<0>();
    tx_cycle_wdiths_on_led<1>();
    tx_cycle_wdiths_on_led<2>();
    tx_cycle_wdiths_on_led<3>();
    tx_cycle_wdiths_on_led<4>();
    tx_cycle_wdiths_on_led<5>();

}

template <uint8_t rx_led>
inline void charge_ir_led() {

    SBI( IR_CATHODE_DDR , rx_led );         // Charging up cathode
    SBI( IR_CATHODE_PORT , rx_led );         // Charging up cathode
    _delay_us( IR_LED_CHARGE_TIME_US );
    CBI( IR_CATHODE_DDR , rx_led );          // Switching to input mode - Pull up enabled
    CBI( IR_CATHODE_PORT , rx_led );         // Pull up disconnected so now just floating input

}

template <uint8_t rx_led>
inline void wait_for_ir_led_trigger() {

    while ( TBI( IR_CATHODE_PIN , rx_led ));  // Spin until the charge dissipates enough for pin to read `0`

}


template <uint8_t rx_led>
void rx_mode_on_led() {

    SBI( IR_ANODE_DDR  , rx_led );
    CBI( IR_ANODE_PORT , rx_led );


    uint8_t too_short_pixel_position = (rx_led + 2) % 6;
    uint8_t just_right_pixel_positon = (rx_led + 3) % 6;
    uint8_t too_long_pixel_position  = (rx_led + 4) % 6;

    // Just remember until next pass
    // so we can display while waiting for trigger

    uint8_t just_right_flag;

    // These latch errors so we can see if they happened.
    // A button press clears any errors
    uint8_t too_short_flag;
    uint8_t too_long_flag;



    do {

        too_short_flag=0;
        just_right_flag=0;
        too_long_flag=0;

        // First pass we have to do by hand to get in phase with the transmitter

        charge_ir_led<rx_led>();
        wait_for_ir_led_trigger<rx_led>();

        TCNT1 = 0;

        do {

            charge_ir_led<rx_led>();

            // OK, we are now all charged up and ready to receive a pulse
            // Since we don't care if the pulse is shorter than 650us (that would be an error)
            // now is a good time to display the results from the last round

            pixel_flash( rx_led , 1 , 0 , 0 );               // Red on RX face to show we are receiving


            if (too_short_flag) {
                pixel_flash( too_short_pixel_position , 0 , 1 , 1 );               // Too short marker
            }

            if (just_right_flag) {
                pixel_flash( just_right_pixel_positon , 0 , 0 , 1 );               // Just right marker
                just_right_flag =0;
            }


            if (too_long_flag) {
                pixel_flash( too_long_pixel_position , 1 , 0 , 1 );               // Too long marker
            }



            PIN_A_HIGH();
            wait_for_ir_led_trigger<rx_led>();
            PIN_A_LOW();

            uint16_t time = TCNT1;                        // Capture the time
            TCNT1 = 0;                                    // Restart timer

            //_delay_us( IR_LED_REST_TIME_US );             // Let it rest. This avoids double triggers on the same IR pulse.


            if (time<RX_TOO_SHORT_TIME_US) {
                too_short_flag=1;;
            } else if (time<RX_TOO_LONG_TIME_US) {
                just_right_flag=1;
            } else {
                too_long_flag=1;
            }

            // Loop back around to charge again

       } while (!BUTTON_DOWN());

        _delay_ms(BUTTON_DEBOUNCE_TIME_MS);             // Debounce

        TCNT1 = 0;
        uint16_t ms_count =0;
        while (BUTTON_DOWN()) {

            if (TCNT1>=1000) {
                ms_count++;
                TCNT1=0;
            }

        };                          // Wait for button up again.

        _delay_ms(BUTTON_DEBOUNCE_TIME_MS);             // Debounce

        if ( ms_count > BUTTON_LONGPRESSS_TIME_MS ) {

            return;     // Long press will step to next LED

        }

        // Short press just clears errors.

    } while (1);

}

void rx_mode() {
    rx_mode_on_led<0>();
    rx_mode_on_led<1>();
    rx_mode_on_led<2>();
    rx_mode_on_led<3>();
    rx_mode_on_led<4>();
    rx_mode_on_led<5>();
}


EMPTY_INTERRUPT(BUTTON_ISR);        // We have to turn on the interrupt to be able to wake, but don't really want to catch it

void sleep_with_button_wake() {

    set_sleep_mode( SLEEP_MODE_PWR_DOWN );      // Lowest power

    SBI( PCICR , BUTTON_PCI );          // Enable the pin group so button change can interrupt (int wakes us from sleep)
    do {

        sei();
        sleep_cpu();
        cli();
        _delay_ms(BUTTON_DEBOUNCE_TIME_MS);

    } while (!BUTTON_DOWN());           // Keep sleeping until button pressed again.

    _delay_ms(BUTTON_DEBOUNCE_TIME_MS);

}

int main(void) {

    init();

    while (1) {
        tx_mode();
        rx_mode();
        sleep_with_button_wake();
    }

}

