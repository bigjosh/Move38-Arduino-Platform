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

// Enable ADC and prime the pump (it takes 2 conversions to get accurate results)

void adc_enable(void);

// Start a new conversion. Read the result ~1ms later by calling adc_readLastVccX10().
// 1ms is safe, but if you need faster then conversion will actually be ready in
// 13 CPU cycles * ADC prescaller (25 cycles for 1st conversion)

void adc_startConversion(void);

// What reading would we get back from adc_readLastResult() that would correspond to this voltage?
// Set up as #define so compare can be static
// Double check the result for the voltage you care about because there can be significant loss of precision and rounding effects

#define ADC_V_TO_READING( v ) ((1.1 * 255.0)/v)

// Returns the previous conversion result (call adc_startConversion() to start a conversion).
// Blocks if you call too soon and conversion not ready yet.

uint8_t adc_readLastResult(void);              // Return 1.1V reference as measured against Vcc scale (0=0V, 255=Vcc)

// Takes the average of 10 battery readings and compares to the minimum voltage needed.
// Returns 0 if battery too low for operations
// Power of two saves some division work
// 256 is a very nice value, but takes a while

uint8_t battery_voltage_fit();

// Disable and power down the ADC to save power

void adc_disable(void);

#endif /* ADC_H_ */