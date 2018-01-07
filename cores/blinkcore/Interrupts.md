# Blinks Interrupt Usage

## `TIMER2_OCR2A_vect`

Fires every time Timer2 matches with 128 (max is 256). 

We needed a 256us clock to properly do IR comms, but there is no suitable prescaller that get us this rate so we had to synthesize it.

We do this by combining the overflow interrupt on timer0 with the match interrupt on timer2. Both are running in lockstep at 512us rate, and so by setting the match on timer2 to 128, we get one interrupt every 256us.

On each match, we call the 256us clock callbacks. 

## `TIMER0_OVF_vect`

Fires every time the Timer0 overflows.

This timer directly drives the RED and GREEN LEDs via PWM output. It is also synchronized with Timer2 whose output drives the BLUE LED though a charge pump. 

It is run off the 4Mhz system clock with a `/8` prescaller, giving a tick rate of 500Khz. 

The overflow happens every 256 ticks, giving an interrupt rate of ~2KHz (every 512ms).

On each overflow, we first call the 256us clock callbacks, then the 512us clock callback.  

### Pixel Refresh

This code lives in `blinkcore`. 

This code sets up the PWM signals to step though each of the 3 colors individually, and though all 6 RGB LEDs. 

This code takes between 9us and 16.6us

### 256us Timer Callback

This code lives in `blinklib` & `irdata`.

This code...

Checks the IR LEDs for triggers and decodes incoming data.


### 512us Timer Callback

This code lives in `blinklib`.

This code...
2. Updates the millisecond clock. 
3. Checks if the button state has changed and decodes clicks and pressed.
4. Checks to see if we have timed out and should go to sleep. 

## `BUTTON_ISR`

Currently points to an `EMPTY_INTERRUPT`. 

We need some ISR here because we use the button to wake from sleep, but it effectively only takes a few us to call and return. 

## `TIMER1_CAPT_vect`

We use this interrupt to precisely time outgoing IR pulses. It is only used when actually sending a pulse train. 

It takes about 23us. 

## `WDT_vect`

Currently a placeholder function. We will need this to implement partial sleeping where we want to wake up after a certain period of time.  
