/*
 * hardware.h
 *
 * Defines the location of all the used hardware
 *
 * Created: 7/23/2017 9:50:54 PM
 *  Author: passp
 */ 


#ifndef HARDWARE_H_
#define HARDWARE_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include "utils.h"


// TODO: Get rid of this when we donm'ty need it everywhere anymore
#define DEBUG_MODE


/*** PIXELS ***/

// Note that we depend on PIXEL_COUNT from blinks.h

// Common Anodes - We drive these 1 to select Pixel.

#define PIXEL1_PORT PORTB
#define PIXEL1_DDR  DDRB
#define PIXEL1_BIT  6

#define PIXEL2_PORT PORTD
#define PIXEL2_DDR  DDRD
#define PIXEL2_BIT  0

#define PIXEL3_PORT PORTB
#define PIXEL3_DDR  DDRB
#define PIXEL3_BIT  7

#define PIXEL4_PORT PORTD
#define PIXEL4_DDR  DDRD
#define PIXEL4_BIT  1

#define PIXEL5_PORT PORTD
#define PIXEL5_DDR  DDRD
#define PIXEL5_BIT  4

#define PIXEL6_PORT PORTD
#define PIXEL6_DDR  DDRD
#define PIXEL6_BIT  2

// RGB Sinks - We drive these cathodes low to light the selected color (note that BLUE has a charge pump on it)
//This will eventually be driven by timers

#define LED_R_PORT PORTD
#define LED_R_DDR  DDRD
#define LED_R_BIT  6

#define LED_G_PORT PORTD
#define LED_G_DDR  DDRD
#define LED_G_BIT  5

#define LED_B_PORT PORTD
#define LED_B_DDR  DDRD
#define LED_B_BIT  3

// This pin is used to sink the blue charge pump
// We drive this HIGH to turn off blue, otherwise blue led could
// come on if the battery voltage is high enough to overcome the forward drop on the
// blue LED + Schottky

#define BLUE_SINK_PORT PORTE
#define BLUE_SINK_DDR  DDRE
#define BLUE_SINK_BIT  3

/*** IR ***/ 

// IR transceivers
// There are 6 IR LEDs - one for each face

#define IR_CATHODE_PORT PORTC
#define IR_CATHODE_DDR  DDRC
#define IR_CATHODE_PIN  PINC

#define IR_ANODE_PORT PORTB
#define IR_ANODE_DDR  DDRB
#define IR_ANODE_PIN  PINB


// CONSTRAINTS:
// Note that all IR anodes must be on same port, as do all IR cathodes. 
// Cathodes must have pin change interrupt
//
// Each anode must be on the same bit as the corresponding anode. (this could be relaxed with extra code)

// All of the 6 GPIO bits used by IR pins. Also assumes these are the same bits in the pin change mask register.

#define IR_BITS     (_BV( 0 )|_BV( 1 )|_BV( 2 )|_BV( 3 )|_BV( 4 )|_BV( 5 ))

// IR pin change interrupts are unused so far, but we will want them for waking from sleep soon...
// TODO: Wake on IR change. 

// We want a pin change interrupt on the CATHODES since these will get charged up 
// and then exposure will make them go low. 
// PORTC is connected to the cathodes, and they are on PCINT0-PCINT5
// which is controlled by PCIE0

/*
    PCICR
    Bit 1 � PCIE1:?Pin Change Interrupt Enable 1
    When the PCIE1 bit is set and the I-bit in the Status Register (SREG) is set, pin change interrupt 1 is
    enabled. Any change on any enabled PCINT[14:8] pin will cause an interrupt. The corresponding interrupt
    of Pin Change Interrupt Request is executed from the PCI1 Interrupt Vector. PCINT[14:8] pins are
    enabled individually by the PCMSK1 Register.
*/

/*
    PCMSK1
    Bits 0, 1, 2, 3, 4, 5, 6 � PCINT8, PCINT9, PCINT10, PCINT11, PCINT12, PCINT13, PCINT14:?Pin
    Change Enable Mask
    Each PCINT[15:8]-bit selects whether pin change interrupt is enabled on the corresponding I/O pin. If
    PCINT[15:8] is set and the PCIE1 bit in PCICR is set, pin change interrupt is enabled on the
    corresponding I/O pin. If PCINT[15:8] is cleared, pin change interrupt on the corresponding I/O pin is
    disabled.
*/


#define IR_PCI     PCIE1
#define IR_ISR     PCINT1_vect
#define IR_MASK    PCMSK1           // Each bit here corresponds to 1 pin
#define IR_PCINT   IR_BITS

/*** Button ***/

#define BUTTON_PORT    PORTD
#define BUTTON_PIN     PIND
#define BUTTON_BIT     7

#define BUTTON_PCI     PCIE2
#define BUTTON_ISR     PCINT2_vect
#define BUTTON_MASK    PCMSK2
#define BUTTON_PCINT   PCINT23

#define BUTTON_DOWN() (!TBI(BUTTON_PIN,BUTTON_BIT))           // PCINT23 - pulled low when button pressed


// This mess is to avoid "warning: "F_CPU" redefined" under Arduino IDE.
// If you know a better way, please tell me!

#ifdef F_CPU
   #undef F_CPU
#endif

#define F_CPU 4000000				// Default fuses are DIV8 so we boot at 1Mhz,
// but then we change the prescaler to get to 4Mhz by the time user code runs

#define FACE_COUNT 6				// Total number of IRLEDs


#endif /* HARDWARE_H_ */