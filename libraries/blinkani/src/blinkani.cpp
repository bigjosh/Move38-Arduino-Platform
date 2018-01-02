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

// Here we simulate an interface in C
// It is ugly, but works and is time & space efficient 


// The data for each effect is held in a union so that we only need a single memory footprint
// for the largest effect.

struct null_data_t {
    // Intentionally blank for uniformity
};

struct blink_data_t {
    uint32_t timeOfNextStep_ms;
    bool isDisplayOn;
    uint16_t period_ms;
    uint8_t remainingOccurances;
    Color color;       
};    


static struct Effect_t {
    
    // Every effect has a nextStep() and an isComplete() function

    void (*nextStep)();               // Called once per loop to update the display.

    bool (*isComplete)();             // Called to check if the current effect is finished running
    
    union {
        
        null_data_t     null_data;
        blink_data_t    blink_data;
        
    } data;        
    
    
} currentEffect;    


/*
    Null Effect - Place holder, does nothing. 
    
*/    

static void null_nextStep() {
}

static bool null_isComplete() {
    return true;
}    

void nullEffect(void) {    
    currentEffect.nextStep    = null_nextStep;
    currentEffect.isComplete  = null_isComplete;
}    

/* 

    Blink Effect - Blink a specified number of times with a specified speed

*/

static void blink_nextStep() {
    
    blink_data_t data = currentEffect.data.blink_data;
    
    uint32_t now = millis();

    if(now >= data.timeOfNextStep_ms) {

        if( data.isDisplayOn) {

            setColor(OFF);

            if( data.remainingOccurances > 0) {
                data.remainingOccurances--;
            }
        }
        else {
            setColor( data.color );
        }

        data.isDisplayOn = !data.isDisplayOn;

        // update our next step time
        data.timeOfNextStep_ms = now + data.period_ms;
    }    
    
}

static bool blink_isComplete() {
    blink_data_t data = currentEffect.data.blink_data;
    return data.remainingOccurances == 0;
}

void blink(uint16_t period_ms, uint8_t occurances, Color newColor) {
    
    blink_data_t data = currentEffect.data.blink_data;
   
    data.color = newColor;
    data.remainingOccurances=occurances;
    
    data.isDisplayOn = true;        // Start with on phase
    
    data.period_ms = period_ms;
    
    data.timeOfNextStep_ms =0;     // Start immediately
    
    currentEffect.nextStep    = blink_nextStep;
    currentEffect.isComplete  = blink_isComplete;
    
}


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

static void registerHook(void) {
    addOnLoop( &blinkAniOnLoopChain );
}

// Manually add our hooks

void blinkAniBegin(void) {    

    nullEffect();       // Start with the null effect (we need to populate the callbacks with something )
        
    registerHook();
}


// send the color you want to fade to, the duration of the fade
void fadeTo( Color newColor, uint16_t duration) {

}

/*
Color getFaceColor(byte face) {
    //
}

//
Color getColor() {
    // if the color is the same on all faces return the color, otherwise return OFF???
    // TODO: Think this through
}
*/