/*
 * pixel.h
 *
 * All the functions for showing colors on the 6 RGB LEDs on the tile face.
 * Also timekeeping functions since we use the same timer as for the LEDs.
 *
 */

#ifndef RGB_PIXELCOLOR_H_
#define RGB_PIXELCOLOR_H_
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


inline pixelColor_t::pixelColor_t() {

    r=0;
    g=0;
    b=0;

}


#endif /* RGB_PIXELS_H_ */