/*
 * ir_comms.h
 *
 * All the functions for communication and waking on the 6 IR LEDs on the tile edges.
 *
 */ 

#ifndef IR_H_
#define IR_H_

#include "blinkcore.h"

#define IRLED_COUNT FACE_COUNT

#define ALL_IR_BITS (0b00111111)        // All six IR LEDs

// Setup pins, interrupts

void ir_init(void);

// Enable IR normal operation (call after init or disable)

// TODO: Specify specific LEDs?

void ir_enable(void);

// Stop IR interrupts (call after enable)

void ir_disable(void);


// Send a pulse on all LEDs that have a 1 in bitmask
// bit 0= D1, bit 1= D2...
// This clobbers whatever charge was on the selected LEDs, so only call after you have checked it.

// Must be atomic so that...
// 1) the IR ISR doesnt show up and see our wierd registers, and
// 2) The flashes don't get interrupted and streched out long enough to cause 2 triggers

// TODO: Queue TX so they only happen after a successful RX or idle time. Unnecessary since TX time so short?

// ASSUMES INTERRUPTS ON!!!! DONT CALL INTERNALLY!
// TODO: Make a public facing version that brackets with ATOMIC

// TODO: INcorporate this into the tick handle so we will be charging anyway.

void ir_tx_pulse( uint8_t bitmask );

// Called anytime on of the IR LEDs triggers, which could
// happen because it received a flash or just because
// enough ambient light accumulated


// Measure the IR LEDs to to see if they have been triggered.
// Returns a 1 in each bit for each LED that was fired.
// Fired LEDs are recharged.

uint8_t ir_test_and_charge( void );

void __attribute__((weak)) ir_callback(uint8_t triggered_bits);


#define WAKEON_IR_BITMASK_NONE     0             // Don't wake on any IR change
#define WAKEON_IR_BITMASK_ALL      IR_BITS       // Don't wake on any IR change

#endif /* IR_H_ */