// Simplest main to test bootloader
// Can be programmed with the bootloader no C startup code or variables.
// Hold button on reset to enter seed mode, otherwsie download mode


#include "hardware.h"

#define  F_CPU 1000000

#include <util/delay.h>

//void  dummymain1() __attribute__((section("zerobase"),used));

void  dummymain1()  {

    asm("eor r1,r1");

    BUTTON_PORT |= _BV(BUTTON_BIT);

    _delay_ms( 500 );           // Give pull up a sec to act

    if ( ! BUTTON_PIN & BUTTON_BIT) {       // Pin goes low when button pressed becuase of pullup

        GPIOR1 = 'S';

    } else {

        GPIOR1 = 'D';

    }

    asm("jmp 0x3800");


}

