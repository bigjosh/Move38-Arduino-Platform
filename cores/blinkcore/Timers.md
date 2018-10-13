# Blinks Timer Usage

The ATMEGA168PB has 3 hardware timers. We use them all. 

## `TIMER0`

The hardware outputs of this timer directly drive the cathodes of the red and green LEDs.

## `TIMER1`

Used to precisely time the IR transmit pulses. Software only, no hardware connections.

## `TIMER2`

One hardware output is used to drive the blue LED charge pump. It also has a match set 1/2 way though the phase to drive the IR receive ISR out of phase with the `TIMER0` ISR.

