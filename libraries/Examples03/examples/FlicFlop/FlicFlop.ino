/*
 *  FlicFlop
 *  by Move38, Inc. 2019
 *  Lead development by Dan King
 *  original game by Nick Bentley, Jonathan Bobrow, Dan King
 *
 *  Rules: https://github.com/Move38/Astro/blob/master/README.md
 *
 *  --------------------
 *  Blinks by Move38
 *  Brought to life via Kickstarter 2018
 *
 *  @madewithblinks
 *  www.move38.com
 *  --------------------
 */

enum blinkTypes {FLOPPER, FLICKER};
byte blinkType = FLICKER;

#define TEAM_COUNT 3
byte scoringTeam = 0;
byte signalTeam = 0;

byte displayColor = 0;

enum gameStates {GAME, TRANSITION, SCORE};
byte gameState = GAME;

Timer flopTimer;
#define FLOP_INTERVAL 2000


Timer animTimer;
#define ANIMATION_INTERVAL 200
byte spinFace = 0;
byte teamHues[4] = {0, 45, 125, 230};

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  if (blinkType == FLOPPER) {
    flopperLoop();
    flopperDisplay();
  } else if (blinkType == FLICKER) {
    flickerLoop();
    flickerDisplay();
  }

  //set up communication
  byte sendData = (blinkType << 5) + (gameState << 3) + (signalTeam);
  setValueSentOnAllFaces(sendData);

  //dump button presses
  buttonSingleClicked();
  buttonDoubleClicked();
}

void flopperLoop() {
  if (gameState == GAME) {
    //time to change team?
    if (flopTimer.isExpired()) {
      signalTeam = (signalTeam % TEAM_COUNT) + 1;
      flopTimer.set(FLOP_INTERVAL);
    }

    //if double clicked, move to transition
    if (buttonDoubleClicked()) {
      gameState = TRANSITION;
    }
  } else if (gameState == TRANSITION) {
    if (scoreCheck()) {
      gameState = SCORE;
      signalTeam = 1;
    }
  } else if (gameState == SCORE) {
    if (buttonDoubleClicked()) {
      if (isAlone()) {
        gameState = GAME;
      }
    }
  }

  //change to flopper?
  if (buttonLongPressed()) {
    if (isAlone) {
      blinkType = FLICKER;
      scoringTeam = 0;
      signalTeam = 0;
      gameState = GAME;
    }
  }
}

void flickerLoop() {
  if (gameState == GAME) {
    if (signalTeam > 0) { //I'm in the game itself
      bool stillChecking = true;
      FOREACH_FACE(f) {
        if (stillChecking) {
          if (!isValueReceivedOnFaceExpired(f)) {//neighbor!

            //team change?
            if (getSignalTeam(getLastValueReceivedOnFace(f)) == 1 && signalTeam == 3) {
              signalTeam = 1;
              stillChecking = false;
            } else if (getSignalTeam(getLastValueReceivedOnFace(f)) == 2 && signalTeam == 1) {
              signalTeam = 2;
              stillChecking = false;
            } else if (getSignalTeam(getLastValueReceivedOnFace(f)) == 3 && signalTeam == 2) {
              signalTeam = 3;
              stillChecking = false;
            }
          }
        }
      }

    } else {//Im just a flicker in waiting
      //wait for a neighbor to appear who is part of the game
      FOREACH_FACE(f) {
        if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
          if (getSignalTeam(getLastValueReceivedOnFace(f)) > 0) {
            scoringTeam = getSignalTeam(getLastValueReceivedOnFace(f));
            signalTeam = scoringTeam;
          }
        }
      }
    }

    //also, time to transition?
    if (transitionCheck()) {
      gameState = TRANSITION;
      signalTeam = 0;
    }

  } else if (gameState == TRANSITION) {
    if (scoreCheck()) {
      gameState = SCORE;
    }
  } else if (gameState == SCORE) {
    if (buttonDoubleClicked() || gameCheck()) {
      gameState = GAME;
      scoringTeam = 0;
    }
  }

  //change to flicker?
  if (buttonLongPressed()) {
    if (isAlone) {
      blinkType = FLOPPER;
      scoringTeam = 0;
      signalTeam = 0;
      gameState = GAME;
    }
  }
}

/////////////////
//DISPLAY LOOPS//
/////////////////

void flopperDisplay() {
  if (animTimer.isExpired()) {
    spinFace = (spinFace + 1) % 6;
    animTimer.set(ANIMATION_INTERVAL);
  }

  if (gameState == GAME) {
    setColor(makeColorHSB(teamHues[signalTeam], 255, 255));
    setColorOnFace(OFF, spinFace);
  } else {
    FOREACH_FACE(f) {
      setColorOnFace(makeColorHSB(teamHues[(f / 2) + 1], 255, 255), (spinFace + f) % 6);
    }
  }
}

void flickerDisplay() {
  if (animTimer.isExpired()) {
    spinFace = (spinFace + 1) % 6;
    if (spinFace == 0) {
      displayColor = ((displayColor + 1) % 3) + 1;
    }
    animTimer.set(ANIMATION_INTERVAL);
  }

  if (gameState == GAME) {//game/waiting display
    if (scoringTeam > 0) {//we are connected to the game
      setColor(OFF);

      setColorOnFace(makeColorHSB(teamHues[scoringTeam], 255, 255), spinFace);

      setColorOnFace(makeColorHSB(teamHues[signalTeam], 255, 255), (spinFace + 2) % 6);
      setColorOnFace(makeColorHSB(teamHues[signalTeam], 255, 255), (spinFace + 3) % 6);
      setColorOnFace(makeColorHSB(teamHues[signalTeam], 255, 255), (spinFace + 4) % 6);


    } else {//we are just chillin
      setColor(OFF);
      setColorOnFace(makeColorHSB(teamHues[displayColor], 255, 255), spinFace);
    }
  } else {//showing final scoring team
    setColor(makeColorHSB(teamHues[scoringTeam], 255, 255));
  }
}

/////////////////////////
//CONVENIENCE FUNCTIONS//
/////////////////////////

byte getBlinkType(byte data) {
  return (data >> 5);
}

byte getGameState(byte data) {
  return ((data >> 3) & 3);
}

byte getSignalTeam(byte data) {
  return (data & 3);
}

bool transitionCheck() {//are any neighbors trying to send me into TRANSITION?
  bool transitionBool = false;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //a neighbor
      if (getGameState(getLastValueReceivedOnFace(f)) == TRANSITION) {//this neighbor wants me to transition
        transitionBool = true;
      }
    }
  }

  return transitionBool;
}

bool scoreCheck() {//can I transition into SCORE mode safely?
  bool scoreBool = true;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //a neighbor
      if (getBlinkType(getLastValueReceivedOnFace(f)) == FLICKER) { //a neighbor I care about
        if (getGameState(getLastValueReceivedOnFace(f)) == GAME) {//this neighbor still hasn't started their transition
          scoreBool = false;
        }
      }
    }
  }

  return scoreBool;
}

bool gameCheck() {//are any neighbors trying to send me into GAME?
  bool gameBool = false;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //a neighbor
      if (getBlinkType(getLastValueReceivedOnFace(f)) == FLICKER) { //a neighbor I care about
        if (getGameState(getLastValueReceivedOnFace(f)) == GAME) {//this neighbor wants me to go back to game
          gameBool = true;
        }
      }
    }
  }

  return gameBool;
}