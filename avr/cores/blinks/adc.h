/*
 * Internal analog to digital converter
 * 
 * We connect the ADC to the battery so we can check its voltage.
 *
 */ 

#ifndef ADC_H_
#define ADC_H_

#include <avr/io.h>

// Initialize the ADC and start the first conversion

void adc_init(void);


// Returns the previous conversion result and starts a new conversion.
// The value is the voltage of the battery * 10.
// So 30 = 3.0 volts

uint8_t adc_lastVccX10(void);               // Return Vcc x10


// Start a new conversion now.
// If you want a fresh conversion rather than the most recently completed one 
// which may be stale, call this before calling adc_lastVccX10() 

void adc_startConversion(void);


#endif /* ADC_H_ */