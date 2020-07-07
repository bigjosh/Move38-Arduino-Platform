Do not manually change these files!

They come from the `blinkbios` and must travel as a coherent set. 

`BlinkBIOS.hex` contains the compiled image of the BlinkBIOS code. It is loaded in upper flash and takes care of lots of common hardware tasks so that individual games do not need duplicated code. 

We program this file every time download a sketch. We also set the `BOOTRST` fuse so that the chip will jump to the bootload section rather than address 0x000 on RESET. Both of these happen in the `AVRDUDE` the recipes in `platform.txt`. 

To be able to successfully talk to the BIOS code, the blink user code must be compiled against the matching header files.

All communications between user code the bios happens though the shared memory blocks defined in these headers. Fields marked as bios-volatile can be updated in background b the bios. 
 
