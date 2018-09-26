/*
 * callbacks.h
 *
 * The user provides these functions to be called by the platform.
 *
 */

#ifndef CALLBACKS_H_
#define CALLBACKS_H_

#include "bitfun.h"

#include <avr/io.h>
#include <avr/interrupt.h>

// Some helpers for making asynchronous callbacks without leaving interrupts off for too long
// We do this by catching the hardware interrupt and then enabling interrupts before calling the
// callback. We then use a flag to prevent re-entering the callback. If the flag is set when the
// callback completes, then we call into it again so it can see that another interrupt happened
// while it was running. This effectively combines all interrupts while the callback is running into
// a single trailing call. Note that the process repeats if another interrupt happens during that
// trailing call.

// TODO: Make these ISR naked so we can really optimize reentrant interrupts into just
// an SBI and a iret.

// Keep all these callback flags in register GPIOR) because we can SBI and CLI
// on this register with a single instruction that does not change anything else.

#define CALLBACK_REG GPIOR0

// Each reentrant interrupt gets two bits in this register
// The `running` bit means that the callback is currently running, so do not re-eneter it
// The `pending` bit means that another interrupt has happened while the running one was executing so
// we need to call again when it finishes.


#define CALLBACK_BUTTON_RUNNING_BIT         0
#define CALLBACK_BUTTON_PENDING_BIT         1

#define CALLBACK_IR_RUNNING_BIT             2
#define CALLBACK_IR_PENDING_BIT             3

#define CALLBACK_TIMER_RUNNING_BIT          4
#define CALLBACK_TIMER_PENDING_BIT          5

#define CALLBACK_PIXEL_FRAME_RUNNING_BIT    6
#define CALLBACK_PIXEL_FRAME_PENDING_BIT    7



// Below is a glorified macro so we do not need to retype the invovkeCallback
// code for each call back - but we still get very clean and short ISR code
// at compile time without function pointers.

// https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern

template <class T >
struct CALLBACK_BASE {

    // Fill in the callback function and bits here.

    static void inline callback(void);

    static const uint8_t running_bit;
    static const uint8_t pending_bit;

    // General code for making a call back

    static void inline invokeCallback(void) {

        if ( TBI( CALLBACK_REG , T::running_bit ) ) {

            // Remember that we need to rerun when current one is finished.

            SBI( CALLBACK_REG ,  T::pending_bit );

            } else {

            // We are not currently running, so set the gate bit...

            SBI( CALLBACK_REG , T::running_bit );

            do {
                CBI( CALLBACK_REG , T::pending_bit);
                sei();
                T::callback();
                cli();
            } while ( TBI( CALLBACK_REG , T::pending_bit ) );

            CBI( CALLBACK_REG , T::running_bit);

        }
    }
};


// TODO: Check all public functions to make sure they are reentrant.
// IF not, either guard them or document.

#endif /* CALLBACKS_H_ */
