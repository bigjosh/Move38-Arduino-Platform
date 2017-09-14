/*
 * blink.h
 *
 * This defines a high-level interface to the blinks tile hardware.
 *
 */ 

#ifndef BLINK_H_
#define BLINK_H_

/* 

	This set of functions let you test for changes in the envronment. 

*/

// Did the state on any face change since last called?
// Get the neighbor states with getNeighborState()

bool neighborChanged();


// Was the button clicked or double clicked since we last checked?
// Note that there is a delay between when the button is first pressed
// and when you see a buttonClicked() becuase we have to wait breifly to
// to decide if that was just a single click or the first click of a double or
// tripple click. 

bool buttonClicked();

bool buttonDoubleClicked();

bool buttonTrippleClicked();

/*

	This set of functions lets you read the current state of the enviornment.

*/

// Returns true if the button currenly pressed down 
// Not debounced, this is the instantanious state of the button

bool getButtonDown();

// Returns the last recieved state of the indicated face, or
// 0 if no messages recieved recently on indicated face

byte getNeighborState( byte face );

// Returns true if we have recently recieved a valid message from a neighboor
// on the indicated face


/*

	This set of functions lets you change the state of the enviornment.

*/

// Set ouor state to newState. This state is repeatedly broadcast to any
// neighboring tiles. 
// Note that setting our state to 0 make us stop broadcasting and effectively 
// disappear from the view of neighbooring tiles. 

void setState( byte newState );


// Set the pixel on the specified face (0-5) to the specified color

void setColor( byte pixel, Color newColor);

// Set all 6 pixels  to the specified color

void setColorAll(  Color newColor );


#endif /* BLINK_H_ */