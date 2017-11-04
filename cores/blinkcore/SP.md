# Service Port


## Features

Newer boards have a service port that features...

* Super high speed, low latency, bi-directional serial port
* Aux pin that can be used for either digital IO or as an analog input

These can be very helpful for...

* debugging code on the tile since you get a printf() (via the serial port)
* collecting very accurate timing info on a scope (via the a Aux pin as digital out)
* intuitively to adjusting things like colors (using a potentiometer connected to the the aux pin as analog in)    

## Connections

To use the port, you probably want to solder on a 4 pin 1.25mm pitch connector like this one...

https://www.digikey.com/products/en?keywords=wm1733-nd

The pins are...

|Pin|Function|Direction|
|---|---|---|
|G|Ground|(na)|
|T|Serial TX|Out from tile|
|R|Serial RX|Into Tile|
|A|Aux pin|digital in or out with or without pull-up, or analog in to Tile| 

At some point I'll make a nice board that breaks out the service port connector into nice serial and aux links with all the level shifting built in. 

## Voltage conversions

Note that all the pins are referenced to whatever the tile battery voltage happens to be, so level shifting may be required.

### Serial TX
I use a FDTI USB serial converter that will see a '1' for any voltage above 2 volts, so the RX pin on that can be connected directly to the service port `T` pin. Otherwise you could use a MOSFET or packaged voltage converter like this...

http://amzn.to/2zjSx8t

### Serial RX

Because of the ESD protection diode on the pin of the AVR, you can connect the service port 'R' pin to a 3.3V (or even a 5V) output TX pin of a serial port as long as you put a current limiting resistor in there. I am using a 3.3K resistor which should limit the current to less than 1mA. 

There is a downside to this in that that trickle current will keep the AVR somewhat powered up even with the battery out. Since the current is limited, it will undervoltage if it tries to light an LED so as a practical matter this means I need to unplug the service port when changing the battery. 

Again, a proper level convert like above will solve these problems.

### Aux

#### Digital Output

This will just be whatever the battery voltage is when set to out a `1`. If you are just connecting up to a scope then this works great.

#### Digital Input

Like the serial RX pin, you could drive this with a voltage higher than the battery voltage though a large current limiting resistor or use a level converter.

You could also enable the optional pull-up on this pin and then use an open collector (or dry contact switch) to pull low.

#### Analog input

In the mode, you get 8-bits that tell you the voltage on the aux pin relative to the battery voltage, so 0 means 0 volts, and 128 means the voltage on the aux pin is 1/2 the battery voltage.

#### Touch sensing 

The aux pin is also connected electrically to the Move30 logo on the PCB, so this opens up the possibly to do CapSense touch sensing on this pad by charging the pad using aux digital out to `1`, and then switching to analog input and seeing how much the voltage drops each time you do a reading. This is an indication of the capacitance connected to the pin - which will be much higher when there is a finger connected to a person touching it.   

# On Latency

## Serial

The serial port is implemented using the on-chip USART, so works completely in the background. 

The `sp_serial_tx()` function will first check so see if there is already a pending transmit in progress, and if so will wait for it to complete. The test adds a couple cycles before the send even if the coast is clear, and the call could block for a dozen cycles if there is a transmit in progress that just started. 

if you are working on timing sensitive stuff, you can also use the `SP_SERIAL_TX_NOW()` function. This function does not do any checking- it immediately (and blindly) writes the byte to the serial port, and takes only a single instruction to execute. If the buffer is not ready (there is already a byte waiting to be transferred into the shift register) then the new byte will be ignored. This is not a scary as it sounds since you can just be sure to always leave at least a dozen instructions between sequential calls to give a byte time to drain out before sending the next one. This will likely happen easily in practice since you will be doing other stuff between consecutive serial writes.

## Aux out

The `SP_AUX_0()` and `SP_AUX_0()` also always complete in a single cycle, so are great for timing. 

To benchmark how long a function takes, you can put a `SP_AUX_1()` at the entry and a `SP_AUX_0()` at the exit and then connect a scope to the aux pin. Measuring the pulse width will give you instant and minimally intrusive metrics on on the min/avg/max run time for the function!

# Connecting a potentiometer for analog input. 

Some things are each to do analog-ly. So much easier to turn a dial than to keep clicking up and down for stuff like color normalization. The analog in function of the aux pin should be great for this stuff. 

Since the input is referenced to the battery voltage, we need a good place to get at the reference voltage.

As long as you init() the service port serial and are not actively sending data, then the pin will be an an idle "space", which is driven at the battery voltage. In this case, you should be able to attach a large pot (>1K) between the `T` and `G` and then connect the pot tap to the `A` pin and read the position of of pot with `sp_aux_analogRead()`. Untested, but should work!     

