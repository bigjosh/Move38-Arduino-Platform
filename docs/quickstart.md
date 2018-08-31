# Blinks Quick Start Guide


## What’s Included
- 6x Blinks
  - 6x custom PCBs,
  - 6x 3D printed bases  
  - 6x 12 polarity aligned magnets
  - 6x CR2032 coin cell batteries
  - 6x battery/magnet security screw
  - 1x Carry Case (magnetic enclosure for carrying 6 Blinks)


- Programmer
  - 1x AVR Pocket [AVR programmer](https://www.sparkfun.com/products/9825) (AKA USBtinyISP)
  - 1x USB mini cable (3’)


- Service Port (used for Serial Monitor)
  - 1x Blinks Serial Adapter
  - 1x FTDI board (Red w/ usb on one side and 6 pins on the other)
  - 1x Molex cable (4 black wires)
  - 1x USB mini cable (6”)


## Needed Software
- Arduino IDE - [Download it here](https://www.arduino.cc/en/Main/Software)
- Blinks Library - [Download it here](https://github.com/Move38/Move38-Arduino-Platform/zipball/master)

*If you want to have the latest and contribute to the development of Blinks, here’s our [Github Repo](https://github.com/Move38/Move38-Arduino-Platform/tree/dev)


## Installing the Blinks Library

After you have installed the Arduino IDE (> 1.8.5) and launched the application, you will notice that Arduino has created a folder in your `Documents` folder, aptly named, `Arduino`. Follow these next 5 steps to have the Blinks library installed:


1. Create a folder called `hardware` inside of the Arduino folder `~/Documents/Arduino/hardware` (it will be next to ‘libraries’, not nested inside) and one more called `Move38-Blinks-Library` inside of the hardware folder you just created. `~/Documents/Arduino/hardware/Move38-Blinks-Library`
2. Download the Blinks Library if you haven’t already done so
3. Unzip the Blinks Library into a folder called `avr`
5. Move the Blinks Library into the `Move38-Blinks-Library` folder (the path will be `~/Documents/Arduino/hardware/Move38-Blinks-Library/avr/`)
6. Restart Arduino (quit and relaunch)

If you click on `Tools` in your menu bar, and navigate down to `Board:`  you should now see `Move38: Blinks Tile` at the bottom. Select it.

Next, click on  `Tools` in your menu bar, and navigate down to `Programmer:`  select the programmer `USBTinyISP` or see below for faster upload time with `Blinks Programmer`

Now if you go to `File/Examples`, you should see Examples for Blinks Tiles at the bottom of that list as well. Open the first one up and see if it compiles.



## Transfer code to Blink

![Image of Transferring Blink Code](assets/transfercode.jpeg)

1. Connect the USB programmer
2. Place the magnetic programming jig on top of the Blink
3. Press `Upload` to transfer

Not successfully transferring to your Blinks? try these [troubleshooting tips from the forum](http://forum.move38.com/t/error-the-selected-serial-port-does-not-exist-or-your-board-is-not-connected/79/3?u=jbobrow)


## Windows Troubleshooting

No programmer detected?
Check to make sure you have the correct [driver installed](https://learn.adafruit.com/usbtinyisp/drivers), relaunch the Ardunio IDE and then you should be able to select USBTinyISP from the Tools > Programmer drop-down in Arduino.

For the extra curious, here is the [source code](https://github.com/sparkfun/Pocket_AVR_Programmer/tree/master/Drivers) for that driver.


## *Faster Upload Time
1. Download [this](https://move38.com/attic/blinks/programmers.txt) file (right click and save link as)
2. Move file to `/Applications/Arduino.app/Contents/Java/hardware/arduino/avr` (it will replace the existing file here)
3. Restart Arduino if it was open
4. Now select the `Blinks Programmer` when programming and it should be roughly 4x faster upload speeds ⚡

Not seeing `Blinks Programmer`? Try out [these steps](http://forum.move38.com/t/faster-upload-time-directory-change-and-modified-programmers-txt/83), found by Alpha Blinks Dev, Ken 🙂


## Using the Blinks Service Port to talk to or listen to your Blink

The Service Port is by no means necessary, but is a phenomenal tool for debugging code and refining early sketches into robust games. The following steps show you how to use your Blinks Service Port.

1. Put together the components of your Blinks Service port so they look like this
  1. USB Cable → FTDI → Blinks Serial Adapter → Molex Cable → Blink (w/ Serial port)
  2. <photo of the above>
2. Open Arduino
3. click on `Tools` in your menu bar, and navigate down to `Port:`  you should now see `COM3` on a PC and `/dev/cu...` on a Mac. Select it.
4. press the magnifying glass in the top right corner of the Arduino IDE to open the Serial Monitor

Note: The serial port is at a fixed 500k baud rate (set it in the drop-down).

To write to the Serial port from a sketch,

1. Add “#include Serial.h”
2. Instantiate a ServicePortSerial class, like this `ServicePortSerial sp`
3. Call the begin method within setup(), like this `sp.begin()`

Here is the simplest of ServicePort sketches:


## Unboxing And Walkthrough Video

{% include youtubePlayer.html id='UA1Vl7x3Y7g' %}

And last but not least, [here’s a handy little unboxing video](https://www.youtube.com/watch?v=UA1Vl7x3Y7g) that will walk you through some basics!
