/*
 * Access the service port
 *
 * The service port has a high speed (1Mbps) bi-directional serial connection plus an Aux pin that can be used
 * as either digitalIO or an analog in.
 *
 * Mostly useful for debugging, but maybe other stuff to? :)
 *
 */


#include "sp.h"

#include <avr/io.h>

// Serial port hardware on service port

#define SP_SERIAL_CTRL_REG      UCSR0A
#define SP_SERIAL_DATA_REG      UDR0
#define SP_SERIAL_READY_BIT     RXC0

// Bit manipulation macros
#define SBI(x,b) (x|= (1<<b))           // Set bit in IO reg
#define CBI(x,b) (x&=~(1<<b))           // Clear bit in IO reg
#define TBI(x,b) (x&(1<<b))             // Test bit in IO reg

// Initialize the serial on the service port.
// Overrides digital mode for service port pins T and R respectively.

void sp_serial_init(void) {
    //Initialize the AUX pin as digitalOut
    //SBI( SP_AUX_DDR , SP_AUX_PIN );

    // Initialize SP serial port for 1M baud, n-8-1
    // This feels like it belongs in hardware.c, maybe in an inline function?

    SBI( SP_SERIAL_CTRL_REG , U2X0);        // 2X speed

    SBI( UCSR0B , TXEN0 );                  // Enable transmitter (disables digital mode on T pin)

    SBI( UCSR0B , RXEN0);                   // Enable receiver    (disables digital mode on R pin)

    UBRR0 = 0;                  // 1Mbd. This is as fast as we can go at 8Mhz, and happens to be 0% error and supported by the Arduino serial monitor.
                                // See datasheet table 25-7.
}


// Send a byte out the serial port. DebugSerialInit() must be called first. Blocks unitl buffer free if TX already in progress.

void sp_serial_tx(unsigned char b) {

    while (!TBI(SP_SERIAL_CTRL_REG,UDRE0));         // Wait for buffer to be clear so we don't overwrite in progress

    SP_SERIAL_DATA_REG=b;                           // Send new byte

}

// Wait for most recently transmitted byte to complete

void sp_serial_flush(void) {

    while (!TBI(SP_SERIAL_CTRL_REG,TXC0));         // Wait until the entire frame in the Transmit Shift Register has been shifted out and there are
                                                   // no new data currently present in the transmit buffer

}


// Is there a char ready to read?

unsigned char sp_serial_rx_ready(void) {

    return TBI( SP_SERIAL_CTRL_REG , SP_SERIAL_READY_BIT );

}

// Read byte from service port serial. Blocks if nothing received yet.

unsigned char sp_serial_rx(void) {

    while ( !TBI( SP_SERIAL_CTRL_REG , SP_SERIAL_READY_BIT ) );

    return( SP_SERIAL_DATA_REG );

}

