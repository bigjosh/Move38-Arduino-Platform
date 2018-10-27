/*
 * pixel.h
 *
 * All the functions for showing colors on the 6 RGB LEDs on the tile face.
 * Also timekeeping functions since we use the same timer as for the LEDs.
 *
 */

#ifndef RGB_PIXELS_H_
#define RGB_PIXELS_H_



// True today, but could imagine a blinks with 1 pixel or one with with 18 pixels...

#define PIXEL_COUNT 6

// Setup pins, interrupts. Call once at power up.

void pixel_init(void);

// Enable pixels after a call to pixel_init or pixel_disable
// Pixels will return to the color they had before being disabled.
// Call pixel_setPixels() first to set initial values of pixels before 1st call to pixel_enable()
void pixel_enable(void);

// Turn of all pixels and the timer that drives them.
// You'd want to do this before going to sleep.

void pixel_disable(void);

/** Display interface ***/

// Here are the raw compare register values for each pixel
// These are precomputed from brightness values because we read them often from inside an ISR
// Note that for red & green, 255 corresponds to OFF and 0 is full on

struct  rawpixel_t {
    uint8_t rawValueR;
    uint8_t rawValueG;
    uint8_t rawValueB;
    //uint8_t paddng;           // Adding this padding save about 10 bytes since double-doubling is faster than adding 3. 
                                // Would be even better to precache the pointer to the next pixel, but we also need an index
                                // to turn off the anode, so benefits lost. Wish there was a good way to turn off the cathode 
                                // based on the pointer... Could compare the bottom byte of the pointer to known list, but so hacky.
                                
    rawpixel_t( uint8_t in_raw_r , uint8_t in_raw_g , uint8_t in_raw_b ) {
        rawValueR =  in_raw_r;        
        rawValueG = in_raw_g;
        rawValueB = in_raw_b;
        
    }        
        
    rawpixel_t() {
        
        // We initialize to all 0's so that the buffer can be placed in the uninizialzed variable pool
        // if we preinitialized to all 0xff (which would be nice), then it would use up flash
        // so instead we provide the pixel_init_rawpixelset() function to efficiently initialize the whole set of 
        // pixels to 0xff.
        //rawValueR = 0xff;
        //rawValueG = 0xff;
        //rawValueB = 0xff;        
    }
                                                
};


const rawpixel_t RAW_PIXEL_OFF( 0xff , 0xff, 0xff );

// We need these struct gymnastics because C fixed array typedefs do not work
// as you (I?) think they would...
// https://stackoverflow.com/questions/4523497/typedef-fixed-length-array

typedef struct {
    rawpixel_t rawpixels[PIXEL_COUNT];
} rawpixelset_t;



extern rawpixelset_t displayedRawPixelSet;        // Currently being displayed. You can have direct access to this to save memory, 
                                                                                    // but use the vertical retrace to avoid visual tearing on updates
                                                  
                                           

extern rawpixelset_t displayedRawPixelSet; 
                                                  
// This is cleared when we are done displaying the current buffer values and about to reset and start again
// Once this is cleared, you have one pixel interrupt period to get your new data into the pixel buffer
// before the next refresh cycle starts.

// We picked to clear this rather than set to 1 because in the ISR is is faster to set to 0 since R1 is always loaded with 0

extern volatile uint8_t vertical_blanking_interval;



#endif /* RGB_PIXELS_H_ */