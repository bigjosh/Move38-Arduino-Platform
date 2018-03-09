/*
* Mortals for Blinks
*
*  Setup: 2 player game. Tiles die slowly over time, all at the same rate.
*  Moving a single tile to a new place allows it to suck life from
*  surrounding tiles, friend or foe.
*
*  Blinks start with 60 seconds of life
*
*  When a tile is moved alone, it sucks 5 seconds of health from each one of
*  its newly found neighbors. A non-moved neighbor with a new neighbor looses 5 seconds
*  and animates in the direction it lost the life (i.e. where the neighbor showed up)
*
*   Tiles resets health to full when triple press occurs
*
*    States for game piece.
*    Alive/Dead
*    Team
*    Attack, Injured
*
*  Technical Details:
*  A long press on the tile changes the color of the tile for prototyping (switch state 1 or 2)
*
*
*/

#define ATTACK_VALUE 5                // Amount of health you loose when attacked.
#define ATTACK_DURRATION_MS   100     // Time between when we see first new neighbor and when we stop attacking.
#define HEALTH_STEP_TIME_MS  1000     // Health decremented by 1 unit this often

#define INJURED_DURRATION_MS  100     // How long we stay injured after we are attacked. Prevents multiple hits on the same attack cycle.

#define INITIAL_HEALTH         60
#define MAX_HEALTH             90

word health;

Timer healthTimer;  // Count down to next time we loose a unit of health

enum State {
  DEAD,
  ALIVE,
  ENGUARDE,   // I am ready to attack!
  ATTACKING,  // Short window when I have already come across my first victim and started attacking
  INJURED
};

byte mode = DEAD;

Timer modeTimeout;     // Started when we enter ATTACKING, when it expires we switch back to normal ALIVE.
// Started when we are injured to make sure we don't get injured multiple times on the same attack

void setup() {
  blinkStateBegin();
}


void loop() {
  
  // Update our mode first

  if(buttonDoubleClicked()) {
    // reset game piece
    mode=ALIVE;
    health=INITIAL_HEALTH;
    healthTimer.set(HEALTH_STEP_TIME_MS);
  }
  
  if (healthTimer.isExpired()) {
    
    if (health>0) {
      
      health--;
      healthTimer.set(HEALTH_STEP_TIME_MS);
      
    } else {
    
      mode = DEAD;

    }
    
  }
  
  if ( mode != DEAD ) {
    
    if(isAlone()) {
      
      mode = ENGUARDE;      // Being lonesome makes us ready to attack!
      
      } else {  // !isAlone()
      
      if (mode==ENGUARDE) {     // We were ornery, but saw someone so we begin our attack in earnest!
        
        mode=ATTACKING;
        modeTimeout.set( ATTACK_DURRATION_MS );
      }
      
    }
    
    
    if (mode==ATTACKING || mode == INJURED ) {
      
      if (modeTimeout.isExpired()) {
        mode=ALIVE;
      }
    }
  } // !DEAD
  
  FOREACH_FACE(f) {
    
    
    if(!isValueReceivedOnFaceExpired(f)) {
      
      byte neighborMode = getLastValueReceivedOnFace(f); 
      
      if ( mode == ATTACKING ) {
        
        // We take our flesh when we see that someone we attacked is actually injured
        
        if ( neighborMode == INJURED ) {
          
          // TODO: We should really keep a per-face attack timer to lock down the case where we attack the same tile twice in a single interaction.
          
          health = min( health + ATTACK_VALUE , MAX_HEALTH );
          
        }
        
      } else if ( mode == ALIVE ) {
        
        if ( neighborMode == ATTACKING ) {
          
          health = max( health - ATTACK_VALUE , 0 ) ;
          
          mode = INJURED;
          
          modeTimeout.set( INJURED_DURRATION_MS );
          
        }
        
      } else if (mode==INJURED) {
        
        if (modeTimeout.isExpired()) {
          
          mode = ALIVE;
          
        }        
      }
    }
  }
  
  
  // Update our display based on new state
  
  switch (mode) {
    
    case DEAD:
      setColor( dim( RED , 40 ) );
      break;
    
    case ALIVE:
      setColor( dim( GREEN , (health * MAX_BRIGHTNESS ) / MAX_HEALTH ) );
      break;
    
    case ENGUARDE:
      setColor( CYAN );
      break;
    
    case ATTACKING:
      setColor( BLUE );
      break;
    
    case INJURED:
      setColor( ORANGE );
      break;
    
  }
  
  setValueSentOnAllFaces( mode );       // Tell everyone around how we are feeling
  
}


