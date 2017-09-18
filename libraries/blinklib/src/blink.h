/*
 * blink.h
 *
 * This defines a high-level interface to the blinks tile hardware.
 *
 */ 

#ifndef BLINK_H_
#define BLINK_H_

/* 

	This set of functions let you test for changes in the environment. 

*/

// Did the state on any face change since last called?
// Get the neighbor states with getNeighborState()

bool neighborChanged();


// Was the button clicked or double clicked since we last checked?
// Note that there is a delay between when the button is first pressed
// and when you see a buttonClicked() becuase we have to wait breifly to
// to decide if that was just a single click or the first click of a double or
// triple click. 

bool buttonClicked();

bool buttonDoubleClicked();

bool buttonTrippleClicked();

/*

	This set of functions lets you read the current state of the environment.

*/

// Returns true if the button currently pressed down 
// Not debounced, this is the instantaneous state of the button

bool buttonDown();

// Returns the last received state of the indicated face, or
// 0 if no messages received recently on indicated face

byte getNeighborState( byte face );

// Returns true if we have recently received a valid message from a neighbor
// on the indicated face


/*

	This set of functions lets you change the state of the environment.

*/

// Set our state to newState. This state is repeatedly broadcast to any
// neighboring tiles. 
// Note that setting our state to 0 make us stop broadcasting and effectively 
// disappear from the view of neighboring tiles. 

void setState( byte newState );

// Color type holds 4 bits for each R,G,B. Top bit is currently unused.

// TODO: Do we need 5 bits of resolution for each color?
// TODO: Use top bit(s) for something useful like automatic
//       blink or twinkle or something like that. 

typedef unsigned Color;


// Number of brightness levels in each channel of a color
#define BRIGHTNESS_LEVELS 32

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



#define OFF     MAKECOLOR_RGB( 0, 0, 0)

// We inline this so we can get compile time simplification for static colors

// Make a new color from RGB values. Each value can be 0-31. 

inline Color makeColorRGB( byte red, byte green, byte blue ) {
    return MAKECOLOR_RGB( red , green , blue );
}


// Dim the specified color. Brightness is 0-31 (0=off, 31=don't dim at all-keep original color)
// Inlined to allow static simplification at compile time

inline Color dim( Color color, byte brightness) {
    return makeColorRGB(
        (GET_R(color)*brightness)/31,
        (GET_G(color)*brightness)/31,
        (GET_B(color)*brightness)/31
    );
}

// Make a new color in the HSB colorspace. All values are 0-255.

Color makeColorHSB( byte hue, byte saturation, byte brightness );
    
// Change the tile to the specified color 

void setColor( Color newColor);

// Fade the tile to the new color taking `millis` milliseconds to make the transition.  
// (1,000 millis = 1 second, max 65 seconds)

void fadeTo( Color newColor, unsigned millis);


// Set the pixel on the specified face (0-5) to the specified color

void setFaceColor(  byte face, Color newColor );


/* 

    Timing functions

*/

// Delay the specified number of milliseconds (1,000 millisecond = 1 second) 

void delay( unsigned long millis );

/* 

    Utility functions

*/


// Get the number of elements in an array.
// https://stackoverflow.com/a/4415646/3152071

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))


/*

    These hook functions are filled in by the sketch

*/


// Called when this sketch is first loaded and then 
// every time the tile wakes from sleep

void setup(void);

// Called repeatedly just after the display pixels
// on the tile face are updated

void loop();

#endif /* BLINK_H_ */