/**
 * @file blinklib.h
 *
 * @brief This defines a high-level interface to the blinks tile hardware.
 *
 */ 

#ifndef BLINKLIB_H_
#define BLINKLIB_H_

#include "blinkcore.h"

#include <stdbool.h>
#include <stdint.h>

// Duplicated from Arduino.h

typedef uint8_t byte;


/**
 * @name Memory
 * @image html static/images/processor.svg
 */
///@{

/**
 * Set our state to newState. This state is repeatedly broadcast to any
 * neighboring tiles. 
 * Note that setting our state to 0 make us stop broadcasting and effectively 
 * disappear from the view of neighboring tiles.
 *
 * @param newState The new state that should be set.
 */

void setState(byte newState);

/**
 * Did the state on any face change since last called?
 * Get the neighbor states with getNeighborState()
 */

bool neighborChanged();

/**
 * Returns the last received state of the indicated face, or
 * 0 if no messages received recently on indicated face
 *
 * @param face Which side of the Blink from which to get the neighbor's state
 */

byte getNeighborState( byte face );

///@}

/**
 * @name Button
 * @image html static/images/button.svg
 */
///@{

/**
 * Returns true if the button has been pressed since
 * the last time it was called. 
 */

bool buttonPressed(void);

/**
 * Returns true if the button has been lifted since
 * the last time it was called. 
 */

bool buttonLifted(void);

/**
 * Was the button single, double , or multi clicked since we last checked?
 * Note that there is a delay after the button is first pressed 
 * before a click is registered because we have to wait to 
 * see if another button press is coming. 
 * A multiclick is 3 or more clicks
 * Remember that these click events fire a short time after the button is lifted on the final click
 * If the button is held down too long on the last click, then click interaction is aborted.
 */

bool buttonSingleClicked();

bool buttonDoubleClicked();

bool buttonMultiClicked();


/**
 * The number of clicks in the longest consecutive valid click cycle since the last time called.
 */
byte buttonClickCount(void);

/**
 * Remember that a long press fires while the button is still down
 */
bool buttonLongPressed(void);

/**
 * Returns true if the button is currently pressed down.
 */

bool buttonDown(void);

///@}

// Color type holds 4 bits for each R,G,B. Top bit is currently unused.

// TODO: Do we need 5 bits of resolution for each color?
// TODO: Use top bit(s) for something useful like automatic
//       blink or twinkle or something like that. 

typedef unsigned Color;

/**
 * @name Color Helpers
 */
///@{
#define BRIGHTNESS_LEVELS 32 ///< Number of brightness levels in each channel of a color

#define GET_R(color) ((color>>10)&31)
#define GET_G(color) ((color>> 5)&31)
#define GET_B(color) ((color    )&31)

// R,G,B are all in the domain 0-31
// Here we expose the interal color representation, but it is worth it
// to get the performance and size benefits of static compilation 
// Shame no way to do this right in C/C++

#define MAKECOLOR_RGB(r,g,b) ((r&31)<<10|(g&31)<<5|(b&31))

#define RED         MAKECOLOR_RGB(31, 0, 0)
#define YELLOW      MAKECOLOR_RGB(31,31, 0)
#define GREEN       MAKECOLOR_RGB( 0,31, 0)
#define CYAN        MAKECOLOR_RGB( 0,31,31)
#define BLUE        MAKECOLOR_RGB( 0, 0,31)
#define MAGENTA     MAKECOLOR_RGB(31, 0,31)


#define WHITE       MAKECOLOR_RGB(31,31,31)

#define OFF         MAKECOLOR_RGB( 0, 0, 0)
///@}

/**
 * @name LEDs
 * @image html static/images/leds.svg
 */
///@{

/**
 * Make a new color from RGB values. Each value can be 0-31.
 *
 * @param red
 * @param green
 * @param blue
 */
// We inline this so we can get compile time simplification for static colors

inline Color makeColorRGB( byte red, byte green, byte blue ) {
    return MAKECOLOR_RGB( red , green , blue );
}


/**
 * Dim the specified color. Brightness is 0-31 (0=off, 31=don't dim at all-keep original color)
 *
 * @param color
 * @param brightness
 */
// Inlined to allow static simplification at compile time

inline Color dim( Color color, byte brightness) {
    return makeColorRGB(
        (GET_R(color)*brightness)/31,
        (GET_G(color)*brightness)/31,
        (GET_B(color)*brightness)/31
    );
}

/**
 * Make a new color in the HSB colorspace. All values are 0-255.
 */

Color makeColorHSB( byte hue, byte saturation, byte brightness );

/**
 * Change the tile to the specified color
 *
 * @param newColor
 */

void setColor( Color newColor);

/**
 * Set the pixel on the specified face (0-5) to the specified color
 *
 * @param face
 * @param newColor
 */

void setFaceColor(  byte face, Color newColor );

///@}

/* 

    Timing functions

*/

/**
 * Delay the specified number of milliseconds (1,000 millisecond = 1 second)
 *
 * @param millis The number of milliseconds to delay
 */

void delay( unsigned long millis );

/**
 * Number of milliseconds since we started (since last time setup called).
 * Note that this can increase by more than 1 between calls, so always use greater than
 * and less than rather than equals for comparisons
 */

// Overflows after about 50 days 

// Note that our clock is only accurate to about +/-10%

unsigned long millis(void);

/* 

    Utility functions

*/

/**
 * Read the unique serial number for this blink tile
 * There are 9 bytes in all, so n can be 0-8
 *
 * @param n
 */

byte getSerialNumberByte( byte n );



/**
 * @name Comms
 * @image html static/images/comms.svg
 */
///@{

/**
 * Send data on a single face. Data is 7-bits wide, top bit is ignored.
 *
 * @param face
 * @param data
 */

void irSendData( uint8_t face , uint8_t data );

/**
 * Broadcast data on all faces. Data is 7-bits wide, top bit is ignored.
 *
 * @param data
 */

void irBroadcastData( uint8_t data );

/**
 * Is there a received data ready to be read on the indicated face? Returns 0 if none.
 *
 * @param face
 */

bool irIsReadyOnFace( uint8_t face );

/**
 * Read the most recently received data. Value 0-127. Blocks if no data ready.
 *
 * @param led
 */

uint8_t irGetData( uint8_t led );


/**
 * @name Error Codes
 */
///@{
#define ERRORBIT_PARITY       2    ///< There was an RX parity error
#define ERRORBIT_OVERFLOW     3    ///< A received byte in lastValue was overwritten with a new value
#define ERRORBIT_NOISE        4    ///< We saw unexpected extra pulses inside data
#define ERRORBIT_DROPOUT      5    ///< We saw too few pulses, or two big a space between pulses
#define ERRORBIT_DUMMY        6
///@}

/**
 * Read the error state of the indicated LED
 * Clears the bits on read
 *
 * @param face
 */

uint8_t irGetErrorBits( uint8_t face );

///@}


/**
 * @name Sketch
 */
///@{

/**
 * Called when this sketch is first loaded and then 
 * every time the tile wakes from sleep
 */

void setup(void);

/**
 * Called repeatedly just after the display pixels
 * on the tile face are updated
 */

void loop();

///@}

#endif /* BLINKLIB_H_ */
