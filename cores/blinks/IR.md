### IR Communications Protocol

Each of the 6 edges of a blink tile has an IR LED to communicate with its adjacent neighbor.

#### Physical layer

Each single IR LED is used for both transmit and receive. The LED pins are connected directly to GPIO pins on the MCU.

For transmitting, the LED is briefly driven with a forward voltage to flash it. These flashes are brief to save power. 

For receiving, the LED is negatively biased to charge it up like a capacitor. Light falling on the LED makes it discharge more quickly, and by timing how long it takes to discharge we can measure the integral how much light is hitting it. [This technique](https://www.sparkfun.com/news/2161) was originally described by Forest Mimms III.

We've chosen our LED to...

* quickly reliably discharge below the digital trigger voltage of our MCU input pins when exposed to a >10us flash from a corresponding transmitting LED at a distance of about 1mm,

AND
 
* take >1ms to discharge in ambient light.

When ever an LED discharges below the digital threshold voltage on the connected pin, an interrupt handler quickly recharges the LED to be ready for the next trigger.


#### Idle windows

We use a ~1.2ms idle window to space events int the protocol. This is long enough to contain a full symbol, but short enough that we are unlikely to see a trigger within a single idle window following a charge.  


#### Symbols

Our two symbols are a 0-bit and a 1-bit.

We represent a 0-bit as two flashes about 100us apart, followed by one idle window. 

We represent a 1-bit as three flashes about 100us apart, followed by one idle window. 

Keep in mind that the LED capacitance is always discharging due to ambient light, so a single discharge can either be due to a flash, or just due to ambient light. The multiple flashes in our symbols are close enough together that the resulting triggers on the receiving LED could not be the result of normal ambient light.


#### Preamble 

A preamble consists of...

1. An idle window
2. A single flash
3. An idle window
4. A single flash
5. An idle window

  
#### Purpose of the preamble

If we only sent symbols, occasionally we might send a symbol just after the receiving LEDs happened to have triggered due to ambient light. In this case, we might mistakenly see the 2 flashes of a 0-bit symbol as 3 flashes of a 1-bit symbol (or read the 4 flashes of a 1-bit as noise). 

The preamble avoid this by making sure that every symbol is always preceded by a flash and one idle window. This ensures that the receiving LED is charged when the symbol is sent, and so the receiving LED will only trigger due to a flash and not ambient light. 

We always send 2 single flashes to because if we only sent one, then it is possible that the single flash could have been immediately preceded by a spontaneous ambient light trigger, and we would see the combination of the ambient trigger and the flash as a 0-bit symbol. 

#### Frames

All data is transmitted inside of frames. 

Each frame starts with a preamble and is followed by 9 symbols representing 8 data bits followed by a parity bit. 

#### Interpretation of single flashes

We interpret each received single flash as a start-of-frame marker.

The means that we might begin receiving a frame after a spontaneous ambient light trigger. We depend on error detection to reject this case before it is interpreted as a valid frame.

We will also often begin receiving a frame after the first flash in the preamble. This case is benign since the 2nd preamble flash will restart the frame. 

#### Error detection

We primarily depend on the fact that the trigger patterns of the two valid symbols are very unlikely to appear due to noise. Ambient light creates single triggers separated by more than an idle window. Noise (like direct sunlight) creates many rapid triggers between idle windows (if idle windows appear at all). 

We also have secondary tests for correct frame length and parity bit for further reduce the chance of receiving corrupt data. These help protect mostly from a series of missed flashes resulting in a missed symbol, or a single missed flash making a 1-bit symbol look like a 0-bit symbol.             



 


  

   

    

These functions provide low level access to the hardware. The `Blinks` API is built on top of these functions and provides a higher level view more that is more appropriate for most game developers.

Reasons to use this HAL:

1. You want to do something not supported in the higher level API. In this case, you can include the `.h` file for the subsystem you need direct access to directly in your sketch, but you have to be careful not to step on the toes of the other API calls. 
2. You want to make your own higher programming model. Wish you could program your Blinks using an SmallTalk actor based model? Or an ELM functional state transformation model? You can build is from scratch starting with these functions!    


#### Conventions
  

Each subsystem has a `.h` file and a `.cpp` file. 

Most subsystems offer these base functions...

`subsystem_init()` - On time hardware initialization run at power up.
`subsystem_enable()` - Start operation. Called after either `_init` or `_disable`
`subsystem_disable()` - Stop the subsystem to save power. Called after `_enable`   


#### Disabling interrupts

Both the IR communications and the display RGB LED systems are very timing sensitive, so it is important that interrupts never be turned off for too long.   

The maximum allowed interrupt latency is dependent on clock speed and other factors that are still changing, so I can't give you a hard answer now- but in general try to only turn off interrupts for a few instructions. If you need atomicity longer than that, you a higher level semaphore or something like that. 

#### Asynchronous Callbacks

To support asynchronous callbacks without impacting interrupt latency, the HAL implements a pseudo interrupt system where the hardware interrupts are intercepted by the HAL and then re-dispatched to the callback functions.

Interrupts are enabled while callback functions are running, but only one instance of a callback can be active at a time. If another asynchronous event occurs while a callback is already running, the new callback is held until the running callback completes. Multiple events that would trigger the same callback are batched into a single call.

This means that...

2. Even though interrupts are enabled,  you callback will never be interrupted by another copy if itself
2. If another trigger events happens while your callback is running, it will get called again right after the running copy returns
3. If multiple trigger events happen while your callback is running, you may only get called once more when your currently running callback returns.
4. If a triggering event occurs, your callback will always get called, even if the trigger disappears before the callback is invoked. 
4. You should always write callbacks so they can deal with being called even if the triggering event is no longer present.    
5. Any variables that are accessed only within your callback do not need to be `volatile` since there will only ever be one copy of the callback running.
6. Any variables that are access by the foreground should be declared as `volatile` from the foreground process's point of view since these could be updated asynchronously by the callback. 
7. Any variables that are shared between different callbacks should be declared as `volatile` from the reading callback's point of view since that callback could be interrupted by another callback.  

#### The DEBUG subsystem

It can be hard to debug code on a little board with no screen or keyboard. The `DEBUG` subsystem gives you at least a couple of bits of IO to interact with your program during debugging. 

To use the `DEBUG` subsystem you must `#define DEBUG` a the top of your source file(s). 

The debug system uses pins #6 and #19 for communications. Hopefully these will be broken out on future PCB versions.  
 
 
#### Other files

`Arduino.h` is automatically included in all Arduino sketches and defines some of the the Arduino API functions that make sense on Blinks. 
  
`WMath.cpp` is taken from the Arduino core. It provides wrappers for some `random()` functions from `stdlib` and the `map()` function.
 