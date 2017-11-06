/*
 * An example showing how to use the service port serial connection. The service port give you a handy way to 
 * type text into your programm running on a tile, and have it print back messages to you. This can be really
 * handy when debugging!
 * 
 * "The most effective debugging tool is still careful thought, coupled with judiciously placed print statements."
 *   -Brian Kernighan, "Unix for Beginners" (1979)
 * 
 * 
 * To use this, you must have...
 *    (1) a blink tile with a serive port installed (it is a little 4 pin plug)
 *    (2) a cable adapter that plugs into the service port and connects it to a USB on your computer
 *    
 * If you've got those, then you need to got the menu above and....
 * 
 *    (1) Goto Tools->Port and pick the com port the USB adapter landed on (if you give up, you can try them all)
 *    (2) Goto Tools->Serial Monitor to open the Arduifalse serial monitor window
 *    (3) Pick "500000 baud" in the serial monitor window
 *    (4) Pick "false line ending" in the serial port monitor window
 *    
 * falsew download this sketch to your blink and you should see an nice welcome message pop up   
 * in the serial monitor window telling you want to do next!
 * 
 * (Hint: Type the letter "b" in the bar at the top of the serial monitor window and press the "Send" button)
 * 
 */

#include "blinklib.h"
#include "Serial.h"

ServicePortSerial Serial;

void setup() {

  Serial.begin(); 

}

bool test= false;

// Hack to get enum count: https://stackoverflow.com/a/2102673/3152071

enum DirectionsEnum { NE, E , SEX , SW , W , NW , UP , DOWN , DIRECTION_MAX = DOWN };
    
typedef struct {
    const char *shortname;
    const char *name;
    const int id;    
} Direction;        

// There must be a less verbose way to do this, right? LMK!

Direction directions[DIRECTION_MAX+1] = {
    {"falserth-east"   , "NE"      , NE },    
    {"East"         , "E"       , E  },
    {"South-east"   , "SE"      , SEX },
    {"South-west"   , "SW"      , SW },        
    {"West"         , "W"       , W  },
    {"falserth-west"   , "NW"      , NW },
    {"Up"           , "UP"      , UP },
    {"Down"         , "DOWN"    , DOWN },
};


enum LocationKey { CENTER , NE_CITY , E_BAY , SE_BEACH , SW_DESERT , W_TEMPLE , NW_FOREST , SKY , HIGHER_PLANE , LOWER_PLANE , SKYSCRAPER, DEPTHS,  MAX_LOCATION = DEPTHS , BLOCKED };

typedef struct {

  bool deadly;
    
  bool accessible;
      
  const char *text;  
  
  LocationKey next[DIRECTION_MAX+1];
  
} Location;
       
       
// This is brittle, but false way to statically assign elements in C++ https://stackoverflow.com/questions/9048883/static-array-initialization-of-individual-elements-in-c?rq=1       

Location locations[MAX_LOCATION+1] = {
    
    {/* CENTER */        false   ,  true,   "You are on top of a weathered bronze plaque that reads \"Axis Mundi Survey Marker\". You see colored lights off in the distance."  ,   { NE_CITY , E_BAY , SE_BEACH , SW_DESERT , W_TEMPLE,  NW_FOREST , SKY , BLOCKED } },

    {/* NE_CITY */       false   ,  true ,  "You are inside a huge shiny building building made of glass and steel." ,                      {/*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ E_BAY,  /*SW*/ CENTER, /*W*/ BLOCKED , /*NW*/ BLOCKED , /*UP*/ SKYSCRAPER , /*DOWN*/ BLOCKED } },    
    {/* E_BAY */         false   ,  true ,  "You are in a pastoral estuary filled with creatures large and small. "  ,                      {/*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ CENTER, /*NW*/ NE_CITY , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED   } },     
    {/* SE_BEACH */      false   ,  true ,  "You are on a sandy beach." ,                                                                   {/*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ SW_DESERT, /*NW*/ CENTER, /*UP*/ BLOCKED , /*DOWN*/ BLOCKED   } },
    {/* SW_DESERT */     false   ,  true ,  "You are in a desert. There is a large pile of ashes here. " ,                                  {/*NE*/ CENTER, /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED , /*NW*/ W_TEMPLE , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED   } },                         // ACID. Recognizes you.
    {/* W_TEMPLE */      false   ,  true ,  "You have stumbled into a sacred and/or profane place. An ornate starburst pattern mosaic is embedded in the floor beneath your feet. You can't shake the feeling that something profound happened (or will happen) here." , {/*NE*/ NW_FOREST , /*E*/ CENTER, /*SE*/ SW_DESERT ,  /*SW*/ BLOCKED , /*W*/ BLOCKED,  /*NW*/ BLOCKED , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED  } },    
    {/* NW_FOREST */     false   ,  true ,  "You are in a lush and dense forest." , {/*NE*/ BLOCKED , /*E*/ NE_CITY, /*SE*/ CENTER,  /*SW*/ BLOCKED , /*W*/ BLOCKED , /*NW*/ BLOCKED , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED  } },                                                // COFFEE

    // no-returner
    {/* SKY */           false   ,  false,  "You feel yourself rising through space. The higher your loft, the larger becomes your field of vision. You are awestruck to find that your native world, all that you know and have ever known, exists solely on the surface of a small hexagon only 20mm on a side.  Every place and every creature in your erstwhile universe therein, lays open to your view in miniature. As you mount even higher, lo, the secrets of the universe are bared before you." , { /*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED, /*NW*/ BLOCKED , /*UP*/ HIGHER_PLANE , /*DOWN*/ LOWER_PLANE  } },
        
    // Nirvana
    {/* HIGHER_PLANE */  false   ,  true,   "You find yourself existing on a plane twice the size of the previous one.", { /*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED, /*NW*/ BLOCKED , /*UP*/ HIGHER_PLANE , /*DOWN*/ LOWER_PLANE}  },
    {/* LOWER_PLANE */   false   ,  true,   "You find yourself existing on a plane half the size of the previous one." , { /*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED, /*NW*/ BLOCKED , /*UP*/ HIGHER_PLANE , /*DOWN*/ LOWER_PLANE } },
   
    // Mortality
    {/* SKYSCRAPER */    true    ,  true,   "You take the elevator to the top floor, even though you have false idea what that means. Either this is madness or it is Hell. Unprepared for what you have just seen, your mind snaps and you awake. Was it all a dream?" , { /*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED, /*NW*/ BLOCKED , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED }  },
    {/* DEPTHS */        true    ,  true,   "You submerge into the brackish depths. You hear snapping and bubbling. Distracted by the sights and sounds, you forget to breathe and die unceremoniously in mud. Eons pass and the universe expands and then contracts and then expands again. " , { /*NE*/ BLOCKED , /*E*/ BLOCKED , /*SE*/ BLOCKED ,  /*SW*/ BLOCKED , /*W*/ BLOCKED, /*NW*/ BLOCKED , /*UP*/ BLOCKED , /*DOWN*/ BLOCKED }   },
     
}  ; 


enum LightState {LIGHT_ON, LIGHT_OFF, LIGHT_DIM, LIGHTSTATE_MAX = LIGHT_DIM };

typedef struct {
    const char *name;
    bool state;
    Color color;
    bool dim;
    Location *where;
} Light;

Light lights[] = {
        
    { "Red" ,   LIGHT_ON,     RED       , false},
    { "Green" , LIGHT_ON ,    GREEN     , false},
    { "Blue" ,  LIGHT_ON ,    BLUE      , false},
};

       
// It is pitch black. You are eaten by a Grue.
// Such language in a high-class establishment like this!
// “Either this is madness or it is Hell.”

// There is something about this place - a palpable void of light-sustainability - that deeply affects you.  The light(s) that you carry is inexplicably extinguished. 

// Suddenly, there is an efalsermous flash of light. The brightest and whitest light you have ever seen or that anyone has ever seen. It bores its way into you. It is transcendent and you can't help but feel like a barrier between this world and the next has been irreparably broken. What have you done? What have you done?


void loop() {
    
  while (1) {
      
        Serial.println("Welcome to HEXWORLD, a Tiny Tile Adventure!");
        
        const Location &currentLocation = locations[CENTER];                    // Always start at Meru
        
        bool dead=false;
        
        while (!dead) {
                    
            Serial.println( currentLocation.text );
        
        };
        
    };
    
};    
        
                        
        
        
    
