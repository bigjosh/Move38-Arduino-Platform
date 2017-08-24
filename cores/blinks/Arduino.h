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

#include <stdbool.h>
#include <math.h>

#include <avr/pgmspace.h>

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

// PIXEL FUNCTIONS

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

void setRGB( uint8_t r, uint8_t g, uint8_t b );

void test(void);

#endif
