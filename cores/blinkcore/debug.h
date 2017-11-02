/*
 * debug.h
 *
 * #define DEBUG_MODE to get some helpful development tools
 *
 * Created: 7/14/2017 1:50:04 PM
 *  Author: josh
 */ 




#ifndef DEBUG_H_

    #define DEBUG_H_
    
    #include "blinkcore.h"    
    
    #include "hardware.h"

    #include "utils.h"          // Grab SBI and CBI

    #include <util/delay.h>     // Pull in _delay_us() for pulse functions

    // This enables debug features like output on the debug port
    // and some extra sanity parameter checks.


    #ifdef DEBUG_MODE

        #define DEBUG_INIT()             SBI( DEBUGA_DDR  , DEBUGA_PIN); SBI(DEBUGB_DDR, DEBUGB_PIN ); SBI( DEBUGC_DDR , DEBUGC_PIN )



        #define DEBUGA_1()               SBI( DEBUGA_PORT, DEBUGA_PIN)
        #define DEBUGA_0()               CBI( DEBUGA_PORT, DEBUGA_PIN)
        #define DEBUGA_PULSE(width_us)   do {DEBUGA_1();_delay_us(width_us-2);DEBUGA_0();} while(0)   // Generate a pulse. width must be >= 2us.

        #define DEBUGB_1()               SBI( DEBUGB_PORT, DEBUGB_PIN)
        #define DEBUGB_0()               CBI( DEBUGB_PORT, DEBUGB_PIN)
        #define DEBUGB_PULSE(width_us)   do {DEBUGB_1();_delay_us(width_us-2);DEBUGB_0();} while (0)   // Generate a pulse. width must be >= 2us.

        #define DEBUGC_1()               SBI( DEBUGC_PORT, DEBUGC_PIN)
        #define DEBUGC_0()               CBI( DEBUGC_PORT, DEBUGC_PIN)
        #define DEBUGC_PULSE(width_us)   do {DEBUGC_1();_delay_us(width_us-2);DEBUGC_0();} while(0)   // Generate a pulse. width must be >= 2us.


        // Initialize the service port for a serial connection on pins T and R. 
        // Note that this disable the DEBUGA and DEBUGB logic outputs. 
        
        void debug_serial_init(void);
                
        // Send a byte out the serial port. Blocks if a transmit already in progress. 
        // Must call debug_iserial_init() first
        
        void debug_serial_tx(byte b);
        
         #ifdef DEBUG_SERIAL_PRESENT
            #define DEBUG_TX_NOW(b) (UDR0=b)      // Send blindly, but instantly (1 instruction)  Could clobber, but you'd have to send pretty darn fast for that.
         #else
            #define DEBUG_TX_NOW(b)               // Do nothing if no serial port, but define as empty so code that has this still compiles
        #endif
        
        #define DEBUG_ERROR(error)       debug_error(error)                 // output an error code

    #else

        #define DEBUG_INIT()
        #define DEBUGA_1()
        #define DEBUGA_0()
        #define DEBUGA_PULSE(width_us)
        #define DEBUGB_1()
        #define DEBUGB_0()
        #define DEBUGB_PULSE(width_us)
        #define DEBUGB_BITS(x)
        #define DEBUGC_1()               
        #define DEBUGC_0()               
        #define DEBUGC_PULSE(width_us)   

    #endif

    // Use ADC6 (pin 19) for an analog input- mostly for dev work
    // Returns 0-255 for voltage between 0 and Vcc
    // Handy to connect a potentiometer here and use to tune params
    // like rightness or speed

    uint8_t analogReadDebugA(void);

#endif /* DEBUG_H_ */