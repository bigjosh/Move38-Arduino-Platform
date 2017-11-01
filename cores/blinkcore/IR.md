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


##### DDR and PORTs of IR LEDs during different states


| State | Anode DDR | Anode PORT | Cathode DDR | Cathode PORT | Notes |
| --- | --- | --- | --- | --- | --- |
| Charging | High (driving) | Low | High (driving) | High  | Led reverse biased |
| Listening | High (driving) | Low | Low (input) | Low (no pull-up) | Cathode sees input go low when charge depleted |
| Sending | High (driving) | High | High (driving) | Low | Normal forward voltage lights LED |

Note that Anode DDR is always driven so we never touch that after enabling IR system. 


#### Timing windows


The timing window must be short enough that spontaneous triggers from ambient light are unlikely. 

The timing window must be long enough to contain the longest symbol, with an additional margin to account for the maximum difference between the sender and receiver clocks. That is, if the sender's clock is slow and the receiver's clock is fast, the maximum symbol sent by the sender must still be shorter than a single time window as measured by the receiver's clock. 

Currently the timing window is specified as 600us by `IR_WINDOW_US` in `ir.c` . 


#### Flashes

All communication is a result of the receiver detecting flashes sent by the sender. All information in encoded in the timing between consecutive flashes. Flashes are digital - you either get them or not.

Longer flashes are easier to detect, but if too long a single flash can be misinterpreted as multiple shorter flashes if the flash takes longer than it takes for the receiver to detect it and then recharge to be ready for the next one.      

The minimum detectable flash length depends on the alignment of the receiving and sending LEDs and the battery voltage levels.  

The minimum time between consecutive flashes depends on the time it takes for the receiver to detect a flash and recharge the LED to be ready for the next one. This in turn mostly depends on the receive interrupt latency. 
  
The length of a flash is currently defined as 10us by `IR_PULSE_TIME_US` in `ir.c`.

The time between consecutive flashes is currently defined as 100us by `IR_SPACE_TIME_US` in `ir.c`.

Note that the current space time is conservative and can likely be shortened once maximum interrupt latency for the whole system is measured.  

#### Idle windows

A timing window with no flashes transmitted is an idle window. 


#### Sync 

A single flash followed by an idle window is a sync. Since a sync is out of band from regular symbols, it allows synchronization between sender and receiver. 

Sync's also insure that symbols are protected from spontaneous ambient light triggers (see preambled below).


#### Preamble

A preamble is two consecutive syncs. All transmitted frames begin with a preamble, although the first sync may not be received (see below).   


#### Symbols

A symbol is the smallest piece of data that can be sent. 

Each symbol is represented by 2 or more consecutive flashes followed by an idle window.

All symbols must fit within a single time window of the receiver, so the number of symbols is limited by the (1) the length of each flash, (2) the time between consecutive flashes, (3) the length of a timing window. 

#### Frames

All data is transmitted inside of frames.

Each transmitted frame starts with a preamble followed by one or more symbols. 

#### Sync function 

A single flash (sync symbol) can not occur inside a frame, so it serves to mark the beginning of a frame. Anytime a sync is received, any frame in progress is aborted. 

#### Preamble transmission 

If a spontaneous discharge on the receiver happens immediately before a sync flash, the receiver could mistakenly see that as as two flashes.

To prevent this, the transmitter always starts a frame with a preamble consisting of two consecutive sync symbols. The first sync flash causes the LED to recharge ensuring that there is no spontaneous trigger before the second sync flash. 

Receivers should be able to deal with multiple consecutive syncs. In fact, receivers should be able to abort any decode in progress and resync anytime they see a sync. 

#### Error detection

We primarily depend on the fact that the trigger patterns of the valid symbols are very unlikely to appear due to noise. Ambient light creates single triggers separated by more than an idle window. Noise (like direct sunlight) can create many rapid triggers that continue longer than a a time window.

Higher protocols can ensure integrity with data level error checking like checksums and partities.    