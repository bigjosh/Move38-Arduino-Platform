/*
 * Access the service port for core debugging
 *
 * The service port has a high speed (1Mbps) bi-directional serial connection plus an Aux pin that can be used
 * as either digitalIO or an analog in.
 *
 *
 */


#ifndef DEBUG_H_

    #define DEBUG_H_

    #include "hardware.h"

    #include "bitfun.h"

    // Initialize the service port for output.
    // Pin T serial output at 1000000bd
    // Pin A digital out
    // Pin R digital out

    struct Debug {


        inline static void init(void) {

            SBI( SP_SERIAL_CTRL_REG , U2X0);        // 2X speed

            SBI( UCSR0B , TXEN0 );                  // Enable transmitter (disables digital mode on T pin)

            UBRR0 = 0;                              // 1Mbd. This is as fast as we can go at 8Mhz, and happens to be 0% error and supported by the Arduino serial monitor.
                                                    // See datasheet table 25-7.


            SBI( SP_A_DDR , SP_A_BIT );             // Pin A output mode
            SBI( SP_R_DDR , SP_R_BIT );             // Pin R output mode

        }

        // Send a byte out the serial port. DebugSerialInit() must be called first. Blocks unitl buffer free if TX already in progress.
        
        // TODO: This compile to this...
        
        /*
        
            //while (!TBI(SP_SERIAL_CTRL_REG,UDRE0));         // Wait for buffer to be clear so we don't overwrite in progress
            1034:	e0 ec       	ldi	r30, 0xC0	; 192
            1036:	f0 e0       	ldi	r31, 0x00	; 0
            1038:	80 81       	ld	r24, Z
            103a:	85 ff       	sbrs	r24, 5
            103c:	fd cf       	rjmp	.-6      	; 0x1038 <_Z14irSendCompletev+0x6>
            //SP_SERIAL_DATA_REG=b;                           // Send new byte
            1040:	83 e4       	ldi	r24, 0x43	; 67
            1042:	80 93 c6 00 	sts	0x00C6, r24	; 0x8000c6 <__TEXT_REGION_LENGTH__+0x7e00c6>  
        */
        
         inline static void tx(uint8_t b) {


            while (!TBI(SP_SERIAL_CTRL_REG,UDRE0));         // Wait for buffer to be clear so we don't overwrite in progress
            SP_SERIAL_DATA_REG=b;                           // Send new byte

        }

        // Wait for most recently transmitted byte to complete

        inline static void flush(void) {

            while (!TBI(SP_SERIAL_CTRL_REG,TXC0));         // Wait until the entire frame in the Transmit Shift Register has been shifted out and there are
            // no new data currently present in the transmit buffer

        }


        // This compiles down to a single instruction so very fast.
        // Just don't call consecutively with less than 8 instructions between calls or
        // it will, overrun

        inline static void tx_now(uint8_t b) {
            SP_SERIAL_DATA_REG=b;                  // Send blindly, but instantly (1 instruction). New byte ignored if there is already a pending one in the buffer (avoid this by leaving 12 clocks between consecutive writes)
        }


        inline static void pin_a_0() {
            CBI( SP_A_PORT, SP_A_BIT);
        }

        inline static void pin_a_1() {
            SBI( SP_A_PORT, SP_A_BIT);
        }

        inline static void pin_r_0() {
            CBI( SP_R_PORT, SP_R_BIT);
        }

        inline static void pin_r_1() {
            SBI( SP_R_PORT, SP_R_BIT);
        }

    };

#endif /* DEBUG_H_ */
