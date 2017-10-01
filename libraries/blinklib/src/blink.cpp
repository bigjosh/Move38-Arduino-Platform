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

static volatile bool buttonState=0;                     // Current debounced state
static volatile uint32_t buttonLastChangeTime=0;        // The last time we saw the button different from the current state
                                                        // Use for debouncing                                                                                                               

static volatile uint32_t buttonChangeEndTime=0;         // When the bouncing for last state change will be finished
static volatile uint32_t button_clickend_time=0;        // When the current click will be terminate

static volatile byte clickCount=0;                                  // Number of clicks in current click window 
//static volatile uint32_t button_longpress_end_time=ULLONG_MAX;      // Time when current click will be long click

static volatile bool buttonPressedFlag=0;               // Has the button been pressed since the last time we checked it?
static volatile bool buttonLiftedFlag=0;                // Has the button been lifted since the last time we checked it?


static volatile bool clickPending=0;                    // Is there a click action pending?
static volatile bool singleClickedFlag=0;               // single click since the last time we checked it?
static volatile bool doubleClickedFlag=0;               // double click since the last time we checked it?
static volatile bool multiClickedFlag=0;                // multi click since the last time we checked it?

static volatile bool buttonLongClickedFlag=0;           // Has the button been long clicked since the last time we checked it?

static volatile uint8_t maxCompletedClickCount=0;       // Remember the most completed clicks to support the clickCount() function


// How clicks are processed:
// Each time a press is detected, we (re)start the click timeout window.
// If a lift is detected within the timeout window, then we increment the click counter.
// If the timeout window has expired and the button is still down, then the person long pressed 
// at the end of the click cycle, which we interpret as wanting to cancel the click cycle. 
// If the timeout window has expired and the button is up, then we detect a valid click cycle
// and update all the click flags and variables. 


// Tally how many clicks we got in the last click cycle and 
// include what kind of click it was into the appropriate flag,
// then reset to be ready for next window.

// Do not need to atomically access these since this can only be called 
// after the window is over, so will not update again until we clear it.
 
// Must be called atomically

static void updateClickState(unsigned long now) {
    
   if ( now >= button_clickend_time ) {        // Most recent cycle already over?    
       
       if (!buttonState) {                     // Only register clicks if the button is up by now. 
                                               // If the button is still down, then it is an aborted click
                                               // so throw it away
    
            if (clickCount==1) singleClickedFlag=1;
            else if (clickCount==2) doubleClickedFlag=1;
            else if (clickCount>=3) multiClickedFlag=1;
            
        
            if (clickCount > maxCompletedClickCount ) {
            
                maxCompletedClickCount=clickCount;
            
            }            
            
        }            
    
        // Get ready for next click cycle (will throw away previous cycle if the button is still down)
    
        clickCount=0;
    }           
}   


/*

// Call anytime the current buttonDown condition does not match the debounced buttonState
// Must be called atomically

static void updateButtonState( const unsigned long now) {
 
    if ( now >= buttonLastChangeTime+BUTTON_DEBOUNCE_MS ) {       // It has been a while since last change, so not bouncing
            
        // Ok, this is a real, debounced change 
            
        buttonState = !buttonState;         // Invert the buttonState (we only get here if they were different)

        updateClickState(now);            // Tally any pending clicks
                        
        if (buttonState) {                  // New state, and the new state is down?
                
            buttonPressedFlag=1;          
                                                                                                       
            button_clickend_time = now + BUTTON_CLICK_TIMEOUT_MS;   // Start click window over again
                
//            button_longpress_end_time = now + BUTTON_LONGPRESS_TIME_MS; 
                
                                                
        } else {        // There was a change, but a lifting rather than pressing
                
            buttonLiftedFlag = 1;
                         
            if ( now <= button_clickend_time ) {        
                    
                // Button lifted inside window, so count this click
                    
                if (clickCount<255) clickCount++;         // Count this click.(needed also so we never overflow, could also do with an AND 3)
                    
            } 
                            
            // TODO: Check long press?
                                                    
        }           
        
    } 
    
}    

*/

// Can be called anytime to update the internal button states to account for changes
// in time and button position.

// We break this out as a separate function because there are two distinct times that the state can change.
// Things can obviously change any time the button position changes and fires the ISR
// But the state can also change spontaneously without any interrupt because the debounce window
// times out. Imagine a very short press that is shorter than the debounce window. In this case,
// the initial down ISR will correctly see the button press event, but then the subsequent up 
// ISR will happen while we are still in the debounce window, so we don't want to detect a lift event.
// The lift event happens abstractly after the debounce window expires and the button position is still up,
// but there is no corresponding ISR for this since noting in the world actually changed. We pick this up 
// by updating the internal state anytime one of the functions that checks it are called. 

// There is a hard case when a short pulse happens, so only the press event is detected (the lift happened during the bounce window).
// If then there is press that happens before one of the polling functions is called, then we will not see the intermediate lift event
// Because the button looks like it is just still down from the first press. Hmmmm..



// Must be called atomically
// Only call if buttonstate and the current button position are different or else you might get false positives. 

static void updateButtonState(unsigned long now , bool buttonPositionNow )  {
    
           
    if ( now > buttonLastChangeTime + BUTTON_DEBOUNCE_MS ) {        // Ignore any changes while bouncing
        
        // If we get here, then we are not bouncing
        
        if (buttonPositionNow) {
            
            // Button position currently down
            
            buttonState=true;
            
            if (buttonPressedFlag) {        // This is an odd case where the button was lifted during the debounce, and then pressed again without a test inbetween
                buttonLiftedFlag = true;    // This is the only way would could get here with button pressed, debounced state up, and pressedFlag true
            }
            
            buttonPressedFlag=true;            
            
        } else {  // !buttonPositionNow
            
            buttonState=false;
            
            if (buttonLiftedFlag) {
                buttonPressedFlag = true;   // This is an odd case where the button was pressed during the debounce, and then lifted again without a test inbetween
            }
            
            buttonLiftedFlag=true;
            
        }
        
    }
    
}    

// This is called asynchronously by the HAL anytime the button changes state
// Also called internally to update the state whenever the foreground checks,
// but must be run atomically from there.  

void button_callback_onChange(void) {

    // Grab a snapshot of the current state since it can change asynchronously
    unsigned long now = millis();    
    bool buttonPositionNow = button_down();

    if ( buttonPositionNow != buttonState ) {        // Only do anything if the button has changed

        updateButtonState(now, buttonPositionNow);
    
        // We only reset the bounce window if the state actually changed.
        // This covers an odd case where the button goes down and triggers an ISR, but by the time the ISR
        // runs, the button is up again so we don't change the state. If we started the window, then we would
        // ignore the next down we got in that bounce. 
    
        buttonLastChangeTime = now;         // We restart the debounce window every time the button position changes
    }        
                               
}  

static void updateButtonStateAtomically(void) {
   
    DO_ATOMICALLY {
        bool buttonPositionNow = button_down();
        
        if ( buttonPositionNow != buttonState ) {        // Only do anything if the button has changed
            updateButtonState( millis() , buttonPositionNow );
        }
    }    
}

// Debounced view of button state

bool buttonDown(void) {
    
    updateButtonStateAtomically();
      
    return buttonState;
    
}


// Check to see if the click state has changed because the click window expired. 

static void updateClickStateAtomically(void) {

    DO_ATOMICALLY {
        updateClickState( millis() );
    }
}    

bool buttonPressed(void) {     

    updateButtonStateAtomically();
    
    if ( buttonPressedFlag ) {
        buttonPressedFlag=false;
        return true;
    }

    return false;
}            
          

bool buttonLifted(void) {
    
    updateButtonStateAtomically();

    // This test does not need to be atomic. No race
    // because we only clear here, and only set in ISR
    
    
    if ( buttonLiftedFlag ) {
        buttonLiftedFlag=false;
        return true;
    }

    return false;
}

bool buttonSingleClicked(void) {
    //return timeoutAndTally( singleClickedFlag );
}


bool buttonDoubleClicked(void) {
    //return timeoutAndTally( doubleClickedFlag);
}

bool buttonMultiClicked(void) {
    //return timeoutAndTally( multiClickedFlag);
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
