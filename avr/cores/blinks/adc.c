/*
 * Internal analog to digital converter
 * 
 * We connect the ADC to the battery so we can check its voltage.
 *
 */ 


#include "blinks.h"
#include "hardware.h"

#include "adc.h"

#include "utils.h"


// Set up adc to read Vcc
// https://wp.josh.com/2014/11/06/battery-fuel-guage-with-zero-parts-and-zero-pins-on-avr/

void adc_init(void) {
	
	ADMUX =
	_BV(REFS0)  |                                  // Reference AVcc voltage
	_BV( ADLAR ) |                                 // Left adjust result so only one 8 bit read of the high register needed
	_BV( MUX3 ) | _BV( MUX2 )  | _BV( MUX1 )      // Measure internal 1.1V bandgap voltage
	;
	

	// Set the prescaller based on F_CPU
	// Borrowed from https://github.com/arduino/Arduino/blob/2bfe164b9a5835e8cb6e194b928538a9093be333/hardware/arduino/avr/cores/arduino/wiring.c#L353
	
	// set a2d prescaler so we are inside the desired 50-200 KHz range.
	#if F_CPU >= 16000000 // 16 MHz / 128 = 125 KHz
	SBI(ADCSRA, ADPS2);
	SBI(ADCSRA, ADPS1);
	SBI(ADCSRA, ADPS0);
	#elif F_CPU >= 8000000 // 8 MHz / 64 = 125 KHz
	SBI(ADCSRA, ADPS2);
	SBI(ADCSRA, ADPS1);
	CBI(ADCSRA, ADPS0);
	#elif F_CPU >= 4000000 // 4 MHz / 32 = 125 KHz
	SBI(ADCSRA, ADPS2);
	CBI(ADCSRA, ADPS1);
	SBI(ADCSRA, ADPS0);
	#elif F_CPU >= 2000000 // 2 MHz / 16 = 125 KHz
	SBI(ADCSRA, ADPS2);
	CBI(ADCSRA, ADPS1);
	CBI(ADCSRA, ADPS0);
	#elif F_CPU >= 1000000 // 1 MHz / 8 = 125 KHz
	CBI(ADCSRA, ADPS2);
	SBI(ADCSRA, ADPS1);
	SBI(ADCSRA, ADPS0);
	#else // 128 kHz / 2 = 64 KHz -> This is the closest you can get, the prescaler is 2
	CBI(ADCSRA, ADPS2);
	CBI(ADCSRA, ADPS1);
	SBI(ADCSRA, ADPS0);
	#endif
	// enable a2d conversions
	SBI(ADCSRA, ADEN);

	
	SBI( ADCSRA , ADSC);                // Kick off a primer conversion (the initial one is noisy)
	
	while (TBI(ADCSRA,ADSC)) ;       // Wait for 1st conversion to complete

	SBI( ADCSRA , ADSC);                // Kick off 1st real conversion (the initial one is noisy)

	
}

// Start a new conversion

void adc_startConversion(void) {
	SBI( ADCSRA , ADSC);					// Start next conversion, will complete silently in 14 cycles
}

// Returns the previous conversion result and starts a new conversion.
// ADC clock running at /8 prescaller and conversion takes 14 cycles, so don't call more often than once per
// 112 microseconds


uint8_t adc_lastVccX10(void) {              // Return Vcc x10
	
	while (TBI(ADCSRA,ADSC)) ;       // Wait for any pending conversion to complete

	uint8_t lastReading = (11 / ADCH);      // Remember the result from the last reading.
	
	adc_startConversion();				// Start next conversion, will complete silently in 14 cycles

	return( lastReading  );
	
}



