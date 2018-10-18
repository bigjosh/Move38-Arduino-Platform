/*
 * blinkOS.h
 *
 * This defines a high-level interface to the blinks operating system.
 *
 */


// Gets us uintx_t
// Not sure why, but it MUST be in the header and not the cpp
#include <stdint.h>

#define IR_FACE_COUNT 6
#define PIXEL_FACE_COUNT 6

// TODO: Change time to uint24 _t
// Supported!!! https://gcc.gnu.org/wiki/avr-gcc#types
// typedef __uint24 millis_t;

typedef uint32_t millis_t;

// loopstate is passed in and out of the user program on each pass
// through the main event loop



// I know this is ugly, but keeping them in a single byte lets us pass them by value
// and also lets us OR them together.

#define BUTTON_BITFLAG_PRESSED          0b00000001
#define BUTTON_BITFLAG_LONGPRESSED      0b00000010
#define BUTTON_BITFLAG_RELEASED         0b00000100
#define BUTTON_BITFLAG_SINGLECLICKED    0b00001000
#define BUTTON_BITFLAG_DOUBECLICKED     0b00010000
#define BUTTON_BITFLAG_MULITCLICKED     0b00100000

struct buttonstate_t {

    uint8_t down;               // 1 if button is currently down

    uint8_t bitflags;

    uint8_t clickcount;         // Number of clicks on most recent multiclick

};


struct ir_data_buffer_t {

    const uint8_t *data;
    uint8_t len;
    uint8_t ready_flag;              // 1 if new packet received

};

struct loopstate_in_t {

    ir_data_buffer_t ir_data_buffers[IR_FACE_COUNT];

    buttonstate_t buttonstate;

    uint8_t woke_flag;           // Have we slept and woke?


    millis_t millis;             // elapsed milliseconds since start

    uint8_t futureexapansion[20];  // Save some space for future expansion

};

// Each pixel has 32 brightness levels for each of the three colors (red,green,blue)
// These brightness levels are normalized to be visually linear with 0=off and 31=max brightness

union pixelColor_t {
    
    struct {
        uint8_t changed:1;
        uint8_t r:5;
        uint8_t g:5;
        uint8_t b:5;
    };
    
    uint16_t as_uint16;
    
    pixelColor_t(uint8_t r_in , uint8_t g_in, uint8_t b_in );
    
};

inline pixelColor_t::pixelColor_t(uint8_t r_in , uint8_t g_in, uint8_t b_in ) {
    
    r=r_in;
    g=g_in;
    b=b_in;
    
}


struct loopstate_out_t {

    pixelColor_t colors[PIXEL_FACE_COUNT];  // Current RGB colors of all faces
                                            // Topmost bit set indicates it changed

    uint8_t futureexapansion[20];  // Save some space for future expansion

};

// The state record we are sending to userland
extern loopstate_in_t loopstate_in;

// These are provided by blinklib

void setupEntry();
void loopEntry( loopstate_in_t const *loopstate_in , loopstate_out_t *loopstate_out);
