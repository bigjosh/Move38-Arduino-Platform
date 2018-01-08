/*
    Happy New Years - 2018

    Firecracker

    Rules:
    If button pressed...
    explode with firecracker like flashing of lights
    when done, pick one of my neighbors to send the spark to
    if receive spark, explode like firecracker
    remember who I received it from
    when done, pick one of my other neighbors to send the spark to
    if no other neighbors available, firecracker is extinguished

*/

#include "blinklib.h"

bool shouldSendSpark = false;
int receivedSparkFrom = -1;
bool hasNeighbor[] = {0, 0, 0, 0, 0, 0};
uint32_t firecrackerTime_ms = 0;
uint16_t firecrackerDuration_ms = 400;
uint16_t sendSparkDelay_ms = 100;
uint16_t sendSparkDuration_ms = firecrackerDuration_ms - sendSparkDelay_ms;

void setup() {
  // put your setup code here, to run once:

}

// Show orange on any face that has a neighbor. OFF on others. 

void showOnlyNeighbors() {
    FOREACH_FACE(f) {
        // acknowledge neighbors presence w/ red glow
        if (hasNeighbor[f]) {
            setFaceColor(f, makeColorHSB(25, 255, 128));
        }
        else {
            setFaceColor(f, OFF);
        }
    }
}

void loop() {
  // put your main code here, to run repeatedly:
  uint32_t now = millis();

  // if button pressed, firecracker
  if (buttonSingleClicked()) {
    receivedSparkFrom = -1;
    firecrackerTime_ms = now;
  }

  if (shouldSendSpark) {

    FOREACH_FACE(f) {
      if (hasNeighbor[f]) {
        if (f != receivedSparkFrom) {
          byte face = 1 << f;
          irSendData(2, face);
        }
      }
    }
  }
  else {
    // let neighbors know I am here
    irBroadcastData(1);
  }

  // read my neighbors
  FOREACH_FACE(f) {
    if (irIsReadyOnFace(f)) {
      byte neighbor = irGetData(f);

      // acknowledge neighbors presence
      hasNeighbor[f] = true;

      if (neighbor == 2) {
        receivedSparkFrom = f;
        firecrackerTime_ms = now;
      }
    }
    else {
      // acknowledge neighbors absence
      hasNeighbor[f] = false;
    }
  }

  // display firecracker
  if (now - firecrackerTime_ms < firecrackerDuration_ms) {

    byte face = millis() % 6; // pick random face
    showOnlyNeighbors();
    setFaceColor(face, WHITE);

    if ( now - firecrackerTime_ms > sendSparkDelay_ms &&
         now - firecrackerTime_ms < sendSparkDelay_ms + sendSparkDuration_ms) {

      shouldSendSpark = true;
    }
  }
  else {
    shouldSendSpark = false;
    receivedSparkFrom = -1;
    showOnlyNeighbors();
  }
}


