/*
 * ir_comms.h
 *
 * All the functions for communication and waking on the 6 IR LEDs on the tile edges.
 *
 */

#ifndef IR_H_
#define IR_H_

#include "shared.h"			// Get FACECOUNT
#include "bitfun.h"

#define IRLED_COUNT FACE_COUNT

#define IR_ALL_BITS (0b00111111)        // All six IR LEDs

// Setup pins, interrupts

void ir_init(void);

// Enable IR normal operation (call after init or disable)

// TODO: Specify specific LEDs?

void ir_enable(void);

// Stop IR interrupts (call after enable)

void ir_disable(void);

// Sends starting a pulse train.
// Each pulse will have an integer number of delays between it and the previous pulse.
// Each of those delay windows is spacing_ticks wide.
// This sets things up and sends the initial pulse.
// Then continue to call ir_tx_sendpulse() to send subsequent pulses
// Call ir_tx_end() after last pulse to turn off the ISR (optional but saves CPU and power)

void ir_tx_start(uint16_t spacing_ticks , uint8_t bitmask , uint16_t initialSpaces );

// Send next pulse int this pulse train.
// leadingSpaces is the number of spaces to wait between the previous pulse and this pulse.
// 0 doesn't really make any sense
// Note that you must called ir_tx_sendpuse fast enough that the buffer doesn't run dry

void ir_tx_sendpulse( uint8_t leadingSpaces );

// Turn off the pulse sending ISR
// Blocks until final pulse transmitted
// TODO: This should return any bit that had to be terminated because of collision

void ir_tx_end(void);

// Measure the IR LEDs to to see if they have been triggered.
// Must be called when interrupts are off.
// Returns a 1 in each bit for each LED that was fired.
// Fired LEDs are recharged.

uint8_t ir_test_and_charge_cli( void );


// Called anytime on of the IR LEDs triggers, which could
// happen because it received a flash or just because
// enough ambient light accumulated

void __attribute__((weak)) ir_callback(uint8_t triggered_bits);


#define WAKEON_IR_BITMASK_NONE     0             // Don't wake on any IR change
#define WAKEON_IR_BITMASK_ALL      IR_BITS       // Don't wake on any IR change

#endif /* IR_H_ */
