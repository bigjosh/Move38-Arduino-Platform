
#include "blinkos.h"

#include "blinkos_timer.h"

#include <util//atomic.h>
#define DO_ATOMICALLY ATOMIC_BLOCK(ATOMIC_FORCEON)                  // Non-HAL code always runs with interrupts on

// Will overflow after about 62 days...
// https://www.google.com/search?q=(2%5E31)*2.5ms&rlz=1C1CYCW_enUS687US687&oq=(2%5E31)*2.5ms

static volatile millis_t millisCounter=1;           // How many milliseconds since startup?
// We begin at 1 so that the comparison `0 < mills() `
// will always be true so we can use `0` time to semantically
// mean `always in the past.

// Note that resolution is limited by timer tick rate

// TODO: Clear out millis to zero on wake

// Make a snapshot so we do not have to work about intermediate updates to this
// multibyte value while we are looking at it

millis_t millis_snapshot;

// We need a snapshot because the milliscounter can update in the background
// based on the every-1024ms ISR

// Only call with interrupts on.

void updateMillisSnapshot(void) {

    DO_ATOMICALLY {
        millis_snapshot = millisCounter;
    }

}


// Note this runs in callback context

void incrementMillis1ms(void) {

    millisCounter++;

}

