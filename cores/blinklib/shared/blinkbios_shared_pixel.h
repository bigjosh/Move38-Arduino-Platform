/*
 * blinkbios_pixel_block.h
 *
 * Defines the shared memory block used to set the color of the pixels
 * and to determine best time to update the values
 *
 */

#ifndef BLINKBIOS_PIXEL_BLOCK_H_
#define BLINKBIOS_PIXEL_BLOCK_H_

// #define USER_VOLATILE or BIOS_VOLATILE based on the presence of #define BIOS_VOLATILE_FLAG
#include "blinkbios_shared_volatile.h"

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

// pixel_color_t is a higher level view RGB encoded of a pixel

// Each pixel has 32 brightness levels for each of the three colors (red,green,blue)
// These brightness levels are normalized to be visually linear with 0=off and 31=max brightness

// Note use of anonymous union members to let us switch between bitfield and int
// https://stackoverflow.com/questions/2468708/converting-bit-field-to-int

union pixelColor_t {

    struct {
        uint8_t reserved:1;
        uint8_t r:5;
        uint8_t g:5;
        uint8_t b:5;
    };

    uint16_t as_uint16;

    pixelColor_t();
    pixelColor_t(uint8_t r_in , uint8_t g_in, uint8_t b_in );
    pixelColor_t(uint8_t r_in , uint8_t g_in, uint8_t b_in , uint8_t reserverd_in );

};

inline pixelColor_t::pixelColor_t(uint8_t r_in , uint8_t g_in, uint8_t b_in ) {

    r=r_in;
    g=g_in;
    b=b_in;

}

inline pixelColor_t::pixelColor_t(uint8_t r_in , uint8_t g_in, uint8_t b_in , uint8_t reserverd_in ) {

    r=r_in;
    g=g_in;
    b=b_in;
    reserved = reserverd_in;

}

/*

template<uint8_t r , uint8_t g , uint8_t b > pixelColor_t preset_pixelcolor_t {
{
    pixelColor_t p = pixelColor_t( r , g , b );
    
    
};

*/

// Maximum value you can assign to one of the primaries in a pixelColor_t
#define PIXELCOLOR_PRIMARY_FULL 31
#define PIXELCOLOR_PRIMARY_HALF 15

// So this mess below is my way of emulating constexpr in C++ <11
// These defines will expand and eval at compile time down to a single uint16_t load
// if instead we tried using a `const pixelColor_r`, then it would blow up into
// a constructor call at runtime. Yuck. 

#define PIXEL_COLOR_FULL_GREEN pixelColor_t( 0 , PIXELCOLOR_PRIMARY_FULL , 0  )
#define PIXEL_COLOR_HALF_GREEN pixelColor_t( 0 , PIXELCOLOR_PRIMARY_HALF , 0  )

#define PIXEL_COLOR_FULL_RED pixelColor_t(  PIXELCOLOR_PRIMARY_FULL , 0  , 0 )
#define PIXEL_COLOR_HALF_RED pixelColor_t(  PIXELCOLOR_PRIMARY_HALF , 0  , 0 )

#define PIXEL_COLOR_FULL_BLUE pixelColor_t( 0 , 0 , PIXELCOLOR_PRIMARY_FULL  )

#define PIXEL_COLOR_OFF pixelColor_t()

inline pixelColor_t::pixelColor_t() {

    // Faster than setting the individual elements?
    // We don't need to do this because in bss this will get cleared to 0 anyway.
    // as_uint16 = 0;
    
}    

// We need these struct gymnastics because C fixed array typedefs do not work
// as you (I?) think they would...
// https://stackoverflow.com/questions/4523497/typedef-fixed-length-array

struct blinkbios_pixelblock_t {

    // RGB pixel buffer
    // Call displayPixelBuffer() to decode and copy this into the raw pixel buffer
    // so it will appear on the LEDs

    BIOS_VOLATILE pixelColor_t pixelBuffer[PIXEL_COUNT];

    // This is cleared when we are done displaying the current buffer values and about to reset and start again
    // Once this is cleared, you have one pixel interrupt period to get your new data into the pixel buffer
    // before the next refresh cycle starts.

    // We picked to clear this rather than set to 1 because in the ISR is is faster to set to 0 since R1 is always loaded with 0

    BOTH_VOLATILE uint8_t vertical_blanking_interval;
    
    // Below is some global state that is probably not interesting to user code

    rawpixel_t rawpixels[PIXEL_COUNT];

    uint8_t currentPixelIndex;  // Which pixel are we currently lighting? Pixels are multiplexed and only refreshed one at a time in sequence.
    uint8_t phase;              // Phase up updating the current pixel. There are 5 phases that include lighting each color, charging the charge pump, and resting the charge pump.
    
    
    // Here we keep the value of TCNT0 that was captured at the last watchdog timer interrupt
    // We put this here because the pixel code enables and sets up Timer0, and really not other good place for it.
    // User code is responsible for enabling the WDT interrupt and then checking this value to see
    // when it gets set asynchronously by the bios WDT ISR. 
    
    volatile uint8_t capturedEntropy;
    
    // Setting this to 1 will skip the low battery check and let the blink run down until the chip 
    // dies. This will happen about 2.4V at the standard 8Mhz speed, but you can switch the prescaller to 4Mhz
    // and run all the way down to 1.8V, but knw that IR timing (inter alia) will be hosed. 
    
    // (We really only have this so that the low battery display can re-use the pixel code and not be 
    // re-entrant since otherwise the pixel ISR code ends up calling the low battery check code)
    
    volatile uint8_t skip_low_voltage_check;

    // For future use?    
    uint8_t slack[8];

};

extern blinkbios_pixelblock_t blinkbios_pixel_block;        // Currently being displayed. You can have direct access to this to save memory,
                                                                     // but use the vertical retrace to avoid visual tearing on updates

#endif /* BLINKBIOS_PIXEL_BLOCK_H_ */