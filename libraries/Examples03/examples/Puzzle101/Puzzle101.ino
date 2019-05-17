/*
 *  Puzzle101
 *  by Move38, Inc. 2019
 *  Lead development by Dan King
 *  original game by Vanilla Liu, Dan King
 *
 *  Rules: https://github.com/Move38/Puzzle101/blob/master/README.md
 *
 *  --------------------
 *  Blinks by Move38
 *  Brought to life via Kickstarter 2018
 *
 *  @madewithblinks
 *  www.move38.com
 *  --------------------
 */

////COMMUNICATION VARIABLES////
enum gameModes {SETUPAUTO, PACKETREADY, PACKETSENDING, PACKETLISTENING, PACKETRECEIVED, GAMEAUTO, TOMANUAL, SETUPMANUAL, LOCKING, GAMEMANUAL, TOAUTO};
byte gameMode = SETUPAUTO;
byte packetStates[6] = {PACKETREADY, PACKETREADY, PACKETREADY, PACKETREADY, PACKETREADY, PACKETREADY};

///ALGORITHM VARIABLES////
byte piecesPlaced = 0;
enum connections {UNDECLARED, APIECE, BPIECE, CPIECE, DPIECE, EPIECE, FPIECE, NONEIGHBOR};
byte neighborsArr[6][6];//filled with the values from above, denotes neighbors. [x][y] x is piece, y is face
byte colorsArr[6][6];//filled with 0-3, denotes color of connection. [x][y] x is piece, y is face

////ASSEMBLY VARIABLES////
bool canBeginAlgorithm = false;
bool isMaster = false;
byte masterFace = 0;//for receivers, this is the face where the master was found
Timer sparkleTimer;

Timer packetTimer;
#define TIMEOUT_DURATION 700

////GAME VARIABLES////
Color autoColors[5] = {OFF, RED, YELLOW, BLUE, WHITE};
Color manualColors[5] = {OFF, ORANGE, GREEN, MAGENTA, WHITE};
byte faceColors[6] = {0, 0, 0, 0, 0, 0};
byte faceBrightness[6] = {0, 0, 0, 0, 0, 0};
byte colorDim = 160;
byte whiteDim = 64;

void setup() {
}

void loop() {
  switch (gameMode) {
    case SETUPAUTO:
      setupAutoLoop();
      assembleDisplay();
      break;
    case PACKETREADY:
      communicationMasterLoop();
      communicationDisplay();
      break;
    case PACKETSENDING:
      communicationMasterLoop();
      communicationDisplay();
      break;
    case PACKETLISTENING:
      communicationReceiverLoop();
      communicationDisplay();
      break;
    case PACKETRECEIVED:
      communicationReceiverLoop();
      communicationDisplay();
      break;
    case GAMEAUTO:
      gameLoop();
      gameDisplay();
      break;
    case TOMANUAL:
      toManualLoop();
      assembleDisplay();
      break;
    case SETUPMANUAL:
      setupManualLoop();
      assembleDisplay();
      break;
    case LOCKING:
      lockingLoop();
      break;
    case GAMEMANUAL:
      gameLoop();
      gameDisplay();
      break;
    case TOAUTO:
      toAutoLoop();
      assembleDisplay();
      break;
  }

  //clear button presses
  buttonPressed();
  buttonDoubleClicked();
  buttonLongPressed();

  //set communications
  FOREACH_FACE(f) {
    byte sendData = (gameMode << 2) + (faceColors[f]);
    setValueSentOnFace(sendData, f);
  }
}

///////////////
//SETUP LOOPS//
///////////////

void setupAutoLoop() {
  //all we do here is wait until we have 5 neighbors
  byte numNeighbors = 0;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //neighbor!
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameMode(neighborData) == SETUPAUTO) { //this neighbor is ready for puzzling
        numNeighbors++;
        faceBrightness[f] = 255;
      } else {
        faceBrightness[f] = whiteDim;
      }
    } else {
      faceBrightness[f] = whiteDim;
    }
  }

  if (numNeighbors == 5) {
    canBeginAlgorithm = true;
  } else {
    canBeginAlgorithm = false;
  }

  if (buttonPressed() && canBeginAlgorithm == true) {//this lets us become the master blink
    makePuzzle();//RUN THE ALGORITHM
    gameMode = PACKETREADY;
    canBeginAlgorithm = false;
    isMaster = true;
  }

  FOREACH_FACE(f) {//here we listen for other blinks to turn us into receiver blinks
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor here
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameMode(neighborData) == PACKETREADY) { //this neighbor will send a puzzle soon
        gameMode = PACKETLISTENING;
        masterFace = f;//will only listen for packets on this face
      }
    }
  }

  //look for blinks telling us to turn manual
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameMode(neighborData) == TOMANUAL) {
        gameMode = TOMANUAL;
      }
    }
  }

  //look for presses that change us to manual
  if (buttonLongPressed()) {
    gameMode = TOMANUAL;
  }
}

void setupManualLoop() {
  //look for neighbors and make some connections!
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//there is someone here
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameMode(neighborData) == SETUPMANUAL || getGameMode(neighborData) == LOCKING) { //this is a compatible neighbor
        if (faceColors[f] == 0) {//oh, this is a new neighbor!
          faceColors[f] = random(2) + 1;
        }
      }
    } else {//no one here
      faceColors[f] = 0;
    }
  }

  //now we resolve color conflicts with our neighbors
  FOREACH_FACE(f) {
    byte neighborColor = getColorInfo(getLastValueReceivedOnFace(f));
    if (neighborColor != faceColors[f]) {
      switch (faceColors[f]) {
        case 1:
          if (neighborColor == 2) {
            faceColors[f] = 2;
          }
          break;
        case 2:
          if (neighborColor == 3) {
            faceColors[f] = 3;
          }
          break;
        case 3:
          if (neighborColor == 1) {
            faceColors[f] = 1;
          }
          break;
      }
    }
  }

  //look for neighbors telling us to transition to game or auto
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameMode(neighborData) == LOCKING) {
        gameMode = LOCKING;
      } else if (getGameMode(neighborData) == TOAUTO) {
        gameMode = TOAUTO;
      }
    }
  }

  //look out for double clicks to start the game
  if (buttonDoubleClicked() && !isAlone()) {//only works when connected
    gameMode = LOCKING;
  }

  //look for presses that change us to auto
  if (buttonLongPressed()) {
    gameMode = TOAUTO;
  }
}

void lockingLoop() {
  //all we do here is make sure all of our neighbors are ready to move on to the game
  bool stillWaiting = false;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);

      //look for neighbors still in setup, then wait
      if (getGameMode(neighborData) == SETUPMANUAL) {
        stillWaiting = true;
      }
    }
  }

  if (stillWaiting == false) {//time to move on
    gameMode = GAMEMANUAL;
  }
}

void toManualLoop() {
  //I can move to actual SETUPMANUAL if all neighbors know we're moving OR don't care (are in incompatible states)
  bool stillWaiting = false;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameMode(neighborData) == SETUPAUTO) {//only stop if there are neighbors left to tell
        stillWaiting = true;
      }
    }
  }

  if (stillWaiting == false) {
    gameMode = SETUPMANUAL;
  }

  //also, if I come across a TOAUTO, I should change to that
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameMode(neighborData) == TOAUTO) {
        gameMode = TOAUTO;
      }
    }
  }
}

void toAutoLoop() {
  //I can move to actual SETUPAUTO if all neighbors are in SETUPAUTO or TOAUTO
  bool stillWaiting = false;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameMode(neighborData) == SETUPMANUAL || getGameMode(neighborData) == TOMANUAL) {//these neighbors haven't gotten the message
        stillWaiting = true;
      }
    }
  }

  if (stillWaiting == false) {
    gameMode = SETUPAUTO;
  }
}

/////////////
//GAME LOOP//
/////////////

void gameLoop() {
  //all we do here is look at our faces and see if they are touching like colors
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //neighbor!
      byte neighborData = getLastValueReceivedOnFace(f);
      byte neighborColor = getColorInfo(neighborData);
      if (neighborColor == faceColors[f]) { //hey, a match!
        faceBrightness[f] = 255;
      } else {//no match :(
        faceBrightness[f] = colorDim;
      }

      //look for neighbors turning us back to setup
      if (gameMode == GAMEAUTO) {
        if (getGameMode(neighborData) == SETUPAUTO) {
          gameMode = SETUPAUTO;
        }
      } else if (gameMode == GAMEMANUAL) {
        if (getGameMode(neighborData) == SETUPMANUAL) {
          gameMode = SETUPMANUAL;
        }
      }

    } else {//no neighbor
      faceBrightness[f] = colorDim;
    }
  }

  //if we are double clicked, we go to assemble mode
  if (buttonDoubleClicked()) {
    if (gameMode == GAMEAUTO) {
      gameMode = SETUPAUTO;
    } else if (gameMode == GAMEMANUAL) {
      gameMode = SETUPMANUAL;
    }
  }


}

/////////////////
//DISPLAY LOOPS//
/////////////////

void assembleDisplay() {
  if (gameMode == SETUPAUTO) {
    if (sparkleTimer.isExpired() && canBeginAlgorithm) {
      FOREACH_FACE(f) {
        setColorOnFace(autoColors[random(3) + 1], f);
        sparkleTimer.set(50);
      }
    }

    if (!canBeginAlgorithm) {
      FOREACH_FACE(f) {
        setColorOnFace(dim(WHITE, faceBrightness[f]), f);
      }
    }
  } else if (gameMode == SETUPMANUAL) {
    //display the connection color
    FOREACH_FACE(f) {
      Color displayColor = manualColors[faceColors[f]];
      if (faceColors[f] == 0) {
        displayColor = dim(WHITE, whiteDim);
      }
      setColorOnFace(displayColor, f);
    }
  } else if (gameMode == TOAUTO) {//dim white flash
    setColor(dim(WHITE, whiteDim));
  } else if (gameMode == TOMANUAL) {//dim white flash
    setColor(dim(WHITE, whiteDim));
  }
}

void gameDisplay() {
  Color displayColor;
  FOREACH_FACE(f) {
    if (gameMode == GAMEAUTO) {
      displayColor = autoColors[faceColors[f]];
    } else if (gameMode == GAMEMANUAL) {
      displayColor = manualColors[faceColors[f]];
    }
    byte displayBrightness = faceBrightness[f];
    setColorOnFace(dim(displayColor, displayBrightness), f);
  }
}

///////////////////////
//COMMUNICATION LOOPS//
///////////////////////

void communicationMasterLoop() {

  if (gameMode == PACKETREADY) {//here we wait to send packets to listening neighbors

    byte neighborsListening = 0;
    byte emptyFace;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        byte neighborData = getLastValueReceivedOnFace(f);
        if (getGameMode(neighborData) == PACKETLISTENING) {//this neighbor is ready to get a packet.
          neighborsListening++;
        }
      } else {
        emptyFace = f;
      }
    }

    if (neighborsListening == 5) {
      gameMode = PACKETSENDING;
      sendPuzzlePackets(emptyFace);
      packetTimer.set(TIMEOUT_DURATION);
    }

  } else if (gameMode == PACKETSENDING) {//here we listen for neighbors who have received packets

    byte neighborsReceived = 0;
    byte emptyFace;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        byte neighborData = getLastValueReceivedOnFace(f);
        if (getGameMode(neighborData) == PACKETRECEIVED) {//this neighbor is ready to play
          neighborsReceived++;
        }
      } else {
        emptyFace = f;
      }
    }

    if (neighborsReceived == 5) { //hooray, we did it!
      gameMode = GAMEAUTO;
      return;
    }

    if (gameMode != GAMEAUTO && packetTimer.isExpired()) { //so we've gone a long time without this working out
      sendPuzzlePackets(emptyFace);
      packetTimer.set(TIMEOUT_DURATION);
    }
  }

  if (buttonDoubleClicked()) {
    gameMode = SETUPAUTO;
  }
}

void sendPuzzlePackets(byte blankFace) {
  //declare packets
  byte packet[6][6];

  //SEND PACKETS
  FOREACH_FACE(f) {
    FOREACH_FACE(ff) {
      packet[f][ff] = colorsArr[f][ff];
    }
    sendDatagramOnFace( &packet[f], sizeof(packet[f]), f);
  }

  //assign self the correct info
  FOREACH_FACE(f) {
    faceColors[f] = colorsArr[blankFace][f];
  }
}

void communicationReceiverLoop() {
  if (gameMode == PACKETLISTENING) {

    //listen for a packet on master face
    if (isDatagramReadyOnFace(masterFace)) {//is there a packet?
      if (getDatagramLengthOnFace(masterFace) == 6) {//is it the right length?
        byte *data = (byte *) getDatagramOnFace(masterFace);//grab the data
        //fill our array with this data
        FOREACH_FACE(f) {
          faceColors[f] = data[f];
        }
        //let them know we heard them
        gameMode = PACKETRECEIVED;

        // mark datagram as received
        markDatagramReadOnFace(masterFace);
      }
    }

    //also listen for the master face to suddenly change back to setup, which is bad
    if (getGameMode(getLastValueReceivedOnFace(masterFace)) == SETUPAUTO) { //looks like we are reverting
      gameMode = SETUPAUTO;
    }

  } else if (gameMode == PACKETRECEIVED) {
    //wait for the master blink to transition to game
    if (getGameMode(getLastValueReceivedOnFace(masterFace)) == GAMEAUTO) { //time to play!
      gameMode = GAMEAUTO;
    }
  }

  if (buttonDoubleClicked()) {
    gameMode = SETUPAUTO;
  }
}

byte getGameMode(byte data) {
  return (data >> 2);//1st, 2nd, 3rd, and 4th bits
}

byte getColorInfo(byte data) {
  return (data & 3);//returns the 5th and 6th bits
}

void communicationDisplay() {
  if (sparkleTimer.isExpired()) {
    FOREACH_FACE(f) {
      setColorOnFace(autoColors[random(3) + 1], f);
      sparkleTimer.set(50);
    }
  }
}

///////////////////////////////
//PUZZLE GENERATION ALGORITHM//
///////////////////////////////

void makePuzzle() {
  resetAll();
  piecesPlaced++;//this symbolically places the first blink in the center
  //place 2-4 NONEIGHBORS in first ring
  byte emptySpots = random(2) + 2;//this is how many NONEIGHBORS we're putting in
  FOREACH_FACE(f) {
    if (f < emptySpots) {
      neighborsArr[0][f] = NONEIGHBOR;
    }
  }

  for (int j = 0; j < 12; j++) {//quick shuffle method, random enough for our needs
    byte swapA = random(5);
    byte swapB = random(5);
    byte temp = neighborsArr[0][swapA];
    neighborsArr[0][swapA] = neighborsArr[0][swapB];
    neighborsArr[0][swapB] = temp;
  }

  //place blinks in remainings open spots
  for (byte j = 0; j < 6 - emptySpots; j++) {
    addBlink(0, 0);
  }

  byte remainingBlinks = 6 - piecesPlaced;
  byte lastRingBlinkIndex = piecesPlaced - 1;
  for (byte k = 0; k < remainingBlinks; k++) {
    addBlink(1, lastRingBlinkIndex);
  }
  colorConnections();
}

void resetAll() {
  piecesPlaced = 0;

  FOREACH_FACE(f) {
    FOREACH_FACE(i) {
      neighborsArr[f][i] = 0;
      colorsArr[f][i] = 0;
    }
  }
}

void addBlink(byte minSearchIndex, byte maxSearchIndex) {
  //we begin by evaluating how many eligible spots remain
  byte eligiblePositions = 0;
  for (byte i = minSearchIndex; i <= maxSearchIndex; i++) {
    FOREACH_FACE(f) {
      if (neighborsArr[i][f] == 0) { //this is an eligible spot
        eligiblePositions ++;
      }
    }
  }//end of eligible positions counter

  //now choose a random one of those eligible positions
  byte chosenPosition = random(eligiblePositions - 1) + 1;//necessary math to get 1-X values
  byte blinkIndex;
  byte faceIndex;
  //now determine which blink this is coming off of
  byte positionCountdown = 0;
  for (byte i = minSearchIndex; i <= maxSearchIndex; i++) {//same loop as above
    FOREACH_FACE(f) {
      if (neighborsArr[i][f] == 0) { //this is an eligible spot
        positionCountdown ++;
        if (positionCountdown == chosenPosition) {
          //this is it. Record the position!
          blinkIndex = i;
          faceIndex = f;
        }
      }
    }
  }//end of position finder

  //so first we simply place the connection data on the connecting faces
  neighborsArr[blinkIndex][faceIndex] = getCurrentPiece();//placing the new blink on the ring blink
  neighborsArr[piecesPlaced][getNeighborFace(faceIndex)] = blinkIndex + 1;//placing the ring blink on the new blink
  piecesPlaced++;

  //first, the counterclockwise face of the blinked we attached to
  byte counterclockwiseNeighborInfo = neighborsArr[blinkIndex][nextCounterclockwise(faceIndex)];
  if (counterclockwiseNeighborInfo != UNDECLARED) { //there is a neighbor or NONEIGHBOR on the next counterclockwise face of the blink we placed onto
    //we tell the new blink it has a neighbor or NONEIGHBOR clockwise from our connection
    byte newNeighborConnectionFace = nextClockwise(getNeighborFace(faceIndex));
    neighborsArr[piecesPlaced - 1][newNeighborConnectionFace] = counterclockwiseNeighborInfo;

    if (counterclockwiseNeighborInfo != NONEIGHBOR) { //if it's an actual blink, it needs to know about the new connection
      neighborsArr[counterclockwiseNeighborInfo - 1][getNeighborFace(newNeighborConnectionFace)] = piecesPlaced;
    }
  }

  //now, the clockwise face (everything reversed, but identical)
  byte clockwiseNeighborInfo = neighborsArr[blinkIndex][nextClockwise(faceIndex)];
  if (clockwiseNeighborInfo != UNDECLARED) { //there is a neighbor or NONEIGHBOR on the next clockwise face of the blink we placed onto
    //we tell the new blink it has a neighbor or NONEIGHBOR counterclockwise from our connection
    byte newNeighborConnectionFace = nextCounterclockwise(getNeighborFace(faceIndex));
    neighborsArr[piecesPlaced - 1][newNeighborConnectionFace] = clockwiseNeighborInfo;

    if (clockwiseNeighborInfo != NONEIGHBOR) { //if it's an actual blink, it needs to know about the new connection
      neighborsArr[clockwiseNeighborInfo - 1][getNeighborFace(newNeighborConnectionFace)] = piecesPlaced;
    }
  }
}

void colorConnections() {
  //you look through all the neighbor info. When you find a connection with no color, you make it
  FOREACH_FACE(f) {
    FOREACH_FACE(ff) {
      if (neighborsArr[f][ff] != UNDECLARED && neighborsArr[f][ff] != NONEIGHBOR) { //there is a connection here
        byte foundIndex = neighborsArr[f][ff] - 1;
        if (colorsArr[f][ff] == 0) { //we haven't made this connection yet!
          //put a random color there
          byte connectionColor = random(2) + 1;
          colorsArr[f][ff] = connectionColor;
          FOREACH_FACE(fff) { //go through the faces of the connecting blink, find the connection to the current blink
            if (neighborsArr[foundIndex][fff] == f + 1) {//the connection on the found blink's face is the current blink
              colorsArr[foundIndex][fff] = connectionColor;
            }
          }
        }
      }
    }
  }
}

byte getNeighborFace(byte face) {
  return ((face + 3) % 6);
}

byte nextClockwise (byte face) {
  if (face == 5) {
    return 0;
  } else {
    return face + 1;
  }
}

byte nextCounterclockwise (byte face) {
  if (face == 0) {
    return 5;
  } else {
    return face - 1;
  }
}

byte getCurrentPiece () {
  // Because a piece is represented by a value simply 1 greater than the pieces placed numner
  // this is more efficient to compile than the original switch statement. I understand this
  // is less clear to read, but it saves us needed space. Check the enum up top to understand
  // that 0 should return PIECE_A and 1 should return PIECE_B (which are shifted by one)
  return piecesPlaced + 1;
}