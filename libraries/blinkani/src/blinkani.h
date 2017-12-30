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
 */

#ifndef BLINKANI_H_
#define BLINKANI_H_

#ifndef BLINKCORE_H_
    #error You must #include blinkcore.h before blinkani.h
#endif

// By default we power up in state 0.

// send the color you want to blink, the rate at which it should blink at, and how many times it should blink
void blink( Color newColor, uint16_t period, uint8_t occurances);

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

#endif /* BLINKANI_H_ */
