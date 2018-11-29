/*
 * blinkbios_millis_block.h
 *
 * Defines the shared memory block used to keep the millisecond counter
 *
 */

#ifndef BLINKBIOS_MILLIS_BLOCK_H_
#define BLINKBIOS_MILLIS_BLOCK_H_

#include <stdint.h>

// #define USER_VOLATILE or BIOS_VOLATILE based on the presence of #define BIOS_VOLATILE_FLAG
#include "blinkbios_shared_volatile.h"

typedef uint32_t millis_t;

struct blinkbios_millis_block_t {
    USER_VOLATILE  millis_t millis;
    USER_VOLATILE  uint8_t step_8us;               // Carry over of sub-millis since our clock does not go evenly into 1ms. This is the number of 8us steps past the value in millis. Never gets above 125.

    USER_VOLATILE  millis_t sleep_time;            // When millis>sleep_time, then we go to sleep
};

extern blinkbios_millis_block_t blinkbios_millis_block;


#endif /* BLINKBIOS_MILLIS_BLOCK_H_ */