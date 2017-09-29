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
// Empirically determined. At 50ms, I can click twice fast enough
// that the second click it gets in the debounce. At 20ms, I do not 
// any bounce even when I sllllooooowwwwly click. 
#define BUTTON_DEBOUNCE_MS 20

// Delay for determining clicks
// So, a click or double click will not be registered until this timeout
// because we don't know yet if it is a single, double, or triple

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

static volatile bool buttonState;                           // 0=up, 1=down
static volatile uint32_t buttonChangeEndTime=0;             // When bouncing for last state change is finished

static volatile byte clickCount=0;                       // Number of clicks in current click window 
static volatile uint32_t button_clickend_time=0;         // Time when current click timeout will end


static volatile bool buttonPressedFlag=0;               // Has the button been pressed since the last time we checked it?
static volatile bool buttonLiftedFlag=0;                // Has the button been lifted since the last time we checked it?

static volatile bool singleClickedFlag=0;               // single click since the last time we checked it?
static volatile bool doubleClickedFlag=0;               // double click since the last time we checked it?
static volatile bool multiClickedFlag=0;                // multi click since the last time we checked it?


// Tally how many clicks we got in the last click cycle and 
// include what kind of click it was into the appropriate flag,
// then reset to be ready for next window.

// Do not need to atomically access these since this can only be called 
// after the window is over, so will not update again until we clear it. 

static void tallyPendingClicks(unsigned long now) {
    
   if ( now >= button_clickend_time ) {        // Most recent cycle already over?    
       
       if (!buttonState) {                     // Only register clicks if the button is up by now. 
                                               // If the button is still down, then it is an aborted click
                                               // so throw it away
    
            switch (clickCount) {
                case 1: singleClickedFlag=1; break;
                case 2: doubleClickedFlag=1; break;
                case 3: multiClickedFlag=1; break;
            }        
            
        }            
    
        clickCount=0;
    }           
}   


// This is called asynchronously by the HAL anytime the button changes state

void button_callback_onChange(void) {
    
    // Grab a snapshot of the current state since it can change asynchronously 
    bool buttonPressedNow=button_down();
    unsigned long now=millis();    
        
    if (buttonPressedNow != buttonState ) {          // New state
        
        if (buttonChangeEndTime <= now ) {       // It has been a while since last change, so not bouncing
            
            // Ok, this is a real change 
            
            buttonState = buttonPressedNow;        
            
            buttonChangeEndTime = now + BUTTON_DEBOUNCE_MS;
            
            if (buttonState) {                  // New state, and the new state is down?
                
                buttonPressedFlag=1;                
                
                tallyPendingClicks(now);                          // Tally any pending clicks 
                                                                            
                button_clickend_time = now + BUTTON_CLICK_TIMEOUT_MS;   // Start click window over again
                                                
            } else {        // There was a change, but a lifting rather than pressing
                
                buttonLiftedFlag = 1;                

                if ( now <= button_clickend_time ) {        
                    
                    // Button lifted inside window, so count this click
                    
                    if (clickCount<3) clickCount++;         // Count this click. Don't bother counting more than 3 clicks (needed also so we never overflow, could also do with an AND 3)
                    
                } else {                    
                    
                    // We are lifting now, so that means that the trailing click in the last cycle took too long
                    // so we ignore any clicks in that cycle. 
                    // This is so that you can start a click cycle, but then change you mind and hold down to cancel. 
                    
                    clickCount=0;
                    
                }
                
                // TODO: Check long press?
                                    
            }                          
        } 
    }        
                
}  

// Debounced view of button state

bool buttonDown(void) {
      
    return buttonState;
    
}



/*

The flags can get updated asynchronously by the callback,
so we must use an atomic test and clear to avoid races.

Glorified macro to test and clear a flag. Less typing and more readable what it happening. 
This should really be built into atomic.h.

Compiles neatly to...

    240:	f8 94       	cli
    242:	80 91 00 01 	lds	r24, 0x0100	; 0x800100 <__data_end>
    246:	10 92 00 01 	sts	0x0100, r1	; 0x800100 <__data_end>
    24a:	78 94       	sei
    24c:	08 95       	ret

*/


static bool testAndClear(volatile bool &flag ) {
    
    bool c;
    
    DO_ATOMICALLY {
        c = flag;
        flag = 0;
    };
    
    return c;
}

// For click check functions will...
// Check to see if there is an already expired click cycle
// and if so, will tally any pending clicks.
// Then tests the desired flag and resets it. 
// This is lazy evaluation so we do not need 
// an extra timer to detect the end of a click cycle. 

static bool timeoutAndTally(volatile bool &flag ) {
    
    bool c;
    
    DO_ATOMICALLY {
        unsigned long now = millis();
        tallyPendingClicks(now);        
        c = flag;
        flag = 0;
    };
    
    return c;
}


bool buttonPressed(void) {        
   return testAndClear(buttonPressedFlag);    
}

bool buttonLifted(void) {
   return testAndClear( buttonLiftedFlag );
}


bool buttonSingleClicked(void) {
    return timeoutAndTally( singleClickedFlag );
}


bool buttonDoubleClicked(void) {
    return timeoutAndTally( doubleClickedFlag);
}

bool buttonMultiClicked(void) {
    return timeoutAndTally( multiClickedFlag);
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
