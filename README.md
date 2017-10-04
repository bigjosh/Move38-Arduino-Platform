# Move38 Blinks for the Arduino IDE
An Arduino core for the Blinks gaming tile. More info at...
http://move38.com


This core requires at least Arduino IDE v1.6.2, where v1.6.5+ is recommended. <br/>
* [Programmers](#programmers)
* **[How to install](#how-to-install)**
	- [Boards Manager Installation](#boards-manager-installation)
	- [Manual Installation](#manual-installation)
* **[Getting started](#getting-started)**

* **[Digging deeper](#digging-deeper)**
	- [Hardware Abstraction Layer](#hardware-abstraction-layer)

## Programmers

Since there is no bootloader in a tile, all code must be programmed rather than downloaded.

You can use any AVR programmer supported by AVRDUDE and the Arduino IDE.

Just connect your programmer and select it in the "Programmers" menu, and connect the 6-pin ISP to the tile. If you do not have a programmer, you can also use an Arduino and a couple of wires. More detailed instructions to come. 

You can then use the "Sketch-Upload to Programmer" menu choice or just press the Play button to compile your code and program it into the connected tile. (Both the menu option and the button do the same thing with a tile.)

## How to install
#### Boards Manager Installation
This installation method requires Arduino IDE version 1.6.4 or greater.
* Open the Arduino IDE.
* Open the **File > Preferences** menu item.
* Enter the following URL in **Additional Boards Manager URLs**:

    ```
    https://bigjosh.github.io//package_index.json
    ``` 

* Open the **Tools > Board > Boards Manager...** menu item.
* Wait for the platform indexes to finish downloading.
* Scroll down until you see the **Move38** entry and click on it.
  * **Note**: If you are using Arduino IDE 1.6.6 then you may need to close **Boards Manager** and then reopen it before the **Move38** entry will appear.
* Click **Install**.
* After installation is complete close the **Boards Manager** window.


#### Manual Installation
You should probably only  do manual install if you plan on hacking the blinks API and contributing changes back up to the master repo on Github. 

Click on the "Download ZIP" button in the upper right corner. Extract the ZIP file, and move the extracted files to the location "**~/Documents/Arduino/hardware/Move38-manual/avr**". Create the folder if it doesn't exist. This readme file should be located at "**~/Documents/Arduino/hardware/Move38-manual/avr/README.md**" when you are done.

Open Arduino IDE, and a new category in the boards menu called "Move38-manual" will show up.

##### Notes 

* We called the "vendor/maintainer" folder `Move38-manual` so that you can also use the boards manager and you will be able to tell the two apart in the boards menu.

* You must manually create the `avr` folder and you must also manually move the files out from this repo into this folder. We could not automatically have the folds inside the repo match the Arduino required folder layout because in in the boards manager, the architecture is in the JSON file rather than the folder structure. Arg. 

* The "**~/Documents/Arduino/hardware/Move38-manual/avr**" folder is a Git repo and is also set up for easy editing in Atmel Studio with a solution inside the `\AS7` sub-folder. 

## Getting started with Move38 Blinks on Arduino

* Open the **Tools > Board** menu item, and select `Blinks` from the `Move38` submenu.
* Select what kind of programmer you're using under the **Programmers** menu.
* Select "File->Examples->Examples for Blink Tile" and choose "HelloBlink".
* Hit the Play button.

The IDE should compile the code and program the Blinks tile... and you should see pretty blinking lights!

### Digging Deeper

#### Hardware Abstraction Layer

Most programmers will want to use the high level `blinks` API, but if you want to get closer to the hardware you can directly call into the `HAL` (Hardware Abstraction Layer) that the `blinks` API is built on top of. Documentation for this layer is in the [README.md](cores/blinks/README.md) in the `cores/blinks` folder.