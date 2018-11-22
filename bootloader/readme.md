This hex file is the output from the BlinkBIOS project.

https://github.com/Move38/Move38-BlinkBIOS

It is loaded in upper flash and takes care of lots of common hardware tasks so that individual games do not need duplicated code.

We program this file every time download a sketch. We also set the `BOOTRST` fuse so that the chip will jump to the bootload section rather than address 0x000 on RESET. Both of these happen in the `AVRDUDE` the recipes in `platform.txt`.
  