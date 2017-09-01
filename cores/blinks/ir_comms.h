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

// The RX API...

// Returns last received data (value 0-3) for requested face
// bit 2 is 1 if data found, 0 if not
// if bit 2 set, then bit 1 & 0 are the data
// So possible return values:
// 0x00=0b00000000=No data received since last read
// 0x04=0b00000100=Received 0
// 0x04=0b00000101=Received 1
// 0x04=0b00000110=Received 2
// 0x04=0b00000111=Received 3

// Superseded by idiomatic Arduino functions

// uint8_t ir_read( uint8_t led);



// If bit set, then a new byte was received before the previous byte in the buffer was read.
// Currently the buffered byte is kept. 
// The bit is cleared when the currently buffered byte is read. 

// Superseded by idiomatic Arduino functions

// uint8_t irled_rx_overflowBits(void);             

// Transmit a value (0-3) on face
// (only 2 bits of data for now)
// If no transmit in progress, then returns immediately and starts the transmit within 500us
// IF a transmit is in progress, then blocks until that is complete. 

// Superseded by idiomatic Arduino functions

//void ir_send( uint8_t face , uint8_t data );


// The functions below are for Arduino consumption

// TODO: Give people bytes rather than measly dibits

// Returns last received dibit (value 0-3) for requested face
// Blocks if no data ready
// `face` must be less than FACE_COUNT

uint8_t irReadDibit( uint8_t face);

// Returns true if data is available to be read on the requested face
// Always returns immediately
// Cleared by subseqent irReadDibit()
// `face` must be less than FACE_COUNT

uint8_t irIsAvailable( uint8_t face );

// Returns true if data was lost because new data arrived before old data was read
// Next read will return the older data (new data does not over write old)
// Always returns immediately
// Cleared by subseqent irReadDibit()
// `face` must be less than FACE_COUNT

uint8_t irOverFlowFlag( uint8_t face );

// Transmits the lower 2 bits (dibit) of data on requested face
// Blocks if there is already a transmission in progress on this face
// Returns immediately and continues transmission in background if no transmit already in progress
// `face` must be less than FACE_COUNT

void irSendDibit( uint8_t face , uint8_t data );

// Transmits the lower 2 bits (dibit) of data on all faces
// Blocks if there is already a transmission in progress on any face
// Returns immediately and continues transmission in background if no transmits are already in progress

void irSendAllDibit(  uint8_t data );

// Blocks if there is already a transmission in progress on this face
// Returns immediately if no transmit already in progress
// `face` must be less than FACE_COUNT

void irFlush( uint8_t face );

#endif /* IR-COMMS_H_ */