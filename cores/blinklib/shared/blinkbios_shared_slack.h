/*
 * blinkbios_pixel_block.h
 *
 * Defines the shared memory block used to check for button presses
 *
 */

#ifndef BLINKBIOS_SLACK_BLOCK_H_
#define BLINKBIOS_SLACK_BLOCK_H_

#include <stdint.h>

// Since these blocks will be locked in until the end of time, mind as well reserve a few extra bytes
// in case we ever need them some day

struct blinkbios_slack_block_t {

    uint8_t slack[20];

};

extern volatile blinkbios_slack_block_t blinkbios_slack_block;


#endif /* BLINKBIOS_SLACK_BLOCK_H_ */