# AS7

This directory holds an Atmel Studio 7 Solution and Project with all the firmware files.

All files except for `sketch.cpp` are included as links to the arduino directories, so any updates you make in AS7 will be reflected in the arduino structure.

The `skecth.cpp` file is a stub of an arduino-style sketch. You can write code where and then copy everything except the `#include "Arduino.h" into an arduino sketch.

The `boards.txt` does not have any boards. It is just here so that the Arduino IDE does not complain when it sees this `AS7` directory as a platform. Otherwise we would need to manually copy files into the `hardware` tree from an AS7 directory. With multipule copies of files things get messy, so worth this hack here especially since it does not impact normal people who use the boards manager to install.

I much prefer working in AS7 to the Arduino IDE so it was worth the effort to make this. 
