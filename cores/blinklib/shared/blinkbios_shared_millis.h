/*
 * blinkbios_millis_block.h
 *
 * Defines the shared memory block used to keep the millisecond counter
 *
 */

#ifndef BLINKBIOS_MILLIS_BLOCK_H_
#define BLINKBIOS_MILLIS_BLOCK_H_

#include <stdint.h>

typedef uint32_t millis_t;

struct blinkbios_millis_block_t {
    millis_t millis;
    uint8_t step_8us;               // Carry over of sub-millis since our clock does not go evenly into 1ms. This is the number of 8us steps past the value in millis. Never gets above 125.

    millis_t sleep_time;   // When millis>sleep_time, then we go to sleep
};

extern volatile blinkbios_millis_block_t blinkbios_millis_block;


#endif /* BLINKBIOS_MILLIS_BLOCK_H_ */