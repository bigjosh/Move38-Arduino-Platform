

#ifndef BLINKCORE_H_
#define BLINKCORE_H_

// Yeah, I know F_CPU really belongs in hardware.h, but then so much stuff would get unnessisarly pulled in.
// Maybe it belongs in its own header file. 
// TODO: Maybe break out into a delay.h since _delay_us() is really the main use case here. 

// This mess is to avoid "warning: "F_CPU" redefined" under Arduino IDE.
// If you know a better way, please tell me!

#ifdef F_CPU
#undef F_CPU
#endif

#define F_CPU 4000000UL				// Default fuses are DIV8 so we boot at 1Mhz,
// but then we change the prescaler to get to 4Mhz by the time user code runs

#define FACE_COUNT 6				// Total number of IRLEDs

// *** Ultils

// Bit manipulation macros
#define SBI(x,b) (x|= (1<<b))           // Set bit in IO reg
#define CBI(x,b) (x&=~(1<<b))           // Clear bit in IO reg
#define TBI(x,b) (x&(1<<b))             // Test bit in IO reg

#define BLINKCORE_UINT16_MAX (0xffff)                               // I can not get stdint.h to work even with #define __STDC_LIMIT_MACROS, so have to resort to this hack. 

#endif
