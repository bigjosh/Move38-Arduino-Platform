# Blinks Interrupt Usage

## `TIMER0_OVF_vect`

Fires every time the Timer0 overflows.

This timer directly drives the RED and GREN LEDs via PWM output. It is also synchronized with Timer2 whose output drives the BLUE LED though a charge pump. 

It is run off the 4Mhz system clock with a `/8` prescaller, giving a tick rate of 500Khz. 

The overflow happens every 256 ticks, giving an interrupt rate of ~2KHz (every 512ms).

On each overflow, first the `ISR_CALLBACK_TIMER` callback is execute with interrupts disabled, and then the pixel refresh is executed with the interrupts enabled.

Empirically this codes takes a max of about 110us.

### Pixel Refresh

This code lives in `blinkcore`. 

This code sets up the PWM signals to step though each of the 3 colors individually, and though all 6 RGB LEDs. 

This code takes between 9us and 16.6us

### Timer Callback

This code lives in `blinklib`.

This code...

1. Checks the IR LEDs for triggers and decodes incoming data.
2. Updates the millisecond clock. 
3. Checks if the button state has changed and decodes clicks and pressed.
4. Checks to see if we have timed out and should go to sleep. 

This code takes between 68us and 83us with no incoming IR traffic.

The latency increases with the number of triggers on IR LEDs in that time period. 

| Senders | Max Time | 
|--|--|
| 1 | 87us |
| 2 | 90 us |

## `BUTTON_ISR`

Currently points to an `EMPTY_INTERRUPT`. 

We need some ISR here because we use the button to wake from sleep, but it effectively only takes a few us to call and return. 

## `TIMER1_CAPT_vect`

We use this interrupt to precisely time outgoing IR pulses. It is only used when actually sending a pulse train. 

It takes about 23us. 

## `WDT_vect`

Currently a placeholder function. We will need this to implement partial sleeping where we want to wake up after a certain period of time.  
