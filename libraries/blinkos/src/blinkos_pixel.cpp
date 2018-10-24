
#include <stdint.h>             // unintX_t
#include <avr/pgmspace.h>

#include "pixel.h"
#include "blinkos_pixel.h"

// init the raw pixel buffers to all 0xff (off)

// Display the buffered pixels by swapping the buffer. Blocks until next frame starts.

void pixel_updateDisplayBuffer( const rawpixelset_t *newRawPixelSet ) {

    vertical_blanking_interval=1;

    while (vertical_blanking_interval);   // The pixel ISR will set this bit after it finishes the last pixel of the current refresh cycle

    displayedRawPixelSet = *newRawPixelSet;     // Copy new data into the display buffer

}

// TODO: Move this out of core...

rawpixelset_t rawPixelBufferSet = {
    RAW_PIXEL_OFF,
    RAW_PIXEL_OFF,
    RAW_PIXEL_OFF,
    RAW_PIXEL_OFF,
    RAW_PIXEL_OFF,
    RAW_PIXEL_OFF,
};


// Update the pixel buffer with raw PWM register values.
// Larger pwm values map to shorter PWM cycles (255=off) so for red and green
// there is an inverse but very non linear relation between raw value and brightness.
// For blue is is more complicated because of the charge pump. The peak brightness is somewhere
// in the middle.

// Values set here are buffered into next call to pixel_displayBufferedPixels()


// Gamma table courtesy of adafruit...
// https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
// Compressed down to 32 entries, normalized for our raw values that start at 255 off.

// TODO: Possible that green and red are similar enough that we can combine them into one table and save some flash space

static const uint8_t PROGMEM gamma8R[32] = {
    255,254,253,251,250,248,245,242,238,234,230,224,218,211,204,195,186,176,165,153,140,126,111,95,78,59,40,19,13,9,3,1
};

static const uint8_t PROGMEM gamma8G[32] = {
    255,254,253,251,250,248,245,242,238,234,230,224,218,211,204,195,186,176,165,153,140,126,111,95,78,59,40,19,13,9,3,1
};

static const uint8_t PROGMEM gamma8B[32] = {
    255,254,253,251,250,248,245,242,238,234,230,224,218,211,204,195,186,176,165,153,140,126,111,95,78,59,40,19,13,9,3,1
};



void blinkos_pixel_bufferedSetPixel( uint8_t pixel, pixelColor_t newColor) {

    rawpixel_t *rawpixel = &(rawPixelBufferSet.rawpixels[pixel]);

    rawpixel->rawValueR= pgm_read_byte(&gamma8R[newColor.r]);
    rawpixel->rawValueG= pgm_read_byte(&gamma8G[newColor.g]);
    rawpixel->rawValueB= pgm_read_byte(&gamma8B[newColor.b]);

}

void blinkos_pixel_displayBufferedPixels(void) {

    //displayedRawPixelSet.rawpixels[0].rawValueR=5;
    pixel_updateDisplayBuffer( &rawPixelBufferSet );

}


