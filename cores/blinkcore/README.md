### Hardware Abstraction Layer Core Files

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
  