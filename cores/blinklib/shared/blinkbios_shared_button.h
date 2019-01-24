/*
 * blinkbios_pixel_block.h
 *
 * Defines the shared memory block used to check for button presses
 *
 */

#ifndef BLINKBIOS_BUTTON_BLOCK_H_
#define BLINKBIOS_BUTTON_BLOCK_H_

// #define USER_VOLATILE or BIOS_VOLATILE based on the presence of #define BIOS_VOLATILE_FLAG
#include "blinkbios_shared_volatile.h"

#include <stdint.h>

// I know this is ugly, but keeping them in a single byte lets us pass them by value
// and also lets us OR them together. Efficiency in the updater trumps since it
// runs every millisecond.

// TODO: Maybe more RAM but less flash to have these as individual bytes?

#define BUTTON_BITFLAG_PRESSED          0b00000001
#define BUTTON_BITFLAG_LONGPRESSED      0b00000010
#define BUTTON_BITFLAG_RELEASED         0b00000100
#define BUTTON_BITFLAG_SINGLECLICKED    0b00001000
#define BUTTON_BITFLAG_DOUBLECLICKED    0b00010000
#define BUTTON_BITFLAG_MULITCLICKED     0b00100000

#define BUTTON_BITFLAG_6SECPRESSED      0b01000000
#define BUTTON_BITFLAG_7SECPRESSED      0b10000000

struct blinkbios_button_block_t {

    USER_VOLATILE  uint8_t down;                // 1 if button is currently down (debounced)

    BOTH_VOLATILE uint8_t bitflags;

    USER_VOLATILE uint8_t clickcount;           // Number of clicks on most recent multiclick

    BOTH_VOLATILE uint8_t wokeFlag;             // Set to 0 upon waking from sleep

    // The variables below are used to track the intermediate button state
    // and probably not interesting to user programs

    uint8_t buttonDebounceCountdown;               // How long until we are done bouncing. Only touched in the callback
                                                   // Set to BUTTON_DEBOUNCE_MS every time we see a change, then we ignore everything
                                                   // until it gets to 0 again

    uint16_t clickWindowCountdown;                 // How long until we close the current click window. 0=done

    uint8_t clickPendingcount;                     // How many clicks so far int he current click window

    uint8_t longpressRegisteredFlag;               // Did we register a long press since the button went down?
                                                   // Avoids multiple long press events from the same down event
                                                   // We could keep an independent longpressCountup, but that would be a 16 bit so heavy to increment and compare.

    uint16_t pressCountup;                         // Start counting up when the button goes down to detect long presses. Has to be 16 bit because long presses are >255 centiseconds.
    
    // For future use?
    uint8_t slack[4];
    

};

// Everything in here is volatile to the user code

extern blinkbios_button_block_t blinkbios_button_block;

#endif /* BLINKBIOS_BUTTON_BLOCK_H_ */