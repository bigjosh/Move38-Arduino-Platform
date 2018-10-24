#include "pixelcolor.h"


// Update the pixel buffer.

void blinkos_pixel_bufferedSetPixel( uint8_t pixel, pixelColor_t newColor );

// Display the buffered pixels. Blocks until next frame starts.

void blinkos_pixel_displayBufferedPixels(void);
