/*
* Speed up the rotation of LEDs lights or slow down based on button press
*/

#include "blinklib.h"
#include "blinkstate.h"
#include "Serial.h"

ServicePortSerial Serial;

uint16_t speed_d_per_s = 360; // speed is represented in degrees/sec, 360 would be a single rotation per second

float speed_d_per_ms = ( (float) speed_d_per_s) / MILLIS_PER_SECOND ;

enum team_t { TEAM_A , TEAM_B };

byte teamHues[] = { 0 , 128 };

enum state_t { STATE_DEAD , STATE_START , STATE_NORMAL , STATE_WITHDRAW ,  STATE_ALONE , STATE_ATTACK };

// We need a margin time both when withdrawing and when attacking

// We need this to make sure there is a delay between when we stop seeing any neighbors and
// when we start sending alone state- otherwise we could immediately attack neighbors as we pull
// away from them

// We also need to make sure there is a delay between when we start attacking neighbors and when
// we switch back to being normal to make sure we finish attacking our new neighbors

#define WITHDRAWAL_TIME_MS  200
#define ATTACK_TIME_MS      200
#define START_TIME_MS       400

// We all come into this world DEAD.
// A triple click puts us into NORMAL with HEALTH_START health.
//  Health drops over time.
//  When we see no neighbors, we go into WITHDRAWAL state.
//  After we have been in withdrawal for a while, we become ALONE
//  When in ALONE state, we start looking for neighbors,
//    Once we see one, we start counting down DOCKING time
//    At the end of DOCKING time period, we count the number of neighbors and add 5000*count to our health
//  When in NORMAL state, if we see an ALONE nearby then we deduct 5000 from our health.

// Health starts game at 60 seconds.

uint32_t health;    // Units ms

#warning shortend health for testing
#define HEALTH_START 30000U   // Starting value of health
//#define HEALTH_START 60000   // Starting value of health

#define HEALTH_MAX  120000U     // We need a cap here to prevent overflow uint16

#define HEALTH_ATTACK 5000U     // How many units we loose when attacked

#define HEALTH_LOSS_PER_MS 1U   // We loose this many health each ms just from normal aging

//#define HEALTH_MAX_DISPLAY 60000    // any health above this will just look full

team_t team;

state_t state;

uint32_t timeOfLastLoop_ms;

void setup() {
	
	Serial.begin();

	Serial.println("Mortals Debug");

	team = TEAM_A;

	state = STATE_DEAD;

	timeOfLastLoop_ms = millis();

}

// Sin in degrees ( standard sin() takes radians )

float sin_d( uint16_t degrees ) {

	return sin( ( degrees / 360.0F ) * 2.0F * PI   );
}

float rotation_d = 0;     // Spin display over time

byte lastValue[FACE_COUNT];     // Keep track of last seen value on each face so we can tell if it changed

// Reduce health with 0 floor

void reduceHealthBy( uint16_t x ) {
	
	if ( health > x ) {
		health -= x;
		} else {
		health = 0;
	}
}

// Increase health with HEALTH_MAX ceiling

void increaseHealthBy( uint16_t x ) {
	
	if ( health < ( HEALTH_MAX - x )) {
		health += x;
		} else {
		health = HEALTH_MAX;
	}
}

// loop like we are Elm main:
// Establish inputs (button,neighbors)
// Update state (health,state)
// Produce outputs (display , transmitted state)

Timer nextState; 

void loop() {
	
	uint32_t now = millis();

	if (buttonLongPressed()) {      // Switch team (displayed color) on long press

		if (team==TEAM_A) {
			team=TEAM_B;
			} else {
			team=TEAM_A;
		}
		
		
	}

	if (buttonMultiClicked()) {     // Restart on triple click

		state = STATE_START;
		health = HEALTH_START;
		
		nextState.setMSFromNow( START_TIME_MS ); // Wait a bit to let neighbors start too before we attack each other!
		Serial.println("START");
		
	}

	uint32_t timeDiff_ms = now - timeOfLastLoop_ms;
	
	// First see what is going on around our edges
	
	bool aloneFlag = true;          // Assume we are alone until we see neighbors
	byte newAttackers=0;            // Anyone on any face attack us that was not attacking last pass?
	byte newNormals=0;
	
	FOREACH_FACE(f) {
				
		byte newNeighboorState = getNeighborState( f );
		
		if ( !isNeighborExpired(f) ) {
			
			aloneFlag = false;                  // We may not be loved, but we are not alone
		}
		
		if ( newNeighboorState != lastValue[f] ) {            // Changed?
			
			if ( newNeighboorState == STATE_ATTACK ) {         // New attacking neighbor!
				
				newAttackers++;
				
			}
			
			if ( newNeighboorState == STATE_NORMAL ) {
				
				newNormals++;
				
			}
			
			lastValue[f] = getNeighborState(f);
		}
	}
	
	
	switch (state) {
		
		case STATE_DEAD:
		
		// Dead is as dead does
		break;
		
		case STATE_START:
		
		if ( now > state_timeout_ms ) {
			
			state = STATE_NORMAL;
			Serial.println("NORMAL");
			
			
		}
		
		break;
		
		case STATE_NORMAL:
		
		// Is this correct behavior? Is it even even valid to have more than one attacker?
		
		reduceHealthBy( HEALTH_ATTACK * newAttackers );      // Will not go below 0
		
		if (newAttackers) {
			Serial.print("Attacked by ");
			Serial.println(newAttackers);
		}
		
		if (aloneFlag) {
			
			// We are newly alone, so start withdrawing...
			
			state = STATE_WITHDRAW;
			state_timeout_ms = now + WITHDRAWAL_TIME_MS;
			
			Serial.println("WITHDRAW");
			
		}
		
		break;
		
		case STATE_WITHDRAW:
		
		if ( now > state_timeout_ms ) {
			
			state = STATE_ALONE;
			Serial.println("ALONE");
			
			
		}
		
		break;
		
		case STATE_ALONE:
		
		if (!aloneFlag) {            // Not alone any more!
			
			state = STATE_ATTACK;
			state_timeout_ms = now + ATTACK_TIME_MS;
			Serial.println("ATTACK");

			
		}
		
		increaseHealthBy( newNormals * HEALTH_ATTACK );

		if (newNormals) {
			Serial.print("Initial Stole from ");
			Serial.println(newNormals);
		}
		
		
		break;
		
		case STATE_ATTACK:
		
		increaseHealthBy( newNormals * HEALTH_ATTACK );

		if (newNormals) {
			Serial.print("Ongoing Stole from ");
			Serial.println(newNormals);
		}

		
		if ( now > state_timeout_ms ) {
			
			state = STATE_NORMAL;
			Serial.println("NORMAL");

			
		}
		
		break;

	}
	
	// Next we age and lose health and maybe die
	
	if (state != STATE_DEAD ) {

		if ( timeDiff_ms > health ) {

			// Oh no! We died!

			state = STATE_DEAD;

			setColor( WHITE );        // Death flash

			delay(100);
			
			} else {

			reduceHealthBy( timeDiff_ms * HEALTH_LOSS_PER_MS );
			
		}
	}
	
	// Next update the display to reflect our current health
	
	if (state == STATE_DEAD ) {
		
		setColor(OFF);
		
		} else { // state != DEAD
		
		// calculate the amount of rotation
		
		//speed_d_per_ms = ( ((1.0f - (min(  health  , HEALTH_MAX_DISPLAY ) / HEALTH_MAX_DISPLAY ))) * 4.0 * 360.0) / MS_PER_S;
		
		//speed_d_per_ms = ((1.0f - ( (HEALTH_MAX_DISPLAY/1.5) / HEALTH_MAX_DISPLAY )) * 4.0 * 360.0) / MS_PER_S;
		
		
		rotation_d += timeDiff_ms * speed_d_per_ms ;
		
		if(rotation_d >= 360.f) {
			rotation_d -= 360.f;
		}
		
		//byte angularWidth_d =  max(  health  , HEALTH_MAX_DISPLAY ) *  360  / HEALTH_MAX_DISPLAY;
		
		//byte brightness =  min(  health  , HEALTH_MAX_DISPLAY ) *  255  / HEALTH_MAX_DISPLAY;
		
		FOREACH_FACE(f) {
			// determine brightness based on the unit circle (sinusoidal fade)
			
			uint16_t angle_of_face_d = 60 * f;      // (360 degrees) / (6 faces) = degrees per face
			
			uint16_t angle_of_health_d =  (min(  health  , 30000 ) / 30000.0) * 360;
			
			byte brightness;
			
			if ( angle_of_face_d < angle_of_health_d ) {
				
				brightness = 255;
				}   else {
				brightness = 0;
			}
			
			//int brightness = 255 * (1 + sin_d( angle_of_face_d  + rotation_d ) ) / 2;
			
			setFaceColor( f ,  makeColorHSB( teamHues[team]  , 255 , brightness  ) );
			
			
			//      if(f==4) {
			//        Serial.print("rotation: ");
			//        Serial.println(rotation);
			//        Serial.print("brightness: ");
			//        Serial.println(brightness);
			//      }
			
		}
		
	}    // state != DEAD

	timeOfLastLoop_ms = now;

	setState( state );
	
}