# Blinks API Reference
[Glossary](glossary.md) - glanceable reference

[API Source Code - absolute truth](https://github.com/Move38/Move38-Arduino-Platform) - (would I lie about something like that?)

## Display
    void setColor(color);
    // sets all 6 pixels on all faces to the same color
    
    void setFaceColor(face, color);
    // sets a single pixel on a face to this color


## Colors

    Color makeColorRGB(red, green, blue);
    // R, G, and B values [0-255]
    
    Color makeColorHSB(hue, saturation, brightness);
    // H, S, and V values [0-255]
    
    Color dim(color, value);
    // returns the color passed in a dimmer state 
    //(31 levels of brightness)
    
    #define RED     makeColorRGB(255,   0,   0)
    #define ORANGE  makeColorRGB(255, 127,   0)
    #define YELLOW  makeColorRGB(255, 255,   0)
    #define GREEN   makeColorRGB(  0, 255,   0)
    #define CYAN    makeColorRGB(  0, 255, 255)
    #define BLUE    makeColorRGB(  0,    , 255)
    #define MAGENTA makeColorRGB(255,   0, 255)
    #define WHITE   makeColorRGB(255, 255, 255)
    #define OFF     makeColorRGB(  0,   0,   0)


## Button
    /*
     *  All button handling is done with flags, so when
     *  you call a function, it returns the value of 
     *  the flag (i.e. whether or not that action 
     *  has occured) and only when you have  
     *  called the function will it reset 
     *  the flag to false
     */
    
    bool buttonPressed();
    // a flag is set to true on the change from button up to button down
    // buttonPressed() returns that flag and sets it back to false
    
    bool buttonReleased();
    // a flag is set to true on the change from button 
    // down to button up buttonReleased() returns that
    // flag and sets it back to false
    
    bool buttonSingleClicked();
    // a flag is set to true when the the 
    // button goes from down to up
    // only once in under 330ms. The flag is set exactly 
    // 330ms after the button is up
    // buttonSingleClicked() returns that flag and sets it back to false
    
    bool buttonDoubleClicked();
    // returns true when a double click has been recorded 
    //(down-up-down-up under 330ms*)
    // ^ also a flag with the same properties as above
    
    bool buttonMultiClicked();
    // returns true when multiple click has been recorded 
    // (down-up...down-up under 330ms*)
    // ^ also a flag with the same properties as above
    
    byte buttonClickCount();
    // returns the number of clicks recorded during button multi clicking
    
    bool buttonLongPressed();
    // returns true when the button has been down for more than 3000ms (3 seconds)
    // ^ also a flag with the same properties as above
    
    bool buttonDown();
    // returns true when the button is down
    // ^ also a flag with the same properties as above


## Communication
    void setValueSentOnAllFaces(value);
    // sets a value (0-63) to be sent 
    // repeatedly (every loop() cycle) on all 6 faces
    
    void setValueSentOnFace(value, face);
    // sets a value (0-63) to be sent repeatedly (every loop() cycle) 
    // on the face specified
    
    byte getLastValueReceivedOnFace(face);
    // retreives the last value seen on the 
    // specified face even if expired
    // (i.e. last message from neighbor)
    // defaults to 0 on startup
    
    bool isValueReceivedOnFaceExpired(face);
    // returns true if the message from a 
    // neighbor has expired
    // (i.e. it's been longer than 200ms since we last 
    // received a message from this neighbor)
    
    bool didValueOnFaceChange(face);
    // returns true if the value on this face is different 
    // from the last stored seen value on this face
    
    bool isAlone();
    // returns true if all faces have expired values
    // (i.e. it is safe to assume this Blink is now alone)
    


## Time
    unsigned long millis();
    // monotonically incrementing timer
    // remains constant in a single pass of loop()
    // i.e. millis() will return the same value at the top 
    // of loop() and the bottom does not
    // increment while asleep
    // resets after ~50 days
    
    
    void Timer.set(duration);
    // a timer that will expire after duration milliseconds
    // used like this:
    // Timer myTimer;
    // myTimer.set(millisTilExpired);
    
    bool Timer.isExpired();
    // a timer remains expired once it become 
    // expired until set again or if
    // a timer is set to NEVER, it will never expire


## Types
    byte
    word
    int
    long
    float
    double
    bool
    Color
    Timer


## Convenience
    FOREACH_FACE(f) {
      // f increments from 0-5 (i.e. each face)
    }
    
    COUNT_OF(array);
    // pass this function an array and it will returns number of elements in the array


## Constants
    #define FACE_COUNT 6
    // 6 for the number of faces the current Blinks have 
    // (maybe a new tesselation for heptagons will be found and we'll come out with a new version, but until then, six.)
    
    #define MAX_BRIGHTNESS 255
    // 255 for the highest brightness level displayed
    // Note: brightness actually only has 31 different values,
    // however, we scale them to 0-255 to match a familia 8-bit color scheme 
    
    #define NEVER ( (uint32_t)-1 )
    // If you would like the timer to effectively never expire, set it to this constant
    
    #define SERIAL_NUMBER_LEN 9
    // Length of the globally unique serial number (9 bytes long)


## Uniqueness
    uint16_t rand( uint16_t limit );
    // Return a random number between 0 and limit inclusive.
    
    byte getSerialNumberByte( byte n );
    // Read the unique serial number for this blink tile
    // There are 9 bytes in all, so n can be 0-8
----------
# Coming Soon…

Follow or contribute to the progress on Move38-Arduino-Platform dev branch

## Collaboration 

Time & Communication

    Metronome.start(period)
    Metronome.stop()
    Metronome.didTick()
    Metronome.getPhase()
    
    Step.next()
    Step.getHighestSeen()
    *Step.didStep() - returns number of times stepped… since
