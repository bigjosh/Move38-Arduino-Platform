To save valuable flash space and to have more control, we replace the normal C startup code with our own. 

Our `startup.S` lands at the first executable location in flash which will either be at `0x000` or `0x3800` depending on the setting of the `BOOTRST` fuse. For normal blinks this startup code will live at `0x3800` in the bootloader memory section, but could potentially also live at 0x0000 if you wanted to write plain programs without a bootloader maybe for development or exotic stuff.   

At the begining of `startup.S` we define the first 16 interrupt vectors. There are a total of 27 vectors on this chip, be the highest one we even enable is 16 so we can reclaim the rest and use that flash memory ourselves.

On power up, the CPU jumps to the first vector, which is a jump to `__init` which is the first (and only) line of our startup code that sets the `zero_reg` (R1) to `0`. We do not do all the other stuff that the normal startup code did like init the stack pointer becuase it is unnecessary on this chip. 

After our `__init` runs, it falls though to the normal C init code that sets up initialized variables to zero and copies initialized variables from flash. It also runs any C++ static constructors. In the bootloader code we avoid all these to save space, and should only need the code that sets the uninitialized variables to `0` (`bss section`).

Then we fall into the `mainx()` function which increases the processor speed to 8Mhz and then calls the `run()` function. This is the entry point to whatever compiled program we have, which could either be a game or the bootloader. 

For now, each independently sets up its own interrupts if it wants and then runs. Eventually the bootload will keep the interrupts and pass control to the game with pixel, IR, and button all handed in the background.

