/*
 * blinkani.h
 *
 * This defines a display manager for blinks tile to animate its pixels at a high level.
 *
 * Animations are different than setting color in that they are time sensitive actions. each
 * animation has a duration (usually ms) and will either run for the duration or if the duration is 0,
 * will continue to loop until another animation is called or setColor clears the display manager.
 *
 * Note that this library depends on the blinklib library for control of each pixel. The blinklib
 * setColor functions are available when using the blinkani library, so the user must understand how
 * the two interact. setColor will always stop and clear display manager animations
 *
 * Note that the display animations only occur when the loop() function returns, so it is important
 * that sketches using this model return from loop() frequently. (i.e. don't use delay)
 *
 * ROADMAP
 * 1. display manager simply forces latest command to be the display mode
 * 2. queue manager for timeline like animation, adding display actions to a stack
 *    1. push, pop, clear
 *    2. setColor() always clears the stack
 *
 */

#ifndef BLINKANI_H_
#define BLINKANI_H_

#ifndef BLINKLIB_H_
    #error You must #include blinklib.h before blinkani.h
#endif

// Call to initialize the blinkani subsystem before starting any effects

void blinkAniBegin(void);


// Show a color for specified time
void flash( Color c , uint16_t durration_ms );

// Show two colors in a row
void blink(Color onColor, uint16_t onDurration_ms, Color offColor , uint16_t offDurration);

// black & instant off

void blink( Color onColor , uint16_t onDurration_ms);


// blink specified number of times
void strobe( uint16_t occurances, Color onColor,  uint16_t onDurration_ms, Color offColor , uint16_t offDurration );

// black off, 50% duty cycle.
void strobe( uint16_t occurances, Color onColor,  uint16_t period_ms );


// rotate a dot around the faces
void rotate( Color onColor, Color offColor , uint16_t stepTime_ms );

// rotate a dot around the faces with a black background
void rotate( Color onColor, uint16_t stepTime_ms );

// rotate specified number of times
void spin( uint16_t occurances, Color onColor, Color offColor , uint16_t stepTime_ms );


// send the color you want to fade to, the duration of the fade
void fadeTo( Color newColor, uint16_t duration);

Color getColor();

Color getFaceColor(byte face);

bool effectCompleted();


#endif /* BLINKANI_H_ */
