/*
 * Prototype Step Function
 *
 * A button press sends step from a single Blink
 * All Blinks call their step function
 *
 * Step is special in that it leaves getNeighborState as the value before step happened...
 */

#include "blinklib.h"
#include "blinkstate.h"
#include "blinkani.h"

#define STEP_SEND_DURATION   200
#define STEP_BUFFER          100


Timer stepSendTimeout;
Timer stepReceiveTimeout;

enum state {
  BREATHE,
  STEP
};

void setup() {
  // put your setup code here, to run once:
  blinkStateBegin();
  blinkAniBegin();
  setValueSentOnAllFaces(BREATHE);
}

void loop() {
  // put your main code here, to run repeatedly:
  if( buttonSingleClicked() ) {
    if( stepSendTimeout.isExpired() ) {
      step();
    }
  }

  // check neighbors for step
  FOREACH_FACE(f) {
    if( getLastValueReceivedOnFace(f) == STEP ) {
      if( stepReceiveTimeout.isExpired() ) {
        step();
      }
    }
  }

  if( stepSendTimeout.isExpired() ) {
    setValueSentOnAllFaces(BREATHE);
  }
}

void step() {
  setValueSentOnAllFaces(STEP);
  stepSendTimeout.set(STEP_SEND_DURATION);
  stepReceiveTimeout.set(STEP_SEND_DURATION + STEP_BUFFER);
  blink(WHITE, 300);
}
