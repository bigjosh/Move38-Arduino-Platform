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

// These #defines will let you see what is happening on the IR link by connecting an
// oscilloscope to the service port pins.

// If defined, TX_DEBUG will output HIGH on pin A on a Dev Candy board anytime
// an IR pulse is sent on IR0. The pulse is high for the duration of the IR on time.

//#define TX_DEBUG


// If defined, RX_DEBUG will output HIGH on pin A on a dev candy board anytime
// an IR pulse is received on IR0. The pulse is high from the moment the pin changes,
// to the moment it is read by the timer polling routine.
// The ServicePort serial will transmit at 1Mpbs the following...
// 'I' on startup initialization
//

//#define IR_RX_DEBUG
//#define IR_RX_DEBUG_LED 4       // Which IR LED should we watch? (0-5)
                                // 4 is nice because you can monitor the LED anode on the square pin of the ISP port


// If defined, we keep some extra error counters for diagnostics.
// Otherwise IR errors don't really show up anywhere except maybe decreased performance.
// TODO: Implement these counters so we have a way of seeing errors.
//#define RX_TRACK_ERRORS

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
// Will send first pulse and then wait initialTicks before sending second pulse.
// Then continue to call ir_tx_sendpulse() to send subsequent pulses
// Call ir_tx_end() after last pulse to turn off the ISR (optional but saves CPU and power)

void ir_tx_start(uint8_t bitmask , uint16_t initialTicks );

// Send next pulse int this pulse train.
// leadingSpaces is the number of spaces to wait between the previous pulse and this pulse.
// 0 doesn't really make any sense
// Note that you must called ir_tx_sendpuse fast enough that the buffer doesn't run dry

void ir_tx_sendpulse( uint16_t delay_cycles);

// Turn off the pulse sending ISR
// Blocks until final pulse transmitted
// TODO: This should return any bit that had to be terminated because of collision

void ir_tx_end(void);

// Measure the IR LEDs to to see if they have been triggered.
// Must be called when interrupts are off.
// Returns a 1 in each bit for each LED that was fired.
// Fired LEDs are recharged.

uint8_t ir_sample_and_charge_LEDs();


#define WAKEON_IR_BITMASK_NONE     0             // Don't wake on any IR change
#define WAKEON_IR_BITMASK_ALL      IR_BITS       // Don't wake on any IR change

#endif /* IR_H_ */