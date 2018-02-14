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
 *   Long press changes team color 
 *
 *    States for game piece.
 *    Alive/Dead
 *    Team
 *    Attack, Injured
 *
 *  Technical Details:
 *  A long press on the tile changes the color of the tile for prototyping (switch state 1 or 2)
 *  Ready to attack tile shows spin pattern. 
 *  On successful attack, shows full brightness on attacked face(s).
 *  In idle, tiles breath their team color. At full health, breathing takes 2 seconds to range 255-128 brightness. At minimum health, 10hrtz breathing at lower brightness. 
 *  On injury, face where were were hit shows full RED which decays over 500ms.   
 * 
 */


#define ATTACK_VALUE 5					// Amount of health units you loose when attacked.
#define ATTACK_DURRATION_MS		100		// Time between when we see first new neighbor and when we stop attacking. 
#define HEALTH_STEP_TIME_MS		1000	// Health decremented by 1 unit this often

#define INJURED_DURRATION_MS	100		// How long we stay injured after we are attacked. Prevents multiple hits on the same attack cycle. 

#define INITIAL_HEALTH			60		 
#define MAX_HEALTH				90	


#define MAX_TEAMS           4     

byte teamIndex = 0;					// Current team


// Map team index to team color. 
// TODO: Precompute these, and cache current one
Color teamColor( byte index ) {		
	return makeColorHSB( 60 + (index * 50) , 255, 255);
}
  
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

#include "Serial.h"

ServicePortSerial sp;

void setup() {
  sp.begin();
  sp.println("Hello!");
}


class Health {

	byte value;
	byte maxValue; 
	
	public: 
	
		Health( byte m_maxValue ) {		// Really just gives us a way to ensure MAX_HEALTH fits into a byte
			maxValue=m_maxValue;
		}
			
		bool isAlive() {
			return value>0;
		}
	
		void set( byte x ) {
		
			value = x;	
		}
		
		byte get(void) {
			
			return value;
			
		}
	
		// All this should be replaced with bounded::integer
		
		
		void increase( byte x ) {
							
			if ( (x > maxValue) ||   ( value > ( maxValue-x ) ) ) {			// Mind your overflows! Order counts, people!
			
					value = maxValue;
			
			} else {
			
				value += x;
			
			}
		}
	
	
	
		void decrease( byte x ) {
		
			if ( x > value  ) {			// Mind your overflows! Order counts, people!
			
				value = 0;
			
			} else {
			
				value -= x;
			
			}
		}
		
	
};

Health health(MAX_HEALTH); 
	
void loop() {
  
  // Update our mode first  
  
  if (buttonLongPressed()) {
	  
	  teamIndex++;
	  if(teamIndex==MAX_TEAMS) {
		  teamIndex=0;
	  }
	 	  
	  // TODO: Should we reset health when changing teams?		   
  }

  if(buttonDoubleClicked()) {
    // reset game piece   
    mode=ALIVE;
    health.set(INITIAL_HEALTH);
    healthTimer.set(HEALTH_STEP_TIME_MS);
  }
  
  if (healthTimer.isExpired()) {
	  
	health.decrease( 1 );
	
	if (health.isAlive()) {
        
      healthTimer.set(HEALTH_STEP_TIME_MS);  
	  
	  sp.print("health:");
	  sp.println(health.get());
	  
    } else {	// if (!health.isAlive())
	    
		mode = DEAD;      
		
	}
  
  }
  
  if ( mode != DEAD ) {
      
    if (isAlone()) {
      
		mode = ENGUARDE;      // Being lonesome always makes us ready to attack!		
        sp.println("enguarde");
    
    } else {  // !isAlone()
      
      if (mode==ENGUARDE) {     // We were ornery, but saw someone so we begin our attack in earnest!
		  
        sp.println("attack");
        mode=ATTACKING;
        modeTimeout.set( ATTACK_DURRATION_MS );
		
      }
        
    }
        
    if (mode==ATTACKING || mode == INJURED ) {
      
      if (modeTimeout.isExpired()) {
        mode=ALIVE;
        sp.println("alive");		
      }
	  
    } 
	
  } // !DEAD
    
  FOREACH_FACE(f) {
          
    if(didValueOnFaceChange(f)) {
		
	    byte neighborState = getLastValueReceivedOnFace(f);		
        
		if ( mode == ATTACKING ) {
      
		  // We take our flesh when we see that someone we attacked is actually injured
    
		  if ( neighborState == INJURED ) {     
        
			// TODO: We should really keep a per-face attack timer to lock down the case where we attack the same tile twice in a since interaction. 
        
			health.increase(ATTACK_VALUE);
	        sp.print("flesh from ");
			sp.println(f);
			
                
		  }
      
		} else if ( mode == ALIVE ) {
        
		  if ( neighborState == ATTACKING ) {
        
			health.decrease(ATTACK_VALUE);
                                        
			mode = INJURED;
        
			modeTimeout.set( INJURED_DURRATION_MS );
			
	        sp.print("flesh to ");
	        sp.println(f);
			
      
		  }
        
		} 		
     } 
  }  
  
  
  // Update our display based on new state
  
  switch (mode) {
    
    case DEAD:
      setColor( dim( RED , 30 ) );
      break;
      
    case ALIVE:
      setColor( dim( teamColor( teamIndex) , ( health.get() * MAX_BRIGHTNESS ) / MAX_HEALTH ) );   
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

