/*
 * blinkOS.h
 *
 * This defines a high-level interface to the blinks operating system.
 *
 */


// Gets us uintx_t
// Not sure why, but it MUST be in the header and not the cpp
#include <stdint.h>
#include "pixelcolor.h"

#define IR_FACE_COUNT 6
#define PIXEL_FACE_COUNT 6

// Maximum IR packet size. Larger values more efficient for large block transfers, but use more memory.
// Packets larger than this are silently discarded.
// Note that we allocate 6 of these, so memory usage goes up fast with bigger packets sizes.

// We picked 35 here to have room for 32 bytes of program data, 1 byte header, 2 bytes block number.

#define IR_RX_PACKET_SIZE     35


// TODO: Change time to uint24 _t
// Supported!!! https://gcc.gnu.org/wiki/avr-gcc#types
// typedef __uint24 millis_t;

typedef uint32_t millis_t;

// loopstate is passed in and out of the user program on each pass
// through the main event loop

// I know this is ugly, but keeping them in a single byte lets us pass them by value
// and also lets us OR them together. Efficiency in the updater trumps since it
// runs every millisecond.

#define BUTTON_BITFLAG_PRESSED          0b00000001
#define BUTTON_BITFLAG_LONGPRESSED      0b00000010
#define BUTTON_BITFLAG_RELEASED         0b00000100
#define BUTTON_BITFLAG_SINGLECLICKED    0b00001000
#define BUTTON_BITFLAG_DOUBECLICKED     0b00010000
#define BUTTON_BITFLAG_MULITCLICKED     0b00100000
#define BUTTON_BITFLAG_DUMMY            0b01000000

struct buttonstate_t {

    uint8_t down;               // 1 if button is currently down

    uint8_t bitflags;

    uint8_t clickcount;         // Number of clicks on most recent multiclick

};


struct ir_data_buffer_t {

    const uint8_t *data;
    uint8_t len;
    uint8_t ready_flag;              // 1 if new packet received. Call markread_data_buffer();

};

// Sends immediately. Blocks until send is complete.
// Higher level should provide some collision control.
// Returns 0 if it was not able to send because there was already an RX in progress on this face

uint8_t ir_send_userdata( uint8_t face, const uint8_t *data , uint8_t len );

struct loopstate_in_t {

    ir_data_buffer_t ir_data_buffers[IR_FACE_COUNT];

    buttonstate_t buttonstate;   // Note that the flags in here are cleared after every pass though the loop

    uint8_t woke_flag;           // Have we slept and woke?


    millis_t millis;             // elapsed milliseconds since start

    uint8_t futureexapansion[20];  // Save some space for future expansion

};

// Mark a packet as consumed so the buffer can receive the next one.
// Done automatically each time loopEntry() returns, but this can let you free up the packet sooner
// for better throughput

// Mark a packet as consumed so the buffer can receive the next one.
// Done automatically each time loopEntry() returns, but this can let you free up the packet sooner
// for better throughput

void ir_mark_packet_read( uint8_t face );

struct loopstate_out_t {

    pixelColor_t colors[PIXEL_FACE_COUNT];  // Current RGB colors of all faces. 5 bits each.
                                        // Topmost bit set indicates it changed

    uint8_t futureexapansion[20];  // Save some space for future expansion

};


// we export these and communicate with the next layer up using them 

extern loopstate_out_t loopstate_out;
extern loopstate_in_t loopstate_in;

// These are provided by blinklib

//void setupEntry();
void loopEntry();


// Handy

#define OS_FACE_COUNT PIXEL_FACE_COUNT

// 'Cause C ain't got iterators and all those FOR loops are too ugly.
#define OS_FOREACH_FACE(x) for(int x = 0; x < OS_FACE_COUNT ; ++ x)       // Pretend this is a real language with iterators

// Get the number of elements in an array.
#define OS_COUNT_OF(x) ((sizeof(x)/sizeof(x[0])))