/*
 * Defines a very coarse color scheme that uses only one byte, so only 4 levels for each R,G, and B.
 * Use where memory counts like in blinkboot.
 *
 */

#ifndef COARSEPIXELCOLOR_H_
#define COARSEPIXELCOLOR_H_

// Each pixel has 4 brightness levels for each of the three colors (red,green,blue)
// These brightness levels are normalized to be visually linear with 0=off and 3=max brightness


union coarsePixelColor_t {

    struct {
        uint8_t reserved:1;
        uint8_t r:2;
        uint8_t g:2;
        uint8_t b:2;
    };

    coarsePixelColor_t();
    coarsePixelColor_t(uint8_t r_in , uint8_t g_in, uint8_t b_in );
    coarsePixelColor_t(uint8_t r_in , uint8_t g_in, uint8_t b_in , uint8_t reserverd_in );

};

inline coarsePixelColor_t::coarsePixelColor_t(uint8_t r_in , uint8_t g_in, uint8_t b_in ) {

    r=r_in;
    g=g_in;
    b=b_in;

}

inline coarsePixelColor_t::coarsePixelColor_t(uint8_t r_in , uint8_t g_in, uint8_t b_in , uint8_t reserverd_in ) {

    r=r_in;
    g=g_in;
    b=b_in;
    reserved = reserverd_in;

}


inline coarsePixelColor_t::coarsePixelColor_t() {

    r=0;
    g=0;
    b=0;

}

#define COARSE_BLACK    coarsePixelColor_t( 0 , 0 , 0 )
#define COARSE_OFF      coarsePixelColor_t( 0 , 0 , 0 )
#define COARSE_RED      coarsePixelColor_t( 3 , 0 , 0 )
#define COARSE_GREEN    coarsePixelColor_t( 0 , 3 , 0 )
#define COARSE_DIMGREEN coarsePixelColor_t( 0 , 1 , 0 )
#define COARSE_BLUE     coarsePixelColor_t( 0 , 0 , 3 )
#define COARSE_ORANGE   coarsePixelColor_t( 3 , 2 , 0 )
#define COARSE_YELLOW   coarsePixelColor_t( 3 , 3 , 0 )
#define COARSE_CYAN     coarsePixelColor_t( 0 , 3 , 3 )
#define COARSE_MAGENTA  coarsePixelColor_t( 3 , 0 , 3 )


void setRawPixelCoarse( uint8_t face , coarsePixelColor_t color );

void setAllRawCorsePixels( coarsePixelColor_t color );



#endif /* RGB_PIXELS_H_ */