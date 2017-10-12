/*
 * ir_comms.h
 *
 * All the functions for communication and waking on the 6 IR LEDs on the tile edges.
 *
 */ 

#ifndef IR_COMMS_H_
#define IR_COMMS_H_

#define IRLED_COUNT FACE_COUNT

 
// Setup pins, interrupts

void ir_init(void);

// Enable IR normal operation (call after init or disable)

// TODO: Specify specific LEDs?

void ir_enable(void);

// Stop IR interrupts (call after enable)

void ir_disable(void);

// Called anytime on of the IR LEDs triggers, which could
// happen because it received a flash or just because
// enough ambient light accumulated

void __attribute__((weak)) ir_callback(uint8_t triggered_bits);

// Send a pulse on all LEDs that have a 1 in bitmask
// bit 0= D1, bit 1= D2...

// TODO: Allow user to specify pulse width? We'd need a variable _delay_us() function.

void ir_tx_pulse( uint8_t bitmask );


// Send a byte on the indicated face. Returns when transmission complete.
void ir_sendData( uint8_t face, uint8_t data );

#define ERRORBIT_PARITY       2    // There was an RX parity error
#define ERRORBIT_OVERFLOW     3    // A received byte in lastValue was overwritten with a new value
#define ERRORBIT_NOISE        4    // We saw unexpected extra pulses inside data
#define ERRORBIT_DROPOUT      5    // We saw too few pulses, or two big a space between pulses

// Read the error state of the indicated LED
// Clears the bits on read

uint8_t ir_getErrorBits( uint8_t face );

// Is there a received data ready to be read?
uint8_t ir_isReady( uint8_t face );

// Read the most recently received data. Blocks if no data ready
uint8_t ir_getData( uint8_t face );


#define WAKEON_IR_BITMASK_NONE     0             // Don't wake on any IR change
#define WAKEON_IR_BITMASK_ALL      IR_BITS       // Don't wake on any IR change

#endif /* IR-COMMS_H_ */