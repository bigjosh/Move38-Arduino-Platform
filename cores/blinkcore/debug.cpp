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