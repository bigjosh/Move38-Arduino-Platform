/*

    This library presents an API for game developers on the Move38 Blinks platform.
    More info at http://Move38.com
    
    This API sits on top of the hardware abstraction layer and presents a simplified representation
    of the current state of the tile and its adjacent neighbors. 
    
    It handles all the low level protocol to communicate state with neighbors, and
    offers high level functions for setting the color and detecting button interactions. 

*/

#include <avr/pgmspace.h>
#include <limits.h>
#include <stdint.h>

#include <Arduino.h>

#define DEBUG_MODE


#include "blinklib.h"

#include "pixel.h"
#include "timer.h"
#include "button.h"
#include "utils.h"

#include "debug.h"

#include "irdata.h"


#include "debug.h"

// Debounce button pressed this much
// Empirically determined. At 50ms, I can click twice fast enough
// that the second click it gets in the debounce. At 20ms, I do not 
// any bounce even when I sllllooooowwwwly click. 
#define BUTTON_DEBOUNCE_MS 20

// Delay for determining clicks
// So, a click or double click will not be registered until this timeout
// because we don't know yet if it is a single, double, or triple

#define BUTTON_CLICK_TIMEOUT_MS 330

#define BUTTON_LONGPRESS_TIME_MS 2000       // How long you must hold button down to register a long press. 


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


// // These all can be updated by callback, so must be volatile.




// Click Semantics
// ===============
// All clicks happen inside a click window, as defined by BUTTON_CLICK_TIMEOUT_MS
// The window resets each time the debounced button state goes down. 
// Any subsequent down events before the window expires are part of the same click cycle.
// If a cycle ends with the button up, then the clicks are tallied.
// If the cycle ends with the button down, then we interpret this that the user wanted to 
// abort the clicks, so we discard the count. 

static volatile bool buttonState=0;                     // Current debounced state

static volatile bool buttonPressedFlag=0;               // Has the button been pressed since the last time we checked it?
static volatile bool buttonLiftedFlag=0;                // Has the button been lifted since the last time we checked it?

static uint8_t buttonDebounceCountdown=0;               // How long until we are done bouncing. Only touched in the callback

static uint16_t clickWindowCountdown=0;                 // How long until we close the current click window. 0=done TODO: Make this 8bit by reducing scan rate. 
static uint8_t clickPendingcount=0;                     // How many clicks so far int he current click window

static uint16_t longPressCountdown=0;                   // How long until the current press becomes a long press

static volatile bool singleClickedFlag=0;               // single click since the last time we checked it?
static volatile bool doubleClickedFlag=0;               // double click since the last time we checked it?
static volatile bool multiClickedFlag=0;                // multi click since the last time we checked it?

static volatile bool longPressFlag=0;                   // Has the button been long pressed since the last time we checked it?

static volatile uint8_t maxCompletedClickCount=0;       // Remember the most completed clicks to support the clickCount() function


// Called once per tick by the timer to check the button position
// and update the button state variables.

static void updateButtonState(void) {
    
       
    bool buttonPositon = button_down();
           
    if ( buttonPositon == buttonState ) {
        
        if (buttonDebounceCountdown) {
            
            buttonDebounceCountdown--;
            
        }  
        
        if (longPressCountdown) {
            
            longPressCountdown--;
            
            if (longPressCountdown==0) {
                
                if (buttonState) {
                    
                    longPressFlag = 1;
                }
            }                                           
            
            // We can nestle thew click window countdown in here because a click will ALWAYS happen inside a long press...
        
            if (clickWindowCountdown) {
            
                clickWindowCountdown--;
            
                if (clickWindowCountdown==0) {      // Click window just expired
                
                    if (!buttonState) {              // Button is up, so register clicks 
                    
                        if (clickPendingcount==1) {
                            singleClickedFlag=1;
                        } else if (clickPendingcount==2) {
                            doubleClickedFlag=1;
                        } else {
                            multiClickedFlag=1;
                        }                                                                        
                    
                    
                        if (clickPendingcount > maxCompletedClickCount ) {
                            maxCompletedClickCount=clickPendingcount;   
                        }                            
                    
                    
                    
                    }                                        
                
                    clickPendingcount=0;        // Start next cycle (aborts any pending clicks if button was still down
                
                
                }                            
            
            }            
        
        }                    
            
        
    }  else {       // New button position
        
        if (!buttonDebounceCountdown) {         // Done bouncing
            
            buttonState = buttonPositon;        
            
            if (buttonPositon) {
                buttonPressedFlag=1;
                                    
                if (clickPendingcount<255) {        // Don't overflow
                    clickPendingcount++;
                }                        
                
                clickWindowCountdown = TIMER_MS_TO_TICKS( BUTTON_CLICK_TIMEOUT_MS );
                longPressCountdown   = TIMER_MS_TO_TICKS( BUTTON_LONGPRESS_TIME_MS ); 
                
            } else {
                buttonLiftedFlag=1;                
            }                                            
        }
        
        // Restart the countdown anytime the button position changes
        
        buttonDebounceCountdown = TIMER_MS_TO_TICKS( BUTTON_DEBOUNCE_MS );
        
    }        
                        
                        
    
}


// Debounced view of button state

bool buttonDown(void) {
          
    return buttonState;
    
}


// This test does not need to be atomic. No race
// because we only clear here, and only set in ISR.

// This compiles quite nicely...

/*
;if ( flag ) {
552:	80 91 15 01 	lds	r24, 0x0115	; 0x800115 <buttonPressedFlag>
556:	81 11       	cpse	r24, r1
;flag=false;
558:	10 92 15 01 	sts	0x0115, r1	; 0x800115 <buttonPressedFlag>
55c:	08 95       	ret
*/

bool testAndClearFlag( volatile bool &flag ) {

if ( flag ) {
    flag=false;
    return true;
}

return false;    
    
}    


bool buttonPressed(void) {     
    return testAndClearFlag ( buttonPressedFlag );
}            
          

bool buttonLifted(void) {
    // This test does not need to be atomic. No race
    // because we only clear here, and only set in ISR
    return testAndClearFlag ( buttonLiftedFlag );
}    
    

bool buttonSingleClicked(void) {
    return testAndClearFlag( singleClickedFlag );
}


bool buttonDoubleClicked(void) {
    return testAndClearFlag( doubleClickedFlag);
}

bool buttonMultiClicked(void) {
    return testAndClearFlag( multiClickedFlag );
}

bool buttonLongPressed(void) {
    return testAndClearFlag( longPressFlag );
}


uint8_t buttonClickCount(void) {
    
    uint8_t t;
    
    // Race here - if a new (and higher count) click expired exactly between the next two lines,
    // we would only read the previous highest count, and then discard the new highest when we set to 0.
    // That's why we need ATOMICALLY here. 
    
    DO_ATOMICALLY {
        t=maxCompletedClickCount;
        maxCompletedClickCount=0;
    }        
    
    return t;
}    

void button_callback_onChange(void) {
    // Empty since we do not need to do anything in the interrupt with the above timer sampling scheme. 
    // Nice because bounces can come fast and furious.     
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
// Note that resolution is limited by timer tick rate


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


// TODO: This is accurate and correct, but so inefficient. 
// We can do better. 

// TODO: chance timer to 500us so increment is faster.
// TODO: Change time to uint24 _t

// Supported!!! https://gcc.gnu.org/wiki/avr-gcc#types

// __uint24 timer24;

static uint16_t cyclesCounter=0;                    // Accumulate cycles to keep millisCounter accurate

#if TIMER_CYCLES_PER_TICK >  0xffff //UINT16_MAX
    #error Overflow on cyclesCounter
#endif

static void updateMillis(void) {
    
    cyclesCounter+=TIMER_CYCLES_PER_TICK;    // Used for timekeeping. Nice to increment here since this is the least time consuming phase
    
    while (cyclesCounter >= CYCLES_PER_MS  ) {      // While covers cases where TIMER_CYCLES_PER_TICK >= CYCLES_PER_MILLISECOND * 2 
        
        millisCounter++;
        cyclesCounter-=CYCLES_PER_MS ;
        
    }    
                       
    // Note that we might have some cycles left. They will accumulate in cyclescounter and eventually get folded into a full milli to
    // avoid errors building up.
                           
}    

// This is called by timer2 about every 512us
// TODO: Reduce this rate by phasing the timer call?

void timer_callback(void) {
    updateIRComs(); 
    updateMillis();            
    updateButtonState();
}    

// This is the entry point where the platform will pass control to 
// us after initial power-up is complete

void run(void) {
    
    //while (1) DEBUGC_PULSE(20);
    
    setup();
    
    while (1) {
     
        loop();
           
    }        
    
}    


