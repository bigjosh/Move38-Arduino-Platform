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

// Returns the previous conversion result.
// Blocks if conversion not ready yet.
// The value is the voltage of the battery * 10.
// So 30 = 3.0 volts

uint8_t adc_readLastVccX10(void);               // Return Vcc x10

// Disable and power down the ADC to save power

void adc_disable(void);

#endif /* ADC_H_ */