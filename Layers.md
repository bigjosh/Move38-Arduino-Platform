## Layers

### 1. Cold power up

This typically only happens when a new battery is installed or when the foreground software locks up.

The chip resets all the ports according to the the datasheet and then jumps to the reset vector, which typically points to the GCC bootstrap code. This code initializes global variables and does a bit of hardware setup. 

Here is an example on how to run code before the bootstrap (very esoteric)...

https://github.com/bigjosh/Ognite-firmware/blob/master/Atmel%20Studio/Candle0005.c#L440

### 2. main()

After all the C setup is complete, the bootstrap jumps to the `main()` function. This function is in `cores/blinkcore/main.c`

Here we...

1. Initialize the binks hardware systems and turn them on.
2. Jump to the `run()` function.
3. Re-run `run()` forever if it returns.
 
The blinks `main()` function is weakly defined, so you can make your own if you want to take over before the blinks hardware gets started up. 

### 3.  run()

Once all the hardware is up and running, then `run()` function takes over. This code lives in `libraries\blinklib\src\blinklib.cpp`.

Here we...

1. Call the user-supplied `setup()` function.
2. Repeatedly...
	a. Clears the cached `millis()` snapshot
    b. Calls the user-suppled `loop()` function
    c. Atomically updates the values shown on the LEDs to match any changes made in `loop()`       
    d. Processes new IR data that has been recieved, and potentially sends IR data. 


Then `run()` function is weakly defined, so you can override it if you want more control over these. 
 
### 4. `setup()` and `loop()`

These are the idiomatic entry points provided by Arduino sketches. 

`setup()` is called once, typically right after a tile has had a battery change.
 
`loop()` is called at the current frame rate, which is currently about 18 times per second. You `loop()` function should complete before the next frame starts, so should not have any blocking code in it. The tile sleeps for the time between when `loop()` returns and the next frame, so the faster you can complete your work and return from loop, the longer batteries will last.

 
 