/*
 * blinkani.cpp
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

#include "blinklib.h"

#include "chainfunction.h"

// Tell blinkstate.h to save the IR functions just for us...

#define BLINKANI_CANNARY

#include "blinkani.h"

struct Effect {
  virtual void nextStep();
  virtual bool isComplete();
};

// this effect is running when no effect is running
struct nullEffect : Effect {

  nullEffect() {
  }

  void nextStep() {

  }

  bool isComplete() {
    return true;
  }
};

// the first and simplest effect
struct blinkEffect : Effect {

  uint32_t _timeOfNextStep_ms;
  bool _isDisplayOn;
  uint16_t _period_ms;
  uint8_t _remainingOccurances;
  Color _blinkColor;

  blinkEffect(uint16_t period_ms, uint8_t occurances, Color newColor) {
    _period_ms = period_ms;
    _remainingOccurances = occurances;
    _blinkColor = newColor;
    // init time
    _timeOfNextStep_ms = 0;
    _isDisplayOn = false;
  }

  void nextStep() {

    uint32_t now = millis();

    if(now >= _timeOfNextStep_ms) {

      if(_isDisplayOn) {

        setColor(OFF);

        if(_remainingOccurances > 0) {
          _remainingOccurances--;
        }
      }
      else {
        setColor(_blinkColor);
      }

      _isDisplayOn = !_isDisplayOn;

      // update our next step time
      _timeOfNextStep_ms = now + _period_ms;
    }
  }

  bool isComplete() {
    return _remainingOccurances > 0;
  }
};

static Effect currentEffect = nullEffect();

// Called one per loop() to check for new data and repeat broadcast if it is time
// Note that if this is not called frequently then neighbors can appear to still be there
// even if they have been gone longer than the time out, and the refresh broadcasts will not
// go out often enough.

// TODO: All these calls to millis() and subsequent calculations are expensive. Cache stuff and reuse.

void blinkAniOnLoop(void) {

  // This is the loop that gets called after every loop
  if(!currentEffect.isComplete()) {
    currentEffect.nextStep();
  }
}

// Make a record to add to the callback chain

static struct chainfunction_struct blinkAniOnLoopChain = {
     .callback = blinkAniOnLoop,
     .next     = NULL                  // This is a waste because it gets overwritten, but no way to make this un-initialized in C
};

// Something tricky here:  I can not find a good place to automatically add
// our onLoop() hook at compile time, and we
// don't want to follow idiomatic Arduino ".begin()" pattern, so we
// hack it by adding here the first time anything that could use state
// stuff is called. This is an ugly hack. :/

// TODO: This is a good place for a GPIO register bit. Then we could inline the test to a single instruction.,

static uint8_t hookRegisteredFlag=0;        // Did we already register?

static void registerHook(void) {
    if (!hookRegisteredFlag) {
        addOnLoop( &blinkAniOnLoopChain );
        hookRegisteredFlag=1;
    }
}

// Manually add our hooks

void blinkAniBegin(void) {
    registerHook();
}

// send the color you want to blink, the rate at which it should blink at, and how many times it should blink
void blink(uint16_t period_ms, uint8_t occurances, Color newColor) {
  currentEffect = blinkEffect(period_ms, occurances, newColor);
}

// send the color you want to fade to, the duration of the fade
void fadeTo( Color newColor, uint16_t duration) {

}

//
Color getFaceColor(byte face) {
    //
}

//
Color getColor() {
    // if the color is the same on all faces return the color, otherwise return OFF???
    // TODO: Think this through
}
