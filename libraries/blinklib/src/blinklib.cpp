/*

    This library presents an API for game developers on the Move38 Blinks platform.
    More info at http://Move38.com

    This API sits on top of the hardware abstraction layer and presents a simplified representation
    of the current state of the tile and its adjacent neighbors.

    It handles all the low level protocol to communicate state with neighbors, and
    offers high level functions for setting the color and detecting button interactions.

*/

// We need this little nugget to get UINT16_T as per...
// https://stackoverflow.com/a/3233069/3152071
// ...and it must come before Arduino.h which also pulls in stdint.h

#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <avr/pgmspace.h>
#include <stddef.h>     // NULL

#include "ArduinoTypes.h"

#include "blinklib.h"
#include "blinkstate.h"			// Get the reference to beginBlinkState()

#include "pixel.h"
#include "timer.h"
#include "button.h"
#include "utils.h"
#include "power.h"

#include "ir.h"
#include "irdata.h"

#include "run.h"

#include <util//atomic.h>
#define DO_ATOMICALLY ATOMIC_BLOCK(ATOMIC_FORCEON)                  // Non-HAL code always runs with interrupts on

// IR CONSTANTS

#define STATE_BROADCAST_SPACING_MS  50           // How often do we broadcast our state to neighboring tiles?

#define STATE_ABSENSE_TIMEOUT_MS 250             // If we don't get any state received on a face for this long, we set their state to 0

// BUTTON CONSTANTS

// Debounce button pressed this much
// Empirically determined. At 50ms, I can click twice fast enough
// that the second click it gets in the debounce. At 20ms, I do not
// any bounce even when I sllllooooowwwwly click.
#define BUTTON_DEBOUNCE_MS 20

// Delay for determining clicks
// So, a click or double click will not be registered until this timeout
// because we don't know yet if it is a single, double, or triple

#define BUTTON_CLICK_TIMEOUT_MS 330

#define BUTTON_LONGPRESS_TIME_MS 2000          // How long you must hold button down to register a long press.

#define SLEEP_TIMEOUT_SECONDS (10*60)          // If no button press in this long then goto sleep

#define SLEEP_TIMEOUT_MS ( (unsigned long) SLEEP_TIMEOUT_SECONDS  * MILLIS_PER_SECOND) // Must cast up becuase otherwise will overflow


// When we should fall sleep from inactivity
Timer sleepTimer;


//#define BUTTON_SLEEP_TIMEOUT_SECONDS (10*60)   // If no button press in this long then goto sleep

// PIXEL FUNCTIONS

// Remeber that thge underlying pixel_* setting functions are double buffered and so
// require a call to pixel_displayBuffer() to aactually show all the updates. We do this
// this call everytime loop() returns to ensure a coherent update, and also make sure that
// sure that the final result of any loop() interation will always hit the display for at least
// one frame to eliminate aliasing and tearing.


void setColorOnFace( Color newColor , byte face ) {

    pixelColor_t newPixelColor;

    // TODO: OMG, this is the most inefficient conversion from a unit16 back to (the same) unit16 ever!
    // But to share a type between the core and blinklib level though pixel.h would require all blinklib
    // users to get the whole pixel.h namespace. There has to be a good way around this. Maybe
    // break out the pixelColor type into its own core .H file? seems wrong. Hmmm....

    newPixelColor.r = GET_5BIT_R( newColor );
    newPixelColor.g = GET_5BIT_G( newColor );
    newPixelColor.b = GET_5BIT_B( newColor );

    pixel_bufferedSetPixel( face , newPixelColor );

}

void setFaceColor( byte face , Color newColor ) {
    setColorOnFace( newColor , face );
}

// Convenience function to set all pixels to the same color.

void setColor( Color newColor ) {

    FOREACH_FACE(f) {
        setColorOnFace( newColor, f );
    }

}

// This maps 0-255 values to 0-31 values with the special case that 0 (in 0-255) is the only value that maps to 0 (in 0-31)
// This leads to some slight non-linearity since there are not a uniform integral number of 1-255 values
// to map to each of the 1-31 values.

byte map8bitTo5bit( byte b ) {

    if (b==0) return 0;

    // 0 gets a special case of `off`, so we divide the rest of the range in to
    // 31 equaly spaced regions to assign the remaing 31 brightness levels.

    uint16_t normalizedB = b-1;     // Offset to a value 0-254 that will be scaled to the remaining 31 on values

    //uint16_t scaledB = (normalizedB / 255) * 31); // This is what we want to say, but it will underflow in integer math

    byte scaledB = ( (uint16_t) normalizedB * 31U) / 255U; // Be very careful to stay in bounds!

    // scaledB is now a number 0-30 that is (almost) lenearly scaled down from the orginal b

    return ( scaledB )+1;       // De-normalize back up to `on` values 1-31.

}

// Make a new color from RGB values. Each value can be 0-255.

Color makeColorRGB( byte red, byte green, byte blue ) {
    return MAKECOLOR_5BIT_RGB( map8bitTo5bit(red) , map8bitTo5bit(green) ,  map8bitTo5bit(blue) );
}

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

    return( makeColorRGB( r , g  , b ) );
}

// OMG, the Ardiuno rand() function is just a mod! We at least want a uniform distibution.

// Here we implement the SimpleRNG pseudo-random number generator based on this code...
// https://www.johndcook.com/SimpleRNG.cpp

#define GETNEXTRANDUINT_MAX UINT16_MAX

static uint16_t GetNextRandUint(void) {

    // These values are not magical, just the default values Marsaglia used.
    // Any unit should work.

    // We make them local static so that we only consume the storage if the random()
    // functions are actually ever called.

    static uint32_t u = 521288629UL;
    static uint32_t v = 362436069UL;

    v = 36969*(v & 65535) + (v >> 16);
    u = 18000*(u & 65535) + (u >> 16);

    return (v << 16) + u;

}

// return a random number between 0 and limit inclusive.
// TODO: Use entropy from the button or decaying IR LEDs
// https://stackoverflow.com/a/2999130/3152071

uint16_t rand( uint16_t limit ) {

    uint16_t divisor = GETNEXTRANDUINT_MAX/(limit+1);
    uint16_t retval;

    do {
        retval = GetNextRandUint() / divisor;
    } while (retval > limit);

    return retval;
}

// Returns the number of millis since last call
// Handy for profiling.

uint32_t timeDelta(void) {
    static uint32_t lastcall=0;
    uint32_t now = millis();
    uint32_t delta = now - lastcall;
    lastcall = now;
    return delta;
}

// Read the unique serial number for this blink tile
// There are 9 bytes in all, so n can be 0-8


byte getSerialNumberByte( byte n ) {

    if (n>8) return(0);

    return utils_serialno()->bytes[n];

}


/** IR Functions **/


// // These all can be updated by callback, so must be volatile.


// Click Semantics
// ===============
// All clicks happen inside a click window, as defined by BUTTON_CLICK_TIMEOUT_MS
// The window resets each time the debounced button state goes down.
// Any subsequent down events before the window expires are part of the same click cycle.
// If a cycle ends with the button up, then the clicks are tallied.
// If the cycle ends with the button down, then we interpret this that the user wanted to
// abort the clicks, so we discard the count.

static volatile byte buttonState=0;                     // Current debounced state

static volatile byte buttonPressedFlag=0;               // Has the button been pressed since the last time we checked it?
static volatile byte buttonReleasedFlag=0;              // Has the button been lifted since the last time we checked it?

static uint8_t buttonDebounceCountdown=0;               // How long until we are done bouncing. Only touched in the callback
                                                        // Set to BUTTON_DEBOUNCE_MS every time we see a change, then we ignore everything
                                                        // until it gets to 0 again

static uint16_t clickWindowCountdown=0;                 // How long until we close the current click window. 0=done TODO: Make this 8bit by reducing scan rate.
static uint8_t clickPendingcount=0;                     // How many clicks so far int he current click window

static uint16_t longPressCountdown=0;                   // How long until the current press becomes a long press

static volatile byte singleClickedFlag=0;               // single click since the last time we checked it?
static volatile byte doubleClickedFlag=0;               // double click since the last time we checked it?
static volatile byte multiClickedFlag=0;                // multi click since the last time we checked it?

static volatile byte longPressFlag=0;                   // Has the button been long pressed since the last time we checked it?

static volatile uint8_t maxCompletedClickCount=0;       // Remember the most completed clicks to support the clickCount() function


static volatile uint8_t buttonChangeFlag = 1;           // Set anytime the button changes state. Used to reset the sleep timer in the foreground.
                                                        // Init to 1 so that we reset the sleep timer on startup.

// Called once per tick by the timer to check the button position
// and update the button state variables.

// Note: this runs in Callback context in the timercallback
// Called every 1ms


static void updateButtonState1ms(void) {

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

            // We can nestle the click window countdown in here because a click will ALWAYS happen inside a long press...

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

            buttonChangeFlag = 1;               // Signal to foreground that something happened on the button

            buttonState = buttonPositon;

            if (buttonPositon) {
                buttonPressedFlag=1;

                if (clickPendingcount<255) {        // Don't overflow
                    clickPendingcount++;
                }

                clickWindowCountdown = BUTTON_CLICK_TIMEOUT_MS ;
                longPressCountdown   = BUTTON_LONGPRESS_TIME_MS;

            } else {
                buttonReleasedFlag=1;
            }
        }

        // Restart the countdown anytime the button position changes

        buttonDebounceCountdown = BUTTON_DEBOUNCE_MS;

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

bool testAndClearFlag( volatile byte &flag ) {

if ( flag ) {
    flag=false;
    return true;
}

return false;

}


bool buttonPressed(void) {
    return testAndClearFlag ( buttonPressedFlag );
}


bool buttonReleased(void) {
    // This test does not need to be atomic. No race
    // because we only clear here, and only set in ISR
    return testAndClearFlag ( buttonReleasedFlag );
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


// Will overflow after about 62 days...
// https://www.google.com/search?q=(2%5E31)*2.5ms&rlz=1C1CYCW_enUS687US687&oq=(2%5E31)*2.5ms

static volatile uint32_t millisCounter=1;           // How many milliseconds since startup?
                                                    // We begin at 1 so that the comparison `0 < mills() `
                                                    // will always be true so we can use `0` time to semantically
                                                    // mean `always in the past.

// Overflows after about 60 days
// Note that resolution is limited by timer tick rate

// TODO: Clear out millis to zero on wake

// This snapshot makes sure that we always see the same value for millis() in a given iteration of loop()
// This "freeze-time" view makes it harder to have race conditions when millis() changes while you are looking at it.
// The value of millis_snapshot gets reset to 0 after each loop() iteration.

static uint32_t millis_snapshot;

// Need not be atomic since never called from the background.

static void updateMillis(void) {

	millis_snapshot = millisCounter;

}

unsigned long millis(void) {
    return( millis_snapshot );
}

// Note we directlyt access millis() here, which is really bad style.
// The timer should capture millis() in a closure, but no good way to
// do that in C++ that is not verbose and inefficient, so here we are.

bool Timer::isExpired() {
	return millis() >= m_expireTime;
}

void Timer::set( uint32_t ms ) {
	m_expireTime= millis()+ms;
}

uint32_t Timer::getRemaining() {

  uint32_t timeRemaining;

  if( millis() >= m_expireTime) {

    timeRemaining = 0;

  } else {

    timeRemaining = m_expireTime - millis();

  }

  return timeRemaining;

}

void Timer::add( uint16_t ms ) {

    // Check to avoid overflow

    uint32_t timeLeft = NEVER - m_expireTime;

    if (ms > timeLeft ) {

        m_expireTime = NEVER;

    } else {

	    m_expireTime+= ms;

    }
}

void Timer::never(void) {
    m_expireTime=NEVER;
}



// TODO: This is accurate and correct, but so inefficient.
// We can do better.

// TODO: chance timer to 500us so increment is faster.
// TODO: Change time to uint24 _t

// Supported!!! https://gcc.gnu.org/wiki/avr-gcc#types

// __uint24 timer24;

#define BLINKCORE_UINT16_MAX (0xffff)               // I can not get stdint.h to work even with #define __STDC_LIMIT_MACROS, so have to resort to this hack.


// Note this runs in callback context

static void incrementMillis1ms(void) {

    millisCounter++;

}

// Have we woken since last time we checked?

static volatile uint8_t wokeFlag=0;

// Returns 1 if we have slept and woken since last time we checked
// Best to check as last test at the end of loop() so you can
// avoid intermediate display upon waking.

uint8_t hasWoken(void) {

    if (wokeFlag) {
        wokeFlag=0;
        return 1;
    }

    return 0;

}

// Turn off everything and goto to sleep

static void sleep(void) {

    pixel_disable();        // Turn off pixels so battery drain
    ir_disable();           // TODO: Wake on pixel
    button_ISR_on();        // Enable the button interrupt so it can wake us

    power_sleep();          // Go into low power sleep. Only a button change interrupt can wake us

    button_ISR_off();       // Set everything back to thew way it was before we slept
    ir_enable();
    pixel_enable();

    wokeFlag = 1;

}

// Called about once every 1ms

#include "sp.h"

void timer_1000us_callback_sei(void) {

    incrementMillis1ms();
    updateButtonState1ms();

}

// This is called by timer ISR about every 512us with interrupts on.

void timer_256us_callback_sei(void) {

    // Decrementing slightly more efficient because we save a compare.
    static unsigned step_us=0;

    step_us += 256;                     // 256us between calls

    if ( step_us>= 1000 ) {             // 1000us in a 1ms
        timer_1000us_callback_sei();
        step_us-=1000;
    }
}

// This is called by timer ISR about every 256us with interrupts on.

void timer_128us_callback_sei(void) {
    IrDataPeriodicUpdateComs();
}

// This is the entry point where the blinkcore platform will pass control to
// us after initial power-up is complete

// We make this weak so that a game can override and take over before we initialize all the hier level stuff

void __attribute__ ((weak)) run(void) {

	// Let blinkstate sink its hooks in
	blinkStateSetup();

    // Call user setup code
    setup();

    while (1) {

        updateMillis();                     // The millis() function only offers a snapshot so that
                                            // we always get the same value no matter when we look inside
                                            // a single loop iteration.


        loop();

        pixel_displayBufferedPixels();      // show all display updates that happened in last loop()
                                            // Also currently blocks until new frame actually starts

        blinkStateLoop();                 // Process any received IR messages.

        if (buttonChangeFlag) {

            buttonChangeFlag = 0;

            sleepTimer.set( SLEEP_TIMEOUT_MS );

        }

        if (sleepTimer.isExpired()) {
            sleep();

        }

        // He we could add a delay rather than call loop() as quickly as possible. This could lower power.

    }

}
