# Move38 Blinks for the Arduino IDE
An Arduino core for the Blinks gaming tile. More info at...
http://move38.com

## Roadmap

### [Getting started](Getting-started.md)

Covers installing this repo so that you can write and download code into a Blinks tile using the Arduino IDE. 

Ends with loading a `HelloBlink` program it a tile.

### Writing games

The best way to start writing games is to work your way though the examples in the "File->Examples->Examples for Blink Tile" menu in the Arduino IDE (after you have installed this repo as described in the Getting Started above).  
  

### Digging Deeper

#### Hardware Abstraction Layer

Most programmers will want to use the high level `blinks` API, but if you want to get closer to the hardware you can directly call into the `HAL` (Hardware Abstraction Layer) that the `blinks` API is built on top of. Documentation for this layer is in the [README.md](cores/blinks/README.md) in the `cores/blinks` folder.