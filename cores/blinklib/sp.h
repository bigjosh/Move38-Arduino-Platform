/*
 * Access the service port
 *
 * The service port has a high speed (1Mbps) bi-directional serial connection plus an Aux pin that can be used
 * as either digitalIO or an analog in.
 *
 * Mostly useful for debugging, but maybe other stuff to? :)
 *
 */


#ifndef SP_H_

    #define SP_H_

    // Initialize the serial on the service port.
    // Overrides digital mode for service port pins T and R respectively.
    // Also enables the pull-up on the RX pin so it can be connected to an open-collector output

    void sp_serial_init(void);

    // Send a byte out the serial port. Blocks if a transmit already in progress.
    // Must call sp_serial_init() first

    void sp_serial_tx(unsigned char b);

    // Wait for all pending transmits to complete

    void sp_serial_flush(void);

    // Is there a char ready to read?

    unsigned char sp_serial_rx_ready(void);

    // Read byte from service port serial. Blocks if nothing received yet.
    // Input buffer is only 1 byte so if an interrupts happens while data is streaming, you will loose any incoming data.
    // Best to limit yourself interactions with plenty of time (or an ACK) between each incoming byte.

    unsigned char sp_serial_rx(void);

#endif /* DEBUG_H_ */
