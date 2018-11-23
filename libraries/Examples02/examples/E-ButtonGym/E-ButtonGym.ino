/*
 * 
 * Exercise the button functions to see how they work and 
 * and also show off osme fancy C++ footwork!
 * 
 * Each of the button events lights one of the pixels 
 * so you can see that it happened. Check `indicators` for the
 * list of events and what pixel/color they will show up as.
 * 
 */


void setup() {

  // No setup needed for this simple example!  

}


class fadingIndicator_t {

    bool (*testFunction)();
  
    inline void trigger(void) {
  
      brightness = 255;
  
    }
    
    inline void fade(void) {
  
      if (brightness) brightness--;
  
    }
  
  
  
  public: 
  
    byte brightness;
  
    byte face;
  
    Color color;
  
    inline void update(void ) {
  
      if ( (*testFunction)() ) {
  
        trigger();
        
      } else {
        fade();
      }
   
    }

    fadingIndicator_t( byte face, Color color , bool (*testFunction)() ) :  brightness(0) , face(face), color(color) , testFunction( testFunction ) {}
      
};

fadingIndicator_t indicators[] {

  { 0 , RED    , buttonPressed },
  { 1 , GREEN  , buttonReleased },
  { 2 , BLUE   , buttonSingleClicked },
  { 3 , MAGENTA, buttonDoubleClicked },
  { 4 , YELLOW , buttonMultiClicked },
  { 5 , CYAN   , buttonLongPressed },
    
};


void loop() {
  
  for(auto &item : indicators ) {

    item.update();

    setFaceColor( item.face , dim( item.color , item.brightness ) );
 
  }

}
