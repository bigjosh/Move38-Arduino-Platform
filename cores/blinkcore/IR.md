### IR Communications Protocol

Each of the 6 edges of a blink tile has an IR LED to communicate with its adjacent neighbor.

#### Physical layer

Each single IR LED is used for both transmit and receive. The LED pins are connected directly to GPIO pins on the MCU.

For transmitting, the LED is briefly driven with a forward voltage to flash it. These flashes are short to save power. 

For receiving, the LED is negatively biased to charge it up like a capacitor. Light falling on the LED makes it discharge more quickly, and by timing how long it takes to discharge we can measure the integral how much light is hitting it. [This technique](https://www.sparkfun.com/news/2161) was originally described by Forest Mimms III.

We've chosen our LED to...

* quickly reliably discharge below the digital trigger voltage of our MCU input pins when exposed to a >10us flash from a corresponding transmitting LED at a distance of about 1mm,

AND
 
* take >1ms to discharge in ambient light.


##### DDR and PORTs of IR LEDs during different states


| State | Anode DDR | Anode PORT | Cathode DDR | Cathode PORT | Notes |
| --- | --- | --- | --- | --- | --- |
| Charging | High (driving) | Low | High (driving) | High  | Led reverse biased |
| Listening | High (driving) | Low | Low (input) | Low (no pull-up) | Cathode sees input go low when charge depleted |
| Sending | High (driving) | High | High (driving) | Low | Normal forward voltage lights LED |

Note that Anode DDR is always driven so we never touch that after enabling IR system. 

#### Receiving 

Higher level code can use `ir_test_and_charge()` to check the digital state of the LEDs to see if they have been discharged since last charged. Any LEDs that have discharged are recharged so they are ready to receive again.

It is also possible to generate an interrupt when an LED crosses the digital threshold voltage, but this is not currently used (except when `RX_DEBUG` is enabled).   

#### Transmitting

Note that with this system, an LED can not receive while it is transmitting, so we depend on a higher level protocol to handshake and make sure both sides take turns sending.  

All transmissions are a series of flashes separated by a number of gaps times called spaces. The space time is typically fixed for a given protocol. 

The gaps are precisely timed and will be accurate to within the latency of interrupts on the system, so keep interrupts disabled for as little time as possible. 

Call `ir_tx_start()` to begin transmitting a pulse train. This sends the first flash.

Then call `ir_tx_sendpulse()` to transmit each of the remaining flashes in the pulse train. Obviously you must call `ir_tx_sendpulse()` quickly enough that you keep up with the specified number of spaces from the previous call. 

Call `ir_tx_end()` at the end of the train. This will block until the final flash is transmitted. 

    
### Tricky PORTC optimization

We exploit the fact that the upper two bits of PORTC are not used on this chip to save some time. By connecting the IR LED cathodes to PORTC bits 0-5 and forcing bits 6,7 high, we can work with the PORT as a full byte and avoid having to mask the top bits out.

If you were to move the cathodes to a different port, or you were to use a chip that actually had PORTC[6:7], then you would need to mask out these bits.   