/*
 * ir_comms.h
 *
 * All the functions for communication and waking on the 6 IR LEDs on the tile edges.
 *
 */ 

#ifndef RGB_PIXELS_H_
#define RGB_PIXELS_H_

#include <avr/io.h>

// True today, but could imagine a blinks with 1 pixel or one with with 18 pixels...

#define PIXEL_COUNT FACE_COUNT

// Setup pins, interrupts. Call once at power up.

void pixel_init(void);

// Enable pixels after a call to pixel_init or pixel_disable
// Pixels will return to the color they had before being disabled.

void pixel_enable(void);

// Turn of all pixels and the timer that drives them.
// You'd want to do this before going to sleep.

void pixel_disable(void);
        
/** Display interface ***/

// Set a single pixel's RGB value
// p is the pixel number. 0 < p < PIXEL_COUNT
// r,g,b are brightness values. 0=off, 255=full brightness

// Note that there will likely be fewer than 256 actual visible values, but the mapping will be linear and smooth

// TODO: Balance, normalize, power optimize, and gamma correct these functions
// Need some exponential compression at the top here
// Maybe look up tables to make all calculations be one step at the cost of memory?

void pixel_setRGB( uint8_t p, uint8_t r, uint8_t g, uint8_t b );

void pixel_SetAllRGB( uint8_t r, uint8_t g, uint8_t b  );

/** Timing ***/


#define CYCLES_PER_SECOND (F_CPU)

#define CYCLES_PER_PIXEL ( 8 * 256 * 5  )
    //    8 = Timer prescaller
    //  256 = Timer steps per overflow
    //    5 = Overflow Phases per pixel
    //    6 = Pixels per frame

#define PIXELS_PER_SECOND ( CYCLES_PER_SECOND / CYCLES_PER_PIXEL )

#define MILLIS_PER_SECOND 1000

#define CYCLES_PER_MILLISECOND ( CYCLES_PER_SECOND / MILLIS_PER_SECOND )

// Milliseconds per pixel

#define MILLIS_PER_PIXEL ( MILLIS_PER_SECOND / PIXELS_PER_SECOND )

// Milliseconds elapsed since last call to pixel_enable()
// Resolution is limited by the pixel clock, which only updates about 
// once per 2.5ms with 4Mhz system clock and /8 prescaller. 
// Resets back to 0 on pixel_disable()
// Assumes interrupts are enabled when called. 

uint32_t pixel_mills(void);
    
#endif /* RGB_PIXELS_H_ */