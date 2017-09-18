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

typedef unsigned int Color;

// R,G,B are all in the domain 0-15

#define MAKECOLOR(r,g,b) ((r&0b00001111)<<8|(g&0b00001111)<<4|(b&0b00001111))

#define RED     MAKECOLOR(0x0f,    0,    0)
#define BLUE    MAKECOLOR(   0,    0, 0x0f)
#define GREEN   MAKECOLOR(   0, 0x0f,    0)

#define OFF     MAKECOLOR(   0,    0,    0)


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

    These hook functions are filled in by the sketch

*/


// Called when this sketch is first loaded and then 
// every time the tile wakes from sleep

void setup(void);

// Called repeatedly just after the display pixels
// on the tile face are updated

void loop();

#endif /* BLINK_H_ */