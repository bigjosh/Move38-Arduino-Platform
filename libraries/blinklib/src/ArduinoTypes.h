/*

	Some types used in idiomatic Arduino code and Arduino compatible function signatures.
	We break them out into thier own header so we can implement against them without having to
	include the toxic Arduino.h itself.

*/


#ifndef ArduinoTypes_h

    #define ArduinoTypes_h


	#include <stdint.h>
    typedef bool boolean;
    typedef uint8_t byte;
    typedef unsigned int word;

    typedef uint32_t ulong;

#endif