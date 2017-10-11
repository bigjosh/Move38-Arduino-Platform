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

    #include "utils.h"          // Grab SBI and CBI

    #include <util/delay.h>     // Pull in _delay_us() for pulse functions


    // This enables debug features like output on the debug port
    // and some extra sanity parameter checks.

    // DebugA on pin #19 PE2
    // DebugB on pin  #6 PE1
    // DebugC on pin  #3 PE0


    #ifdef DEBUG_MODE

        #define DEBUG_INIT()            SBI( DDRE  , 2); SBI(DDRE,1); SBI( DDRE , 0 )

        #define DEBUGA_1()               SBI( PORTE , 2)
        #define DEBUGA_0()               CBI( PORTE , 2)
        #define DEBUGA_PULSE(width_us)   DEBUGA_1();_delay_us(width_us-2);DEBUGA_0()   // Generate a pulse. width must be >= 2us.

        #define DEBUGB_1()               SBI( PORTE , 1)
        #define DEBUGB_0()               CBI( PORTE , 1)
        #define DEBUGB_PULSE(width_us)   DEBUGB_1();_delay_us(width_us-2);DEBUGB_0()   // Generate a pulse. width must be >= 2us.

        #define DEBUGC_1()               SBI( PORTE , 0)
        #define DEBUGC_0()               CBI( PORTE , 0)
        #define DEBUGC_PULSE(width_us)   DEBUGC_1();_delay_us(width_us-2);DEBUGC_0()   // Generate a pulse. width must be >= 2us.


        #define DEBUG_ERROR(error)       debug_error(error)                 // output an error code


        static void inline DEBUGB_BITS( uint8_t b ) {

            DEBUGB_PULSE(1);

            for( int i=0; i<8; i++ ) {

                if (TBI( b , i ) ){
                    DEBUGB_1();
                }
                _delay_us(20);
                DEBUGB_0();

            }

        }

    #else

        #define DEBUG_INIT()
        #define DEBUGA_1()
        #define DEBUGA_0()
        #define DEBUGA_PULSE(width_us)
        #define DEBUGB_1()
        #define DEBUGB_0()
        #define DEBUGB_PULSE(width_us)
        #define DEBUGB_BITS(x)

    #endif

    // Use ADC6 (pin 19) for an analog input- mostly for dev work
    // Returns 0-255 for voltage between 0 and Vcc
    // Handy to connect a potentiometer here and use to tune params
    // like rightness or speed

    uint8_t analogReadDebugA(void);

#endif /* DEBUG_H_ */