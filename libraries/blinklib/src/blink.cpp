/*

    This library presents an API for game developers on the Move38 Blinks platform.
    More info at http://Move38.com
    
    This API sits on top of the hardware abstraction layer and presents a simplified representation
    of the current state of the tile and its adjacent neighbors. 
    
    It handles all the low level protocol to communicate state with neighbors, and
    offers high level functions for setting the color and detecting button interactions. 

*/


#include <avr/pgmspace.h>

#include "Arduino.h"

#include "blink.h"

#include "pixel.h"
#include "time.h"
#include "button.h"
#include "utils.h"


void setColor( Color newColor ) {
    
    // Adjust the 0-31 scale from the blinks color model to the internal 0-255 scale
    // TODO: Should internal model be only 5 bits also?  Would save space in the gamma lookup table. I bet if we get the gamma values right that you can't see more than 5 bits anyway. 
    
    pixel_SetAllRGB( (( newColor >> 10 ) & 31 ) << 3 , (( newColor >> 5 ) & 31 ) << 3 , (( newColor  ) & 31  ) << 3 );
    
}    


// makeColorRGB defined as a macro for now so we get compile time calcuation for static colors.

/* 

Color makeColorRGB( byte r, byte g, byte b ) {
    return ((r&31)<<10|(g&31)<<5|(b&31));
}  

*/  


Color makeColorHSB( uint8_t hue, uint8_t saturation, uint8_t brightness ) {

	uint8_t r;
	uint8_t g;
	uint8_t b;

	if (saturation == 0)
	{
		// achromatic (grey)
		r =g = b= brightness;
	}
	else
	{
		unsigned int scaledHue = (hue * 6);
		unsigned int sector = scaledHue >> 8; // sector 0 to 5 around the color wheel
		unsigned int offsetInSector = scaledHue - (sector << 8);  // position within the sector
		unsigned int p = (brightness * ( 255 - saturation )) >> 8;
		unsigned int q = (brightness * ( 255 - ((saturation * offsetInSector) >> 8) )) >> 8;
		unsigned int t = (brightness * ( 255 - ((saturation * ( 255 - offsetInSector )) >> 8) )) >> 8;

		switch( sector ) {
			case 0:
			r = brightness;
			g = t;
			b = p;
			break;
			case 1:
			r = q;
			g = brightness;
			b = p;
			break;
			case 2:
			r = p;
			g = brightness;
			b = t;
			break;
			case 3:
			r = p;
			g = q;
			b = brightness;
			break;
			case 4:
			r = t;
			g = p;
			b = brightness;
			break;
			default:    // case 5:
			r = brightness;
			g = p;
			b = q;
			break;
		}
	}
    
    // Bring the 0-255 range values down to 0-31 range
    
    return( makeColorRGB( r >> 3 , g >> 3  , b >> 3 ) );
}


    
// Note that we do not expose _delay_ms() to the user since then they would need
// access to F_CPU and it would also limit them to only static delay times. 
// By abstracting to a function, we can dynamically adjust F_CPU and 
// also potentially sleep during the delay to save power rather than just burning cycles.     
    
void delay( unsigned long millis ) {
    delay_ms( millis );
}    

unsigned long millis(void) {
    
    return( pixel_mills() );
    
}    


// Read the unique serial number for this blink tile
// There are 9 bytes in all, so n can be 0-8


byte getSerialNumberByte( byte n ) {
    
    if (n>8) return(0);
    
    return utils_serialno()->bytes[n];

}


bool buttonDown(void) {
    
    return button_down();
    
}    

// This is the entry point where the platform will pass control to 
// us after initial power-up is complete

void run(void) {
    
    setup();
    
    while (1) {
     
        loop();
           
    }        
    
}    
