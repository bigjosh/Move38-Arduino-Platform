/*
 * SimpleIRTest.cpp
 *
 * This is the simplest possible IR test program.
 * Is accesses the blink hardware directly with no interrupts and requires no library code (runs bare metal).
 *
 * It has two modes: TX and RX.
 *
 * TX mode
 * =======
 * Blink sends an IR pulse every 1ms on face IR0.
 * The RGB LED on P0 shows green to indicate TX mode.
 * The pulse width is indicated by the number of BLUE LEDS lit:
 *
 *    1 blue = 5us
 *    2 blue = 7us
 *    3 blue = 10us
 *    4 blue = 12us
 *    5 blue = 15us
 *
 * The 'A' pin on the service port connector goes high while the IR LED is on.
 *
 * RX mode
 * =======
 * Blink listens for an IR pulse on face IR0 every 2ms on face IR0.
 * The RGB LED on face P0 shows RED to indicate RX.
 * The time since the last trigger is indicated by which BLUE LEDS lit:
 *
 *    P2  Cyan    = <650us              (Too short)
 *    P3  Blue    = >650us and <950us   (Just right)
 *    P4  Magenta = >950us              (Too long)
 *
 * The 'A' pin on the service port connector pulses high when the IR0 LED is triggered. This pulse width is fixed and
 * only indicates that a trigger happened.
 *
 * Usage
 * =====
 * You can cycle between modes and TX mode pulse times by pressing and releasing the button.
 * Put one blink in TX mode and select a pulse width. Put another blink in RX mode. Aim their IR0 faces together.
 * For the blink in RTX mode...
 * P2 Cyan indicates that the IR LED is triggering by ambient light before the pulse arrives, or something is touching the LED
 * P3 Blue indicates that pulses with expected timing are being received
 * P4 Magenta in Rx mode indicates that some TX pulses are being missed
 * Each of these indications will stay lit for about 0.1 second after the event so you can see it.
 *
 * If only P5 on RX lights then this is a good sign that good communication is possible.
 *
 * Connect an oscilloscope to the A pin on both blinks. Trigger on the A pin from the TX blink and watch to
 * see how long it takes for the RX to trigger after the TX pulse is sent.
 * Watch for the min and max of the timing between the RX and TX going high on pin A of the two blinks
 * Short and consistent times are good.
 *
 */

#include <avr/io.h>

#define F_CPU 1000000
#include <util/delay.h>

#include "hardware.h"

// Bit manipulation macros
#define SBI(x,b) (x|= (1<<b))           // Set bit in IO reg
#define CBI(x,b) (x&=~(1<<b))           // Clear bit in IO reg
#define TBI(x,b) (x&(1<<b))             // Test bit in IO reg


#define BUTTON_DEBOUNCE_TIME_MS 100      // Debounce button presses

#define IR_LED_CHARGE_TIME_US  5         // How long to charge up the IR LED in RX mode
#define IR_LED_REST_TIME_US   15         // How long to wait after trigger detected before recharging

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

#define TEST_IR_LED 0           // Which LED should we use?


// You can connect pin A on the service port to an oscilloscope
// Note that the pin A is always behind by 2us, but at least it is consistent
#define PIN_A_HIGH() SBI( SP_A_PORT , SP_A_BIT )
#define PIN_A_LOW()  CBI( SP_A_PORT , SP_A_BIT )

void init() {


    // Set up the A pin to be OUTPUT
    SBI( SP_A_DDR , SP_A_BIT );

    // Start in TX mode where we blink IR0.
    // Set cathode and anode to output mode

    SBI( IR_CATHODE_DDR , TEST_IR_LED );        // Cathode output mode
    SBI( IR_ANODE_DDR   , TEST_IR_LED );        // Anode output mode

    // IR LED is currently OFF because both pins init to 0
    // We will turn it on by driving the anode high

    SBI( BUTTON_PORT , BUTTON_BIT );            // Enable the pull up on the button

    init_rgb_leds();

    init_timer1();

}



// Cycles though the different pulse widths on button press and then returns

// We have to unroll this because there is no way to do variable timing as short as 5us (thats only 5 instructions)

void tx_mode() {

    SBI( IR_CATHODE_DDR ,TEST_IR_LED );

    TCNT1 =0;                                   // Reset the stopwatch

    do {

        while (TCNT1 < 750 );                       // Wait 750us
        SBI( IR_ANODE_PORT , TEST_IR_LED );         // LED ON!
        PIN_A_HIGH();
        _delay_us(5-4);                             // Account for 4 cycle overhead of actually changing the port
        CBI( IR_ANODE_PORT , TEST_IR_LED );         // LED OFF!
        PIN_A_LOW();
        TCNT1 =0;                                   // Reset interpulse stopwatch

        pixel_flash( 0 , 0 , 1 , 0 );               // Show we are transmitting
        pixel_flash( 1 , 0 , 0 , 1 );               // 1 blue = 5us

    } while (!BUTTON_DOWN());

    _delay_ms(BUTTON_DEBOUNCE_TIME_MS);             // Debounce
    while (BUTTON_DOWN());                          // Wait for button lift

    // --------

    do {

        while (TCNT1 < 750 );                       // Wait 750us
        SBI( IR_ANODE_PORT , TEST_IR_LED );         // LED ON!
        PIN_A_HIGH();
        _delay_us(7-4);                             // Account for 2 cycle overhead of actually changing the port
        CBI( IR_ANODE_PORT , TEST_IR_LED );         // LED OFF!
        PIN_A_LOW();
        TCNT1 =0;                                   // Reset interpulse stopwatch

        pixel_flash( 0 , 0 , 1 , 0 );               // Show we are transmitting
        pixel_flash( 1 , 0 , 0 , 1 );               // 1 blue = 5us
        pixel_flash( 2 , 0 , 0 , 1 );               // 1 blue = 5us


    } while (!BUTTON_DOWN());

    _delay_ms(BUTTON_DEBOUNCE_TIME_MS);             // Debounce
    while (BUTTON_DOWN());                          // Wait for button lift


    // --------

    do {

        while (TCNT1 < 750 );                       // Wait 750us
        SBI( IR_ANODE_PORT , TEST_IR_LED );         // LED ON!
        PIN_A_HIGH();
        _delay_us(10-4);                             // Account for 2 cycle overhead of actually changing the port
        CBI( IR_ANODE_PORT , TEST_IR_LED );         // LED OFF!
        PIN_A_LOW();
        TCNT1 =0;                                   // Reset interpulse stopwatch

        pixel_flash( 0 , 0 , 1 , 0 );               // Show we are transmitting
        pixel_flash( 1 , 0 , 0 , 1 );               // 1 blue = 5us
        pixel_flash( 2 , 0 , 0 , 1 );               // 1 blue = 5us
        pixel_flash( 3 , 0 , 0 , 1 );               // 1 blue = 5us


    } while (!BUTTON_DOWN());

    _delay_ms(BUTTON_DEBOUNCE_TIME_MS);             // Debounce
    while (BUTTON_DOWN());                          // Wait for button lift

    // --------

    do {

        while (TCNT1 < 750 );                       // Wait 750us
        SBI( IR_ANODE_PORT , TEST_IR_LED );         // LED ON!
        PIN_A_HIGH();
        _delay_us(12-4);                             // Account for 2 cycle overhead of actually changing the port
        CBI( IR_ANODE_PORT , TEST_IR_LED );         // LED OFF!
        PIN_A_LOW();
        TCNT1 =0;                                   // Reset interpulse stopwatch

        pixel_flash( 0 , 0 , 1 , 0 );               // Show we are transmitting
        pixel_flash( 1 , 0 , 0 , 1 );               // 1 blue = 5us
        pixel_flash( 2 , 0 , 0 , 1 );               // 1 blue = 5us
        pixel_flash( 3 , 0 , 0 , 1 );               // 1 blue = 5us
        pixel_flash( 4 , 0 , 0 , 1 );               // 1 blue = 5us


    } while (!BUTTON_DOWN());

    _delay_ms(BUTTON_DEBOUNCE_TIME_MS);             // Debounce
    while (BUTTON_DOWN());                          // Wait for button lift

    // --------

    do {

        while (TCNT1 < 750 );                       // Wait 750us
        SBI( IR_ANODE_PORT , TEST_IR_LED );         // LED ON!
        PIN_A_HIGH();
        _delay_us(15-4);                             // Account for 2 cycle overhead of actually changing the port
        CBI( IR_ANODE_PORT , TEST_IR_LED );         // LED OFF!
        PIN_A_LOW();
        TCNT1 =0;                                   // Reset interpulse stopwatch

        pixel_flash( 0 , 0 , 1 , 0 );               // Show we are transmitting
        pixel_flash( 1 , 0 , 0 , 1 );               // 1 blue = 5us
        pixel_flash( 2 , 0 , 0 , 1 );               // 1 blue = 5us
        pixel_flash( 3 , 0 , 0 , 1 );               // 1 blue = 5us
        pixel_flash( 4 , 0 , 0 , 1 );               // 1 blue = 5us
        pixel_flash( 5 , 0 , 0 , 1 );               // 1 blue = 5us


    } while (!BUTTON_DOWN());

    _delay_ms(BUTTON_DEBOUNCE_TIME_MS);             // Debounce
    while (BUTTON_DOWN());                          // Wait for button lift


}

#define  COUNTDOWN_FLAG_START 20       // Making this higher will make the feedback pixel stay on longer after an event

void rx_mode() {

    uint16_t last_time;

    // These flags are int so that when set they will stay set long enough to actually see them
    // so we set to a count of how many times though the loop they should get lit
    // and decrement each time we show them until 0.

    uint8_t countdown_flag_too_short;
    uint8_t countdown_flag_just_right;
    uint8_t countdown_flag_too_long;

    TCNT1 = 0;

    do {

        SBI( IR_CATHODE_PORT , TEST_IR_LED );         // Charging up cathode
        _delay_us( IR_LED_CHARGE_TIME_US );
        CBI( IR_CATHODE_DDR , TEST_IR_LED );          // Switching to input mode - Pull up enabled
        CBI( IR_CATHODE_PORT , TEST_IR_LED );         // Pull up disconnected so now just floating input

        // OK, we are now all charged up and ready to receive a pulse
        // Since we don't care if the pulse is shorter than 650us (that would be an error)
        // now is a good time to display the results from the last round

        pixel_flash( 0 , 1 , 0 , 0 );               // Too short marker

        if (countdown_flag_too_short) {
            pixel_flash( 2 , 0 , 1 , 1 );               // Too short marker
            countdown_flag_too_short--;
        }

        if (countdown_flag_just_right) {
            pixel_flash( 3 , 0 , 0 , 1 );               // Just right marker
            countdown_flag_just_right--;
        }


        if (countdown_flag_too_long) {
            pixel_flash( 4 , 1 , 0 , 1 );               // Too long marker
            countdown_flag_too_long--;
        }

        while ( TBI( IR_CATHODE_PIN , TEST_IR_LED ));  // Spin until the charge dissipates enough for pin to read `0`

        uint16_t time = TCNT1;                        // Capture the time
        TCNT1 = 0;                                    // Restart timer

        _delay_us( IR_LED_REST_TIME_US );             // Let it rest. This avoids double triggers on the same IR pulse.

        if (time<650) {
            countdown_flag_too_short =COUNTDOWN_FLAG_START;
        } else if (time<850) {
            countdown_flag_just_right=COUNTDOWN_FLAG_START;
        } else {
            countdown_flag_too_long  =COUNTDOWN_FLAG_START;
        }

        // Loop back around to charge again


    } while (!BUTTON_DOWN());

    _delay_ms(BUTTON_DEBOUNCE_TIME_MS);             // Debounce
    while (BUTTON_DOWN());                          // Wait for button lift

}

int main(void) {

    init();

    while (1) {

        tx_mode();
        rx_mode();
    }

}

