/*
 * Defines a very coarse color scheme that uses only one byte, so only 4 levels for each R,G, and B.
 * Use where memory counts like in blinkboot.
 *
 */

#include <stdint.h>
#include "pixel.h"
#include "coarsepixelcolor.h"

// TODO: Move this to FLASH! Why is PROGMEM not working here?

const uint8_t coarseColorGamma[4] = { 255 , 218 , 111 , 0 };        // The RAW PWM value for each brightness from full on to full off
//const PROGMEM uint8_t coarseColorGamma[4] = { 255 , 218 , 111 , 0 };        // The RAW PWM value for each brightness from full on to full off

void setRawPixelCoarse( uint8_t face , coarsePixelColor_t color ) {

    rawpixel_t *rawpixel = &(displayedRawPixelSet.rawpixels[face]);

    // TODO: This could be shorter in ASM using a shift + AND to to dereconstruct the color

    //rawpixel->rawValueR = pgm_read_byte_near( coarseColorGamma[ color.r ]  );
    //rawpixel->rawValueG = pgm_read_byte_near( coarseColorGamma[ color.g ]  );
    //rawpixel->rawValueB = pgm_read_byte_near( coarseColorGamma[ color.b ]  );
    rawpixel->rawValueR = ( coarseColorGamma[ color.r ]  );
    rawpixel->rawValueG = ( coarseColorGamma[ color.g ]  );
    rawpixel->rawValueB = ( coarseColorGamma[ color.b ]  );

}

void setAllRawCorsePixels( coarsePixelColor_t color ) {
    uint8_t f= PIXEL_COUNT;
    while (f--) {
        setRawPixelCoarse( f , color );
    }
}



