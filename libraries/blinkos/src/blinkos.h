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

struct loopstate_in_t {

    uint8_t ir_rx[IR_FACE_COUNT];   // Most recently received IR values on each face
                                    // Topmost bit indicates new


    buttonstate_t buttonstate;

    bool woke_flag:1;           // Have we slept and woke?


    millis_t millis;               // elapsed milliseconds since start

    uint8_t futureexapansion[20];  // Save some space for future expansion

};

struct loopstate_out_t {
    
    uint8_t ir_tx[IR_FACE_COUNT];   // IR values to send on each face
                                    // Topmost bit indicates new (newer gets priority or resends to improve latency)
    
    // Topmost bit set indicates it changed
    uint16_t colors[PIXEL_FACE_COUNT];  // Current RGB colors of all faces

    uint8_t futureexapansion[20];  // Save some space for future expansion    
};    

// The state record we are sending to userland
extern loopstate_in_t loopstate_in;

// These are provided by blinklib

extern void setupEntry();
extern void loopEntry( loopstate_in_t const *loopstate_in , loopstate_out_t *loopstate_out);
