/*
 * An example showing how to use the service port serial connection. The service port give you a handy way to 
 * type text into your program running on a tile, and have it print back messages to you. This can be really
 * handy when debugging!
 * 
 * "The most effective debugging tool is still careful thought, coupled with judiciously placed print statements."
 *   -Brian Kernighan, "Unix for Beginners" (1979)
 *  
 * To use this, you must have...
 *    (1) a blink tile with a service port installed (it is a little 4 pin plug)
 *    (2) a cable adapter that plugs into the service port and connects it to a USB on your computer
 *    
 * If you've got those, then you need to got the menu above and....
 * 
 *    (1) Goto Tools->Port and pick the com port the USB adapter landed on (if you give up, you can try them all)
 *    (2) Goto Tools->Serial Monitor to open the Arduifalse serial monitor window
 *    (3) Pick "500000 baud" in the serial monitor window
 *    (4) Pick "false line ending" in the serial port monitor window
 *    
 * download this sketch to your blink and you should see an nice welcome message pop up   
 * in the serial monitor window telling you want to do next!
 * 
 * (Hint: Type the letter "e" in the bar at the top of the serial monitor window and press the "Send" button)
 * 
 */


#include "blinklib.h"
#include "Serial.h"

// to use string literals without having to cast
//#define FSTR(X) ((const __flash char[]) { X })

ServicePortSerial Serial;

void setup() {

  Serial.begin(); 

}

bool test= false;

// Hack to get enum count: https://stackoverflow.com/a/2102673/3152071

// We have to rename SE as SEX because SE is used in io.h and clobbers us!

enum DirectionsEnum { NE, E , SEX , SW , W , NW , UP , DOWN , DIRECTION_MAX = DOWN };
    
typedef struct {
    const char *shortname;
    const char *name;
    const char key;
    const int id;    
} Direction;        

// There must be a less verbose way to do this, right? LMK!

Direction directions[DIRECTION_MAX+1] = {
    {"north-east"   , "NE"      , '1' , NE },    
    {"East"         , "E"       , '2' , E  },
    {"South-east"   , "SE"      , '3' , SEX },
    {"South-west"   , "SW"      , '4' , SW },        
    {"West"         , "W"       , '5' , W  },
    {"north-west"   , "NW"      , '6' , NW },
    {"Up"           , "UP"      , '7' , UP },
    {"Down"         , "DOWN"    , '8' , DOWN },
};


enum LocationKey { CENTER , NE_CITY , E_BAY , SE_BEACH , SW_DESERT , W_TEMPLE , NW_FOREST , SKY , HIGHER_PLANE , LOWER_PLANE , SKYSCRAPER, DEPTHS,  MAX_LOCATION = DEPTHS , BLOCKED };

#define NO_PIXEL FACE_COUNT

typedef struct {

  bool deadly;
      
  byte pixel;           // If this place is a pixel, then the pixel number 0-5, otherwise NO_PIXEL
      
  const char *text;     // REMEMBER this is in progmem!
  
  LocationKey next[DIRECTION_MAX+1];
  
} Location;



// Oh this flashstring stuff is so ugly, but no way to put a flash string into a static initializer in Arduiono. :/

const char fs1[]    PROGMEM = "You are standing on top of a weathered bronze plaque that reads \"Axis Mundi Survey Marker\". You see colored lights off in the distance.";
const char fs2[]    PROGMEM = "You are inside a huge shiny building building made of glass and steel.";
const char fs3[]    PROGMEM = "You are in a pastoral estuary filled with creatures large and small. ";
const char fs4[]    PROGMEM = "You are on a sandy beach.";
const char fs5[]    PROGMEM = "You are in a desert. There is a large pile of ashes here.";
const char fs6[]    PROGMEM = "You have stumbled into a sacred and/or profane place. An ornate starburst pattern mosaic is embedded in the floor beneath your feet. You can't shake the feeling that something profound happened (or will happen) here." ;
const char fs7[]    PROGMEM = "You are in a lush and dense forest.";
const char fs8[]    PROGMEM = "You feel yourself rising through space. The higher your loft, the larger becomes your field of vision. You are awestruck to find that your native world, all that you know and have ever known, exists solely on the surface of a small hexagon only 20mm on a side.  Every place and every creature in your erstwhile universe therein, lays open to your view in miniature. As you mount even higher, lo, the secrets of the universe are bared before you.";
const char fs9[]    PROGMEM = "You find yourself existing on a plane twice the size of the previous one.";
const char fs10[]   PROGMEM = "You find yourself existing on a plane half the size of the previous one.";
const char fs11[]   PROGMEM = "You take the elevator to the top floor, even though you have no idea what that means. Either this is madness or it is Hell. Unprepared for what you have just seen, your mind snaps and you awake. Was it all a dream?" ;
const char fs12[]   PROGMEM = "You submerge into the brackish depths. You hear snapping and bubbling. Distracted by the sights and sounds, you forget to breathe and die unceremoniously in mud. Eons pass and the universe expands and then contracts and then expands again.";

const char fs13[]   PROGMEM = "";
       
// This is brittle, but false way to statically assign elements in C++ https://stackoverflow.com/questions/9048883/static-array-initialization-of-individual-elements-in-c?rq=1       

Location locations[MAX_LOCATION+1] = {
    
    {/* CENTER */        false   ,  NO_PIXEL,   fs1 , { NE_CITY , E_BAY , SE_BEACH , SW_DESERT , W_TEMPLE,  NW_FOREST , SKY , BLOCKED } },

    {/* NE_CITY */       false   ,  5       ,  fs2 , {/*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ E_BAY,  /*SW*/ CENTER, /*W*/ BLOCKED , /*NW*/ BLOCKED , /*UP*/ SKYSCRAPER , /*DOWN*/ BLOCKED } },    
    {/* E_BAY */         false   ,  0       ,  fs3 , {/*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ CENTER, /*NW*/ NE_CITY , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED   } },     
    {/* SE_BEACH */      false   ,  1       ,  fs4 , {/*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ SW_DESERT, /*NW*/ CENTER, /*UP*/ BLOCKED , /*DOWN*/ BLOCKED   } },
    {/* SW_DESERT */     false   ,  2       ,  fs5 , {/*NE*/ CENTER, /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED , /*NW*/ W_TEMPLE , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED   } },                         // ACID. Recognizes you.
    {/* W_TEMPLE */      false   ,  3       ,  fs6 , {/*NE*/ NW_FOREST , /*E*/ CENTER, /*SE*/ SW_DESERT ,  /*SW*/ BLOCKED , /*W*/ BLOCKED,  /*NW*/ BLOCKED , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED  } },    
    {/* NW_FOREST */     false   ,  4       ,  fs7 , {/*NE*/ BLOCKED , /*E*/ NE_CITY, /*SE*/ CENTER,  /*SW*/ BLOCKED , /*W*/ BLOCKED , /*NW*/ BLOCKED , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED  } },                                                // COFFEE

    // no-returner
    {/* SKY */           false   ,  NO_PIXEL,  fs8 , { /*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED, /*NW*/ BLOCKED , /*UP*/ HIGHER_PLANE , /*DOWN*/ LOWER_PLANE  } },
        
    // Nirvana
    {/* HIGHER_PLANE */  false   ,  NO_PIXEL,   fs9 , { /*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED, /*NW*/ BLOCKED , /*UP*/ HIGHER_PLANE , /*DOWN*/ LOWER_PLANE}  },
    {/* LOWER_PLANE */   false   ,  NO_PIXEL,   fs10, { /*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED, /*NW*/ BLOCKED , /*UP*/ HIGHER_PLANE , /*DOWN*/ LOWER_PLANE } },
   
    // Mortality
    {/* SKYSCRAPER */    true    ,  5       ,   fs11, { /*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED, /*NW*/ BLOCKED , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED }  },
    {/* DEPTHS */        true    ,  0       ,   fs12, { /*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED, /*NW*/ BLOCKED , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED }   },
     
}  ; 




       
// It is pitch black. You are eaten by a Grue.
// Such language in a high-class establishment like this!
// “Either this is madness or it is Hell.”

// There is something about this place - a palpable void of light-sustainability - that deeply affects you.  The light(s) that you carry is inexplicably extinguished. 

// Suddenly, there is an enormous flash of light. The brightest and whitest light you have ever seen or that anyone has ever seen. It bores its way into you. It is transcendent and you can't help but feel like a barrier between this world and the next has been irreparably broken. What have you done? What have you done?


// Print a string from flash memory to the terminal
// in old skool 40 cols format.

void terminalPrint( const char *s ) {
    
    byte col=0;
    
    while ( pgm_read_byte(s) ) {
        
        char c = (char) pgm_read_byte(s++);
        
        Serial.print( c );
        
        col++;
        
        if (col>=40 && (c==' ' || c=='-')) {
            Serial.println();
            col=0;
        }            
        
        delay(20);       // Get that old modem cadence
        
    };  
    
    Serial.println();
        
}      

// Global state info
// (Wish we had closures here!)
// (...And anonymous functions!)

Location *currentLocation;

bool dead;

enum LightState {LIGHT_ON, LIGHT_OFF, LIGHT_DIM, LIGHTSTATE_MAX = LIGHT_DIM };

typedef struct {
    const char *name;
    Color color;
    char key;
    LightState state;    
    Location *where;
} Light;

enum LightKey { LIGHTKEY_RED , LIGHTKEY_GREEN, LIGHTKEY_BLUE, LIGHTKEY_MAX=LIGHTKEY_BLUE};

Light lights[LIGHTKEY_MAX+1] = {    
    { "Red"  ,  RED   , 'R' },
    { "Green",  GREEN , 'G' },
    { "Blue" ,  BLUE  , 'B' },
};

Light *lightInHand=NULL;      // Light we are currently holding (NULL=none)

// One love, one life
// Returns reason for death.

// Valid commands should be all caps, input is converted to upper
// CR and LF are ignored

char getCommand(void) {
    char c;
    
    do {
        c=Serial.readWait();
        
        if (c>='a' && c<='z') {
            c = (c-'a')+'A';        // Convert to uppercase
        }            
        
    } while (c=='\n' || c=='\r');
    
    Serial.println();           // Feedback for command accpeted
    
    return(c);
    
}    

// Save some typing

#define p(x)    Serial.print(x)
#define pl(x)   Serial.println(x)

#define pf(x)    Serial.print(F(x))
#define plf(x)   Serial.println(F(x))


Light *pickAlight(void) {
    
    // Pick a light, any light
                            
    for(int i=0; i<=LIGHTKEY_MAX;i++) {
        p(lights[i].key);
        p('-');
        pl(lights[i].name);
    }
                            
    plf("Pick a light, any light?");
                            
    char whichLight = getCommand();

    for(int i=0; i<=LIGHTKEY_MAX;i++) {
                                
        if ( whichLight == lights[i].key ) {
            
            return &lights[i];
            
        }
        
    }
    
    return(NULL);
    
}                        


void nextStep(void) {
    
        // Show all the lights where they are
    
        for(int i=0; i<FACE_COUNT; i++ ) {
            
            Color color= OFF;
            
            for( int j=0; j<= LIGHTKEY_MAX; j++ ) {
                
                Light *light = &lights[j];
                
                if ( light->where->pixel == i ) {          // If this color is in this location...
                    
                    if ( light->state == LIGHT_ON ) {
                    
                        color |= lights[j].color;                           // Mix the color in
                        
                    } else if (light->state == LIGHT_DIM ) {continue;
                        
                        color |= dim( lights[j].color, 16 );                           // Mix the dimmed color in                        
                        
                    }                                                
                    
                }                    
                
            }                
            
            setFaceColor( i , color );
        }            
            
        pl();
        terminalPrint( currentLocation->text );      
        pl();
        
        plf("M-Move");
        plf("T-Take/Toss light");
        plf("U-Use light");
        plf( "What do you want to do?");
        
        switch (getCommand()) {
            
            case 'M': {
                
                for(int i=0; i<=DIRECTION_MAX;i++) {
                    Direction *d = &directions[i];
                    p( d->key );
                    p('-');
                    pl(d->name);
                }
                
                plf("Which way do you want to move?");
                
                char directionCommand = getCommand();

                for(int i=0; i<=DIRECTION_MAX;i++) {
                    
                    Direction *d = &directions[i];
                    
                    if (d->key == directionCommand) {
                        
                        LocationKey next = currentLocation->next[i];
                        
                        if ( next != BLOCKED ) {
                            
                            // Ok, we are walking here!
                            
                            currentLocation = &locations[next];
                            
                            // If we are holding a light then bring it with us
                            
                            if (lightInHand) {
                                
                                lightInHand->where=currentLocation;
                                
                                if (currentLocation == &locations[CENTER] ) {           // Center is special because it has no LED
                                    
                                    /// Taking as lite light to the center turns it off
                                    
                                    if (lightInHand->state != LIGHT_OFF) {
                                    
                                        lightInHand->state = LIGHT_OFF;
                                    
                                        plf( "There is something about this place - a palpable void of light-sustainability - that deeply affects you.  The light that you carry is inexplicably extinguished.");
                                    }                                    
                                }                                    
                                
                            }                                
                            
                            return;
                            
                        } else {
                            
                            plf("Sorry, you can't get thar from hear.");
                            return;
                            
                        }                                                        
                    }  // For i in directions
                                    
                }                
                
                plf("If you want to go that way, you will have to go without me.");
                               
            }
        
            break;

        case 'T':
            plf("1-Take");
            plf("2-Toss");
            plf("Take or toss?");
            
            switch (getCommand()) {
                
                case '1': {     // Take
                    
                        if (lightInHand) {
                                                        
                            // Already carrying one light
                            
                            plf("Do I look like a light sherpa?");
                            return;
                            
                        } else {                            
                            
                            Light *whichLight = pickAlight();
                            
                            if (whichLight) {
                            
                                if (whichLight->where == currentLocation) {
                                    
                                    lightInHand = whichLight;
                                    
                                    pf("You are now carring the ");
                                    p( whichLight->name );
                                    plf( " light.");                                    
                                    return;
                                    
                                } else {
                                    
                                    pf("There is no ");
                                    p( whichLight->name );
                                    plf( " light here.");                                    
                                    return;
                                    
                                }                                    
                                                                                                                
                            }  else {               
                            
                                plf( "You've been looking at lights too long.");
                                return;                             
                                
                            }                                
                        }                                                 
                    }                       
                    break;
                    
              case '2':  {        // Toss
                            
                        Light *whichLight = pickAlight();
                            
                        if (whichLight) {
                                
                            if (lightInHand==whichLight) {
                                    
                                lightInHand = NULL;
                                    
                                pf("You dropped the ");
                                p( whichLight->name );
                                plf( " light.");
                                return;
                                    
                            } else {
                                    
                                pf("You are not even carring the ");
                                p( whichLight->name );
                                plf( " light.");
                                return;
                                    
                            }
                                
                        }  else {
                                
                            plf( "That's not a light, bud.");
                            return;
                                
                        }
                    
                    }               
            
                    break;
                    
              }
              
              break;                     
        
        case 'U':
            plf("Which light do you want to use?");
            break;
        
        default:
            plf("I don't understand your accent.");
            break;
        
    };
    
}    

const char *oneLife(void) {

    plf("------");
    plf("Welcome to HEXWORLD, a Tiny Tile Adventure!");
    
    //put everything back the way it should be on the 1st day
        
    currentLocation = &locations[CENTER];                    // Always start at Meru
    lights[ LIGHTKEY_RED ].state=LIGHT_ON;
    lights[ LIGHTKEY_RED ].where=&locations[SE_BEACH];
    
    lights[ LIGHTKEY_GREEN ].state=LIGHT_ON;
    lights[ LIGHTKEY_GREEN ].where=&locations[NE_CITY];
    
    lights[ LIGHTKEY_BLUE ].state=LIGHT_ON;
    lights[ LIGHTKEY_BLUE ].where=&locations[NW_FOREST];
    
    
    dead = false;
        
    while (!dead) {
        
        nextStep();
                        
    };
        
    return "Enlightenment";
    
}            
    

void loop() {
    
    
  while (1) {
      
      oneLife();
      
    };
    
};    
        
                        
        
        
    

