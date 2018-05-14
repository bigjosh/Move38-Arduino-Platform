# Move38 Blinks for the Arduino IDE
An Arduino core for the Blinks gaming tile. More info at...
http://move38.com

## Roadmap

### [Getting started](Getting-started.md)

Covers installing this repo so that you can write and download code into a Blinks tile using the Arduino IDE. 

Ends with loading a `HelloBlink` program it a tile.

### Writing games

The best way to start writing games is to work your way though the examples in the "File->Examples->Examples for Blink Tile" menu in the Arduino IDE (after you have installed this repo as described in the Getting Started above).  
  

### [Service Port](Service Port.MD)

Describes the service port connector on each blink. Lets you add `print` statements to your programs, which can be very helpful during development.  

### Digging Deeper

### API Layers

The blinks hardware can do incredible things, and you can have unfettered access to it at any level you want. This documents describes those layers from bare metal up.  



#### Hardware Abstraction Layer

Most programmers will want to use the high level `blinks` API, but if you want to get closer to the hardware you can directly call into the `HAL` (Hardware Abstraction Layer) that the `blinks` API is built on top of. Documentation for this layer is in the [README.md](cores/blinks/README.md) in the `cores/blinks` folder.