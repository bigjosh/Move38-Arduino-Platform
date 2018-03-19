/*
 * shared.h
 *
 * Some core values that are shared with higher levels.
 *
 * These could be exposed cleanly via function calls, but then they would only be known at runtime.
 * Putting them here lets higher levels see them without having to pull in morew of the core namespace.
 *
 * Created: 7/14/2017 1:50:04 PM
 *  Author: josh
 */


#ifndef SHARED_H_
#define SHARED_H_

/*** Utils serial number ***/

// Legth of the globally unique serial number returned by utils_serialno()

#define SERIAL_NUMBER_LEN 9

/*** Global processor speed ***/

// This mess is to avoid "warning: "F_CPU" redefined" under Arduino IDE.
// If you know a better way, please tell me!


#ifdef F_CPU
#undef F_CPU
#endif

#define F_CPU 8000000UL				// Default fuses are DIV8 so we boot at 1Mhz,
									// but then we change the prescaler to get to 8Mhz by the time user code runs


/*** Number of faces ***/

#define FACE_COUNT 6				// Total number of IRLEDs


#endif 