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

// #define TX_DEBUG
// Pin A will go high when IR pulse sent on IR0

#define RX_DEBUG
// Pin A will go high when IR0 is triggered (the charge is drained by light hitting the LED). Note this is dependent on interrupts being on. In ir.cpp
// Pin R will go high during each sample window. In ir.cpp
// You might think you could just put a scope on the LED pin and watch it directly, but the resistance of the probe kills the effect so
// we have to do it in software. The input pin has very, very high impedance.
// TX will send chars based on the receive state of IR0. Look in irdata.cpp to see what they are.

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

uint8_t ir_sample_bits( void );

// Charge the bits set to 1 in 'chargeBits'
// Probably best to call with ints off so doesn't get interrupted
// Probably best to call some time after ir_sample_bits() so that a long pulse will not be seen twice.

void ir_charge_LEDs( uint8_t chargeBits );


#define WAKEON_IR_BITMASK_NONE     0             // Don't wake on any IR change
#define WAKEON_IR_BITMASK_ALL      IR_BITS       // Don't wake on any IR change

#endif /* IR_H_ */