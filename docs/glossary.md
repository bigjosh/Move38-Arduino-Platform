# Glossary
[Blinks API Reference](api.md) - full API reference

[API Source Code - absolute truth](https://github.com/Move38/Move38-Arduino-Platform) - (would I lie about something like that?)

## Display
    setColor(color)
    
    setFaceColor(face, color)


## Colors
```
makeColorRGB(red, green, blue) - values[0-255]
makeColorHSB(hue, saturation, brightness) - values[0-255]
dim(color, value) - values[0-255]    

#RED    
#ORANGE 
#YELLOW 
#GREEN   
#CYAN    
#BLUE    
#MAGENTA 
#WHITE   
#OFF
```    

## Button
``` 
buttonPressed()
buttonReleased()
buttonSingleClicked()
buttonDoubleClicked()
buttonMultiClicked()
buttonClickCount()
buttonLongPressed()
buttonDown()
```

## Communication
``` 
 setValueSentOnAllFaces(value)
 setValueSentOnFace(value, face);
 getLastValueReceivedOnFace(face);
 isValueReceivedOnFaceExpired(face);
 didValueOnFaceChange(face);
 isAlone();

 ```
    
  

## Time
``` 
 millis() - monotically incrementing timer
 Timer.set(duration)
 Timer.isExpired()
```

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
 
```
*FOREACH_FACE(f) { }
*COUNT_OF(array);
*FACE_COUNT 6
*define MAX_BRIGHTNESS 
```

    – the asterisk signals that this function is syntactic sugar… i.e. you can get the same information by means of the functions included, but it is such a common occurrence that we include it for everyone’s benefit.


# Coming Soon…

## Collaboration -- Time & Communication
```
	Metronome.start(period)
    Metronome.stop()
    Metronome.didTick()
    Metronome.getPhase()
    
    Step.next()
    Step.getHighestSeen()
    *Step.didStep() - returns number of times stepped… since
```

