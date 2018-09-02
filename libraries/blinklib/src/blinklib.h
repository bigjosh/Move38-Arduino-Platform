/*
 * blink.h
 *
 * This defines a high-level interface to the blinks tile hardware.
 *
 */

#ifndef BLINKLIB_H_
#define BLINKLIB_H_

//#include "blinkcore.h"

#include "ArduinoTypes.h"

#include "chainfunction.h"

#include <stdbool.h>
#include <stdint.h>


/*

	This set of functions let you test for changes in the environment.

*/

// Did the state on any face change since last called?
// Get the neighbor states with getNeighborState()

// TODO: State view not implemented yet. You can use irIsReadyOnFace() instead.

//bool neighborChanged();

// Was the button pressed or lifted since the last time we checked?
// Note that these register the change the instant the button state changes
// without any delay, so good for latency sensitive cases.
// It is debounced, so the button must have been in the previous state a minimum
// debounce time before a new detection will occur.

bool buttonPressed(void);
bool buttonReleased(void);

// Was the button single, double , or multi clicked since we last checked?
// Note that there is a delay after the button is first pressed
// before a click is registered because we have to wait to
// see if another button press is coming.
// A multiclick is 3 or more clicks

// Remember that these click events fire a short time after the button is lifted on the final click
// If the button is held down too long on the last click, then click interaction is aborted.

bool buttonSingleClicked();

bool buttonDoubleClicked();

bool buttonMultiClicked();


// The number of clicks in the longest consecutive valid click cycle since the last time called.
byte buttonClickCount(void);

// Remember that a long press fires while the button is still down
bool buttonLongPressed(void);

/*

	This set of functions lets you read the current state of the environment.

*/

// Returns true if the button currently pressed down
// (Debounced)

bool buttonDown();



/*
    IR communications functions
*/


// Send data on a single face.
// Data is 6-bits wide, top bits are ignored.

void irSendData( uint8_t face , uint8_t data   );

// Simultaneously send data on all faces that have a `1` in bitmask
// Data is 6-bits wide, top bits are ignored.
void irSendDataBitmask(uint8_t data, uint8_t bitmask);

// Broadcast data on all faces.
// Data is 6-bits wide, top bits are ignored.

void irBroadcastData( uint8_t data );

// Is there a received data ready to be read on the indicated face? Returns 0 if none.

bool irIsReadyOnFace( uint8_t face );

// Read the most recently received data. Value 0-127. Blocks if no data ready.

uint8_t irGetData( uint8_t led );


/*

	This set of functions lets you control the colors on the face RGB LEDs

*/

// Set our state to newState. This state is repeatedly broadcast to any
// neighboring tiles.
// Note that setting our state to 0 make us stop broadcasting and effectively
// disappear from the view of neighboring tiles.

// TODO: State view not implemented yet. You can use irBroadcastData() instead.

// void setState( byte newState );

// Color type holds 4 bits for each R,G,B. Top bit is currently unused.

// TODO: Do we need 5 bits of resolution for each color?
// TODO: Use top bit(s) for something useful like automatic
//       blink or twinkle or something like that.

// Argh, these macros are so ugly... but so ideomatic arduino. Maybe use a class with bitfields like
// we did in pixel.cpp just so we can sleep at night?

typedef uint16_t Color;

// Number of brightness levels in each channel of a color
#define BRIGHTNESS_LEVELS_5BIT 32
#define MAX_BRIGHTNESS_5BIT    (BRIGHTNESS_LEVELS_5BIT-1)

#define GET_5BIT_R(color) ((color>>10)&31)
#define GET_5BIT_G(color) ((color>> 5)&31)
#define GET_5BIT_B(color) ((color    )&31)

// R,G,B are all in the domain 0-31
// Here we expose the internal color representation, but it is worth it
// to get the performance and size benefits of static compilation
// Shame no way to do this right in C/C++

#define MAKECOLOR_5BIT_RGB(r,g,b) ((r&31)<<10|(g&31)<<5|(b&31))

#define RED         MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT, 0                    ,0)
#define ORANGE      MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT,MAX_BRIGHTNESS_5BIT/2 ,0)
#define YELLOW      MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT,MAX_BRIGHTNESS_5BIT   ,0)
#define GREEN       MAKECOLOR_5BIT_RGB( 0                 ,MAX_BRIGHTNESS_5BIT   ,0)
#define CYAN        MAKECOLOR_5BIT_RGB( 0                 ,MAX_BRIGHTNESS_5BIT   ,MAX_BRIGHTNESS_5BIT)
#define BLUE        MAKECOLOR_5BIT_RGB( 0                 , 0                    ,MAX_BRIGHTNESS_5BIT)
#define MAGENTA     MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT, 0                    ,MAX_BRIGHTNESS_5BIT)

#define WHITE       MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT,MAX_BRIGHTNESS_5BIT   ,MAX_BRIGHTNESS_5BIT)

#define OFF         MAKECOLOR_5BIT_RGB( 0                 , 0                    , 0)

// This maps 0-255 values to 0-31 values with the special case that 0 (in 0-255) is the only value that maps to 0 (in 0-31)
// This leads to some slight non-linearity since there are not a uniform integral number of 1-255 values
// to map to each of the 1-31 values.

// Make a new color from RGB values. Each value can be 0-255.

Color makeColorRGB( byte red, byte green, byte blue );

#define MAX_BRIGHTNESS (255)

// Dim the specified color. Brightness is 0-255 (0=off, 255=don't dim at all-keep original color)
// Inlined to allow static simplification at compile time

inline Color dim( Color color, byte brightness) {
    return MAKECOLOR_5BIT_RGB(
        (GET_5BIT_R(color)*brightness)/255,
        (GET_5BIT_G(color)*brightness)/255,
        (GET_5BIT_B(color)*brightness)/255
    );
}

// Make a new color in the HSB colorspace. All values are 0-255.

Color makeColorHSB( byte hue, byte saturation, byte brightness );

// Change the tile to the specified color
// NOTE: all color changes are double buffered
// and the display is updated when loop() returns

void setColor( Color newColor);

// Set the pixel on the specified face (0-5) to the specified color
// NOTE: all color changes are double buffered
// and the display is updated when loop() returns

void setFaceColor(  byte face, Color newColor );

void setColorOnFace( Color newColor , byte face );

/*

    Timing functions

*/

// Number of running milliseconds since power up.
//
// Important notes:
// 1) does not increment while sleeping
// 2) is only updated between loop() interations
// 3) is not monotonic, so always use greater than
//    and less than rather than equals for comparisons
// 4) overflows after about 50 days
// 5) is only accurate to about +/-10%

unsigned long millis(void);

#define NEVER ( (uint32_t)-1 )          // UINT32_MAX would be correct here, but generates a Symbol Not Found.



class Timer {

	private:

		uint32_t m_expireTime;		// When this timer will expire

	public:

		Timer() : m_expireTime(0) {};		// Timers come into this world pre-expired.

		bool isExpired();

		void set( uint32_t ms );

    uint32_t getRemaining();
};


/*

    Utility functions

*/


// Return a random number between 0 and limit inclusive.

uint16_t rand( uint16_t limit );

// Read the unique serial number for this blink tile
// There are 9 bytes in all, so n can be 0-8


byte getSerialNumberByte( byte n );


/*

    Button functions

*/


// Debounced view of button state

bool buttonDown(void);

// Returns true if the button has been pressed since
// the last time it was called.

bool buttonPressed(void);


/*

    IR communications functions

*/

// Send data on a single face. Data is 6-bits wide (0-63 value).

void irSendDataBitmask( uint8_t face , uint8_t data );

// Broadcast data on all faces. Data is 6-bits wide (0-63 value).

void irBroadcastData( uint8_t data );

// Is there a received data ready to be read on the indicated face? Returns 0 if none.

bool irIsReadyOnFace( uint8_t face );

// Read the most recently received data. Blocks if no data ready. Data is 6-bits wide (0-63 value).

uint8_t irGetData( uint8_t face );


/* Power functions */

// The blink will automatically sleep if the button has not been pressed in
// more than 10 minutes. The sleep is preemptive - the tile stops in the middle of whatever it
// happens to be doing.

// The tile wakes from sleep when the button is pressed. Upon waking, it picks up from exactly
// where it left off. It is up to the programmer to check to see if the blink has slept and then
// woken and react accordingly if desired.


// Returns 1 if we have slept and woken since last time we checked
// Best to check as last test at the end of loop() so you can
// avoid intermediate display upon waking.

uint8_t hasWoken(void);

/*

    These hook functions are filled in by the sketch

*/


// Called when this sketch is first loaded and then
// every time the tile wakes from sleep

void setup(void);

// Called repeatedly just after the display pixels
// on the tile face are updated

void loop();

// Add a function to be called after each pass though loop()

void addOnLoop( chainfunction_struct *chainfunction );


/*

	Some syntactic sugar to make our progrmas look not so ugly.

*/


#define FACE_COUNT 6

// 'Cause C ain't got iterators and all those FOR loops are too ugly.
#define FOREACH_FACE(x) for(int x = 0; x < FACE_COUNT ; ++ x)       // Pretend this is a real language with iterators

// Get the number of elements in an array.
#define COUNT_OF(x) ((sizeof(x)/sizeof(x[0])))



#endif /* BLINKLIB_H_ */
