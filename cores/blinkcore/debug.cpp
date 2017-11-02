/*
 * Some often needed defines and handy debugging tools
 * 
 */ 


#include "hardware.h"

#include "utils.h"

#include "debug.h"


// Use ADC6 (pin 19) for an analog input- mostly for dev work
// Returns 0-255 for voltage between 0 and Vcc
// Handy to connect a potentiometer here and use to tune params
// like rightness or speed

uint8_t analogReadDebugA(void) {
	ADMUX =
	_BV(REFS0)   |                  // Reference AVcc voltage
	_BV( ADLAR ) |                  // Left adjust result so only one 8 bit read of the high register needed
	_BV( MUX2 )  | _BV( MUX1 )      // Select ADC6
	;
	
	ADCSRA =
	_BV( ADEN )  |                  // Enable ADC
	_BV( ADSC )                     // Start a conversion
	;
	
	
	while (TBI(ADCSRA,ADSC)) ;       // Wait for conversion to complete
	
	return( ADCH );
	
}

// Initialize the serial on the service port. 

void debug_init_serial(void) {
    
    SBI( UCSR0A , U2X0);        // 2X speed
    SBI( UCSR0B , TXEN0 );      // Enable transmitter (disables DEBUGA)
    SBI( UCSR0B , RXEN0);       // Enable receiver    (disables DEBUGB)
    
    #if F_CPU!=4000000  
        #error Serial port calculation in debug.cpp must be updated if not 4Mhz CPU clock.
    #endif
    
    UBRR0 = 0;                  // 500Kbd. This is as fast as we can go at 4Mhz, and happens to be 0% error and supported by the Arduino serial monitor. 
                                // See datasheet table 25-7. 
}    

// Send a byte out the serial port. DebugSerialInit() must be called first. Blocks if TX already in progress.

void DEBUG_TX(uint8_t b) {    
    DEBUGC_1();
    //while (!TBI(UCSR0A,UDRE0)); 
    DEBUGC_0();
    UDR0=b;
}


// Send a byte out the serial port immedeatly (clobbers any in-progress TX). DebugSerialInit() must be called first.

void DEBUG_TX_NOW(uint8_t b) {
    UDR0=b;
}    

