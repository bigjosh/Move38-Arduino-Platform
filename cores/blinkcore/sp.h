/*
 * Access the service port 
 *
 * The service port has a high speed (500Kbps) bi-directional serial connection plus an Aux pin that can be used
 * as either digitalIO or an analog in. 
 *
 * Mostly useful for debugging, but maybe other stuff to? :)
 * 
 */ 


#ifndef SP_H_

    #define SP_H_
    
    #include "hardware.h"
       
    #include "bitfun.h"    
    
    
    // Set the pin direction for the service port pins
    // Note that you can not use the R pin in output mode with the serial adapter board 
    // because the level conversion transistor will pull the R pin down. 

    #define SP_PIN_A_MODE_OUT()       SBI( SP_A_DDR , SP_A_BIT )
    #define SP_PIN_A_MODE_IN()        CBI( SP_A_DDR , SP_A_BIT )
    #define SP_PIN_R_MODE_OUT()       SBI( SP_R_DDR , SP_R_BIT )
    #define SP_PIN_R_MODE_IN()        CBI( SP_R_DDR , SP_R_BIT )
    #define SP_PIN_T_MODE_OUT()       SBI( SP_T_DDR , SP_T_BIT )
    #define SP_PIN_T_MODE_IN()        CBI( SP_T_DDR , SP_T_BIT )
    
    // Set the level on the service port pins. 
    // These execute in a single instruction
    // You must set the aux pin to output mode first or else 
    // driving 1 will just enable the pullup if pin is in input mode (which is the default at reset)

    #define SP_PIN_A_SET_1()               SBI( SP_A_PORT, SP_A_BIT)
    #define SP_PIN_A_SET_0()               CBI( SP_A_PORT, SP_A_BIT)
    #define SP_PIN_R_SET_1()               SBI( SP_R_PORT, SP_R_BIT)
    #define SP_PIN_R_SET_0()               CBI( SP_R_PORT, SP_R_BIT)
    #define SP_PIN_T_SET_1()               SBI( SP_T_PORT, SP_T_BIT)
    #define SP_PIN_T_SET_0()               CBI( SP_T_PORT, SP_T_BIT)
        
        
    // Initialize the serial on the service port.
    // Overrides digital mode for service port pins T and R respectively.
    // Also enables the pull-up on the RX pin so it can be connected to an open-collector output
    
    void sp_serial_init(void);
    void sp_serial_init(unsigned long);
        
    // Send a byte out the serial port. Blocks if a transmit already in progress. 
    // Must call sp_serial_init() first
        
    void sp_serial_tx(uint8_t b);
        
    #define SP_SERIAL_TX_NOW(b) (SP_SERIAL_DATA_REG=b)      // Send blindly, but instantly (1 instruction). New byte ignored if there is already a pending one in the buffer (avoid this by leaving 12 clocks between consecutive writes)
        
    // Wait for all pending transmits to complete

    void sp_serial_flush(void);
        
    // Is there a char ready to read?

    uint8_t sp_serial_rx_ready(void);

    // Read byte from service port serial. Blocks if nothing received yet.
    // Input buffer is only 1 byte so if an interrupts happens while data is streaming, you will loose any incoming data. 
    // Best to limit yourself interactions with plenty of time (or an ACK) between each incoming byte.

    uint8_t sp_serial_rx(void); 
    
        
    // Free up service port pin R for digital IO again after sp_serial_init() called
    void sp_serial_disable_rx(void);
    
    // Free up service port pin T for digital IO again after sp_serial_init() called
    void sp_serial_disable_tx(void);
    
                               
    // Read the analog voltage on service port pin A
    // Returns 0-255 for voltage between 0 and Vcc
    // Handy to connect a potentiometer here and use to tune params
    // like brightness or speed
    // Make sure SP_PIN_A is in input mode (default on power up) or this will be very boring
        
    uint8_t sp_aux_analogRead(void);
        
#endif /* DEBUG_H_ */
