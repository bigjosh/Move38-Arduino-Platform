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


struct Effect_t {

    // Every effect has a nextStep() and an isComplete() function

    virtual void nextStep();               // Called once per loop to update the display.

    virtual bool isComplete();             // Called to check if the current effect is finished running

    Effect_t *prevEffect;                  // A pointer back to the previous effect that called this one
                                           // NULL at top of chain
};

// Pointer to the linked list of active effects. HEAD is currently running effect.
// NULL= no active effects.

static Effect_t *currentEffect=NULL;

// Make a new effect the current one
// the previous running effect is pushed and will resume when this new effect completes.
// Use this when composing an effect out of lower level effects.

 static void addEffect( Effect_t *effect ) {

    // Save currently running effect

     effect->prevEffect = currentEffect;

     // Set the new effect as currently running one

     currentEffect = effect;

 }

 // Terminates any existing running effects.
 // Use this to initially start an effect.

 static void clearEffects() {

     currentEffect = NULL;

 }

 /*

    Flash Effect - Show the specified color for the specified period of time.

*/

struct FlashEffect_t : Effect_t {

    uint32_t endTime;

    void nextStep() {
        // Nothing here, we know where are done when time runs out
    }

    bool isComplete() {
        uint32_t now = millis();
        return endTime <= now;
    }

    void start( Color c , uint16_t duration_ms) {
        uint32_t now = millis();
        setColor( c );
        endTime = now + duration_ms;
        addEffect( this );
    }

};

static FlashEffect_t flashEffect;

// public API call

void flash( Color c , uint16_t durration_ms ){

   clearEffects();
   flashEffect.start( c , durration_ms );

}


/*

    Blink Effect - Biphasic Blink

*/

struct BlinkEffect_t : Effect_t {

    // Since we immediately display the `on` phase, we only need to
    // remember what to do in the off phase

    Color m_offColor;
    uint16_t m_offDurration;

    bool m_completeFlag;

    void nextStep() {

        // We pushed a flash for the on state in start(), so we will not get
        // here until that has completed and we are ready for the off state.

        flashEffect.start( m_offColor , m_offDurration );

        m_completeFlag = true;      // As soon as the 2nd flash that we started is finished, then we are too.

    }

    bool isComplete() {
        return m_completeFlag;
    }

    void start(Color onColor,  uint16_t onDurration_ms, Color offColor , uint16_t offDurration) {

        // Set us up to display the off phase when the on phase is complete
        // Note that we add this *before* we start the flash so that we will get called
        // when flash completes.

        m_offColor = offColor;
        m_offDurration = offDurration;
        m_completeFlag = false;

        addEffect( this );

        // show the on phase now

        flashEffect.start( onColor , onDurration_ms );
    }

};

// All effects have a statically defined instance like this to allocate the memory at compile time

static BlinkEffect_t blinkEffect;

void blink(Color onColor, uint16_t onDurration_ms, Color offColor , uint16_t offDurration) {

    clearEffects();

    blinkEffect.start( onColor,  onDurration_ms, offColor , offDurration);

}

// black & instant off

void blink( Color onColor , uint16_t onDurration_ms) {
    blink( onColor ,  onDurration_ms , OFF , 0 );
}

/*

    Strobe Effect - A series of Blinks

*/

struct StrobeEffect_t : Effect_t {

    Color m_onColor;
    uint16_t m_onDurration;

    Color m_offColor;
    uint16_t m_offDurration;

    uint16_t occurancesLeft;

    void nextStep() {

        // We pushed a flash for the on state in start(), so we will not get
        // here until that has completed and we are ready for the off state.


        if (occurancesLeft) {
                blinkEffect.start( m_onColor , m_onDurration , m_offColor,  m_offDurration  );
                occurancesLeft--;
        }

    }

    bool isComplete() {
        return occurancesLeft==0;
    }

    void start( uint16_t occurances, Color onColor, uint16_t onDurration_ms, Color offColor , uint16_t offDurration) {

        // Set us up to display the off phase when the on phase is complete
        // Note that we add this *before* we start the flash so that we will get called
        // when flash completes.

        m_onColor = onColor;
        m_onDurration = offDurration;

        m_offColor = offColor;
        m_offDurration = offDurration;

        occurancesLeft = occurances;

        addEffect( this );

        // Get the first blink started
        nextStep();
    }

};

// All effects have a statically defined instance like this to allocate the memory at compile time

static StrobeEffect_t strobeEffect;


// blink specified number of times
void strobe( uint16_t occurances, Color onColor, uint16_t onDurration_ms,   Color offColor , uint16_t offDurration ) {
    clearEffects();
    strobeEffect.start( occurances, onColor,  onDurration_ms, offColor , offDurration  );
}

// black off, 50% duty cycle.
void strobe( uint16_t occurances, Color onColor,  uint16_t period_ms ) {

    uint16_t durration = period_ms/2;       // 50% duty cycle

    strobe( occurances , onColor , durration , OFF , durration );

}


/*

    Rotate Effect - Rotate a color around the faces

*/

struct RotateEffect_t : Effect_t {

    Color m_onColor;

    Color m_offColor;

    uint8_t faceStep;


    uint32_t nextStepTime;
    uint16_t stepDelayTime_ms;


    void nextStep() {

        uint32_t now = millis();

        if (now >= nextStepTime) {

            if (faceStep==FACE_COUNT) {     // We are done, turn off last pixel

                setFaceColor( 5 , m_offColor );

            } else {

                setFaceColor( faceStep , m_onColor );

                // Clear previous pixel

                if (faceStep>0) {
                    setFaceColor( faceStep-1 , m_offColor );
                }

            }

            faceStep++;

            nextStepTime = now + stepDelayTime_ms;

        }

    }

    bool isComplete() {

        return faceStep > FACE_COUNT;

    }


    void start( Color onColor, Color offColor , uint16_t stepTime_ms) {

        m_onColor = onColor;
        m_offColor = offColor;

        stepDelayTime_ms = stepTime_ms;

        faceStep=0;    // Start at the beginning

        nextStepTime =0;
        nextStep();   // prime the pump

        addEffect( this );
    }

};

// All effects have a statically defined instance like this to allocate the memory at compile time

static RotateEffect_t rotateEffect;

// rotate a dot around the faces
void rotate( Color onColor, Color offColor , uint16_t stepTime_ms ) {
    clearEffects();
    rotateEffect.start(onColor,  offColor , stepTime_ms );
}

// rotate a dot around the faces with a black background
void rotate( Color onColor, uint16_t stepTime_ms ) {
    clearEffects();
    rotateEffect.start(onColor,  OFF , stepTime_ms );
}


/*

    Spin Effect - A series of rotations

*/

struct SpinEffect_t : Effect_t {

    Color m_onColor;
    Color m_offColor;

    uint16_t m_stepDelay;

    uint16_t occurancesLeft;

    void nextStep() {

        if (occurancesLeft) {
                rotateEffect.start( m_onColor , m_offColor,  m_stepDelay );
                occurancesLeft--;
        }

    }

    bool isComplete() {
        return occurancesLeft==0;
    }

    void start( uint16_t occurances, Color onColor, Color offColor , uint16_t stepDelay ) {

        // Set us up to display the off phase when the on phase is complete
        // Note that we add this *before* we start the flash so that we will get called
        // when flash completes.

        m_onColor = onColor;
        m_offColor = offColor;

        m_stepDelay = stepDelay;

        occurancesLeft = occurances;

        addEffect( this );

        // Get the first rotate started
        nextStep();
    }

};

// All effects have a statically defined instance like this to allocate the memory at compile time

static SpinEffect_t spinEffect;

// rotate specified number of times
void spin( uint16_t occurances, Color onColor, Color offColor , uint16_t stepTime_ms ) {
    clearEffects();
    spinEffect.start( occurances, onColor,  offColor , stepTime_ms );
}

bool effectCompleted() {

    return currentEffect == NULL;

}

void blinkAniOnLoop(void) {

  // This is the loop that gets called after every loop

  // Tail any already completed effects...
  // Requires that at least one effect always be running (currently the idleEffect at the top)

  while ( currentEffect && currentEffect->isComplete()) {
      currentEffect = currentEffect->prevEffect;
  }

  if (currentEffect) {
    (currentEffect->nextStep)();
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
    clearEffects();
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
