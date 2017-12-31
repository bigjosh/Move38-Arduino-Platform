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

#ifndef BLINKCORE_H_
    #error You must #include blinkcore.h before blinkani.h
#endif

// send the color you want to blink, the rate at which it should blink at, and how many times it should blink
void blink( uint16_t period, uint8_t occurances, Color newColor);

// send the color you want to fade to, the duration of the fade
void fadeTo( Color newColor, uint16_t duration);

Color getColor();

Color getFaceColor(byte face);

/*

    These hook functions are filled in by the sketch

*/

// Manually add our hooks. Note that currently you never need to Call this - it is automtically invoked the first time you call an blinkani function.
// It is included to to not break code that references it, and in case we ever switch to requiring it.

void blinkAniBegin(void);

#endif /* BLINKANI_H_ */
