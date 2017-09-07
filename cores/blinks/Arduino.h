// Reworked for Blinks platform.
// Really should be renamed Blinks.h, but Arduino.h seems to be hardcoded lots of places.

/*
  Arduino.h - Main include file for the Arduino SDK
  Copyright (c) 2005-2013 Arduino Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef Arduino_h
#define Arduino_h

#include "blinks.h"

#include <stdbool.h>
#include <math.h>

#include <avr/pgmspace.h>

// Grab _delay_ms()- used to implement delay() below
// TODO: We should give the user a better way?

#include <util/delay.h>			

#ifdef __cplusplus
extern "C"{
#endif

#define HIGH 0x1
#define LOW  0x0

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352


#define LSBFIRST 0
#define MSBFIRST 1

// undefine stdlib's abs if encountered
#ifdef abs
#undef abs
#endif

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

typedef unsigned int word;

#define bit(b) (1UL << (b))

typedef bool boolean;
typedef uint8_t byte;

void setup(void);
void loop(void);

#ifdef __cplusplus
} // extern "C"
#endif



#ifdef __cplusplus

uint16_t makeWord(uint16_t w);
uint16_t makeWord(byte h, byte l);

#define word(...) makeWord(__VA_ARGS__)

// WMath prototypes
long random(long);
long random(long, long);
void randomSeed(unsigned long);
long map(long, long, long, long, long);

#endif



// *** Now the Blinks specific API functions

// Grab the FACE_COUNT, F_CPU, 
#include "blinks.h"

// *** PIXEL FUNCTIONS ***

// True today, but could imagine a blinks with 1 pixel or one with with 18 pixels...

#define PIXEL_COUNT FACE_COUNT

// Set a single pixel's RGB value
// p is the pixel number. 0 < p < PIXEL_COUNT
// r,g,b are brightness values. 0=off, 255=full brightness

// Note that there will likely be fewer than 256 actual visible values, but the mapping will be linear and smooth

// TODO: Balance, normalize, power optimize, and gamma correct these functions
// Need some exponential compression at the top here
// Maybe look up tables to make all calculations be one step at the cost of memory?

void setPixelRGB( uint8_t p, uint8_t r, uint8_t g, uint8_t b );


// Set a single pixel's HSB value
// p is the pixel number. 0 < p < PIXEL_COUNT

void setPixelHSB( uint8_t p, uint8_t inHue, uint8_t inSaturation, uint8_t inBrightness );

// Set all pixels to one color

void setAllRGB( uint8_t r, uint8_t g, uint8_t b );

// Turn of all pixels and the timer that drives them.
// You'd want to do this before going to sleep.

void disablePixels(void);

// Re-enable pixels after a call to disablePixels.
// Pixels will return to the color they had before being disabled.

void enablePixels(void);

// *** BUTTON FUNCTIONS ***

// TODO: This will be replaced with proper button functions.
// Returns 1 if button pressed since the last time this was called

uint8_t buttonPressed(void);

// Returns 1 if button is currently pressed down

uint8_t buttonDown(void);

// Delay for specified number of milliseconds

// TODO: Make some kind of delayed promise mechanism

#define delay(ms) _delay_ms(ms)

// *** IR COMMS FUNCTIONS ***

// TODO: Give people bytes rather than measly dibits

// Returns last received dibit (value 0-3) for requested face
// Blocks if no data ready
// `face` must be less than FACE_COUNT

uint8_t irReadDibit( uint8_t face);

// Returns true if data is available to be read on the requested face
// Always returns immediately 
// Cleared by subseqent irReadDibit()
// `face` must be less than FACE_COUNT

uint8_t irIsAvailable( uint8_t face );

// Returns true if data was lost because new data arrived before old data was read
// Next read will return the older data (new data does not over write old)
// Always returns immediately
// Cleared by subseqent irReadDibit()
// `face` must be less than FACE_COUNT

uint8_t irOverFlowFlag( uint8_t face );

// Transmits the lower 2 bits (dibit) of data on requested face
// Blocks if there is already a transmission in progress on this face
// Returns immediately and continues transmission in background if no transmit already in progress
// `face` must be less than FACE_COUNT

void irSendDibit( uint8_t face , uint8_t data );


// Transmits the lower 2 bits (dibit) of data on all faces
// Blocks if there is already a transmission in progress on any face
// Returns immediately and continues transmission in background if no transmits are already in progress

void irSendAllDibit(  uint8_t data );

// Blocks if there is already a transmission in progress on this face
// Returns immediately if no transmit already in progress
// `face` must be less than FACE_COUNT

void irFlush( uint8_t face );

// *** POWER FUNCTIONS ***

// Sorry, I can't figure out a better way to get Arduino IDE to compile this type...

#ifndef SLEEP_TIMEOUT_
#define SLEEP_TIMEOUT_

// Possible sleep timeout values

typedef enum {

    TIMEOUT_16MS = (_BV(WDIE) ),
    TIMEOUT_32MS = (_BV(WDIE) | _BV( WDP0) ),
    TIMEOUT_64MS = (_BV(WDIE) | _BV( WDP1) ),
    TIMEOUT_125MS= (_BV(WDIE) | _BV( WDP1) | _BV( WDP0) ),
    TIMEOUT_250MS= (_BV(WDIE) | _BV( WDP2) ),
    TIMEOUT_500MS= (_BV(WDIE) | _BV( WDP2) | _BV( WDP0) ),
    TIMEOUT_1S   = (_BV(WDIE) | _BV( WDP2) | _BV( WDP1) ),
    TIMEOUT_2S   = (_BV(WDIE) | _BV( WDP2) | _BV( WDP1) | _BV( WDP0) ),
    TIMEOUT_4S   = (_BV(WDIE) | _BV( WDP3) ),
    TIMEOUT_8S   = (_BV(WDIE) | _BV( WDP3) | _BV( WDP0) )
    
} sleepTimeoutType;


#endif

#define WAKEON_IR_BITMASK_NONE     0             // Don't wake on any IR change
#define WAKEON_IR_BITMASK_ALL      IR_BITS       // Don't wake on any IR change

// Goto low power sleep - get woken up by button or IR LED
// Be sure to turn off all pixels before sleeping since
// the PWM timers will not be running so pixels will not look right.

// TODO: We should probably ALWAYS sleep with timers on between frames to save CPU power.

void powerdown(void);

// Sleep with a predefined timeout.
// This is very power efficient since chip is stopped except for WDT

void powerdownWithTimeout( sleepTimeoutType timeout );


#endif
