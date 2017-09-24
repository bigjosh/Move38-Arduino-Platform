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

// Debounce button pressed this much
#define BUTTON_DEBOUNCE_MS 50

// Delay for determining clicks
// So, a click or double click will not be registered until this timeout
// because we don't know yet if it is a single, double, or tripple

#define BUTTON_CLICK_TIMEOUT_MS 330


void setColor( Color newColor ) {
    
    // Adjust the 0-31 scale from the blinks color model to the internal 0-255 scale
    // TODO: Should internal model be only 5 bits also?  Would save space in the gamma lookup table. I bet if we get the gamma values right that you can't see more than 5 bits anyway. 
    
    pixel_SetAllRGB( (( newColor >> 10 ) & 31 ) << 3 , (( newColor >> 5 ) & 31 ) << 3 , (( newColor  ) & 31  ) << 3 );
    
}    


void setFaceColor( byte face , Color newColor ) {
    
    // Adjust the 0-31 scale from the blinks color model to the internal 0-255 scale
    // TODO: Should internal model be only 5 bits also?  Would save space in the gamma lookup table. I bet if we get the gamma values right that you can't see more than 5 bits anyway.
    
    pixel_setRGB( face , (( newColor >> 10 ) & 31 ) << 3 , (( newColor >> 5 ) & 31 ) << 3 , (( newColor  ) & 31  ) << 3 );
    
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

// TODO: Use millis timer to do this
    
void delay( unsigned long millis ) {
    delay_ms( millis );
}    


// Read the unique serial number for this blink tile
// There are 9 bytes in all, so n can be 0-8


byte getSerialNumberByte( byte n ) {
    
    if (n>8) return(0);
    
    return utils_serialno()->bytes[n];

}


// Keep all the states in one volatile byte that is shared between forground and callback contexts

static volatile byte button_state_bits;

// This state is live - it changes asynchronously from up to down based on the debounce state of the button
#define BUTTON_STATE_DOWN               0                   // The button is currently down (debounced)

// These states are sticky. They are set in the callback and cleared when checked in the foreground 
#define BUTTON_STATE_CLICK_PENDING      1             // Waiting for timeout to declare a click
#define BUTTON_STATE_CLICKED            2             // Waiting for timeout to declare a click

#define BUTTON_STATE_DOUBLE_PENDING     3             // Waiting for timeout to declare a double click
#define BUTTON_STATE_DOUBLECLICKED      4             // A double click was detected

#define BUTTON_STATE_TRIPLE_PENDING     5             // Waiting for timeout to declare a triple click
#define BUTTON_STATE_TRIPLECLICKED      6             // A triple click was detected


// // These all can be updated by callback, so must be volatile.

static volatile bool buttonState;                       // 0=up, 1=down
static volatile uint32_t buttonChangeEndTime=0;       // When bouncing for last state change is finished
static volatile uint32_t button_clickend_time=0;         // Time when current click timeout will end


static volatile byte buttonPressCount=0;                // Has the button been pressed since the last time we checked it?

// This is called asynchronously by the HAL anytime the button changes state

void button_callback_onChange(void) {
    
    // Grab a snapshot of the current state since it can change
    bool buttonPressedNow=button_down();
    unsigned long now=millis();
    
    if (buttonPressedNow != buttonState ) {          // New state
        
        if (buttonChangeEndTime <= now ) {       // It has been a while since last change, so not bouncing
            
            // Ok, this is a real change 
            
            buttonState = buttonPressedNow;        
            buttonChangeEndTime = now + BUTTON_DEBOUNCE_MS;
            
            if (buttonState) {                  // New state, and the new state is down?
                
                // Count the new press
                
                if (buttonPressCount<255) {     // Don't overflow
                    buttonPressCount++;
                }                    
                                
            }                
            
        } else {
            
            // TODO: DO we keep pushing the debounce time out as long as we are bouncing?
            
        }
        
    }        
    
            
}  

// Debounced view of button state

bool buttonDown(void) {
      
    return buttonState;
    
}


uint8_t buttonPressedCount(void) {
    
    // the flag can get update asynchronously by the callback,
    // so must to an atomic test and clear
    
    byte c;
    
    DO_ATOMICALLY {
        c = buttonPressCount;
        buttonPressCount = 0;
    };        
        
    return c;
    
}


// TODO: Need this?

volatile uint8_t verticalRetraceFlag=0;     // Turns to 1 when we are about to start a new refresh cycle at pixel zero
// Once this turns to 1, you have about 2ms to load new values into the raw array
// to have them displayed in the next frame.
// Only matters if you want to have consistent frames and avoid visual tearing
// which might not even matter for this application at 80hz
// TODO: Is this empirically necessary?


// Will overflow after about 62 days...
// https://www.google.com/search?q=(2%5E31)*2.5ms&rlz=1C1CYCW_enUS687US687&oq=(2%5E31)*2.5ms

static volatile uint32_t millisCounter=0;           // How many millisecends since most recent pixel_enable()?
// Overflows after about 60 days
// Note that resolution is limited by pixel refresh rate

static uint16_t cyclesCounter=0;                    // Accumulate cycles to keep millisCounter accurate

// The pixel rate will likely not be an even multiple of milliseconds
// so this accumulates cycles that we than dump into millis


// TODO: Clear out millis to zero on wake

unsigned long millis(void) {
    
    uint32_t tempMillis;

    // millisCounter is 4 bytes long and is updated in the background
    // so we need an atomic block to make sure it does not change while we are reading it

    DO_ATOMICALLY {
        tempMillis=millisCounter;
    }
    return( tempMillis );
}


#define MILLIS_PER_SECOND 1000

#define CYCLES_PER_SECOND (F_CPU)

#define CYCLES_PER_MILLISECOND ( CYCLES_PER_SECOND / MILLIS_PER_SECOND )


static void updateMillis(void) {
    
    cyclesCounter+=PIXEL_CYCLES_PER_FRAME;    // Used for timekeeping. Nice to increment here since this is the least time consuming phase
    
    while (cyclesCounter >= CYCLES_PER_MILLISECOND ) {
        
        millisCounter++;
        cyclesCounter-=CYCLES_PER_MILLISECOND;
        
    }
    
                       
    // Note that we might have some cycles left. They will accumulate and eventually get folded into a full milli to
    // avoid errors building up.
                           
}    

// This is called at the end of each frame, so about 66Hz

void pixel_callback_onFrame(void) {
    updateMillis();            
}    

// This is the entry point where the platform will pass control to 
// us after initial power-up is complete

void run(void) {
    
    setup();
    
    while (1) {
     
        loop();
           
    }        
    
}    
