/*
 * blinkbios_pixel_block.h
 *
 * Defines the shared memory block used to set the color of the pixels
 * and to determine best time to update the values
 *
 */

#ifndef BLINKBIOS_PIXEL_BLOCK_H_
#define BLINKBIOS_PIXEL_BLOCK_H_

/** Display interface ***/

#define PIXEL_COUNT 6

// Here are the raw compare register values for each pixel
// These are precomputed from brightness values because we read them often from inside an ISR
// Note that for red & green, 255 corresponds to OFF and 0 is full on

struct  rawpixel_t {
    uint8_t rawValueR;
    uint8_t rawValueG;
    uint8_t rawValueB;
    uint8_t paddng;             // Adding this padding save about 10 code bytes since x4 (double-doubling) is faster than adding 3.

    rawpixel_t( uint8_t in_raw_r , uint8_t in_raw_g , uint8_t in_raw_b ) {
        rawValueR =  in_raw_r;
        rawValueG = in_raw_g;
        rawValueB = in_raw_b;

    }

    rawpixel_t() {};

};


const rawpixel_t RAW_PIXEL_OFF( 0xff , 0xff, 0xff );

// We need these struct gymnastics because C fixed array typedefs do not work
// as you (I?) think they would...
// https://stackoverflow.com/questions/4523497/typedef-fixed-length-array

struct blinkbios_pixelblock_t {

    rawpixel_t rawpixels[PIXEL_COUNT];

    // This is cleared when we are done displaying the current buffer values and about to reset and start again
    // Once this is cleared, you have one pixel interrupt period to get your new data into the pixel buffer
    // before the next refresh cycle starts.

    // We picked to clear this rather than set to 1 because in the ISR is is faster to set to 0 since R1 is always loaded with 0

    volatile uint8_t vertical_blanking_interval;


    // Below is some global state that is probably not interesting to user code

    uint8_t currentPixelIndex;  // Which pixel are we currently lighting? Pixels are multiplexed and only refreshed one at a time in sequence.
    uint8_t phase;              // Phase up updating the current pixel. There are 5 phases that include lighting each color, charging the charge pump, and resting the charge pump.

};

extern blinkbios_pixelblock_t blinkbios_pixel_block;        // Currently being displayed. You can have direct access to this to save memory,
                                                  // but use the vertical retrace to avoid visual tearing on updates

#endif /* BLINKBIOS_PIXEL_BLOCK_H_ */