

#ifndef BLINKCORE_H_
#define BLINKCORE_H_

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


// 'Cause C ain't got iterators and all those FOR loops are too ugly. 
#define FOREACH_FACE(x) for(int x = 0; x < FACE_COUNT ; ++ x)       // Pretend this is a real language with iterators

// Get the number of elements in an array.
#define COUNT_OF(x) ((sizeof(x)/sizeof(x[0]))) 

#define DO_ATOMICALLY ATOMIC_BLOCK(ATOMIC_FORCEON)                  // Non-HAL code always runs with interrupts on, so give users a way to do things atomically.

#define BLINKCORE_UINT16_MAX (0xffff)                               // I can not get stdint.h to work even with #define __STDC_LIMIT_MACROS, so have to resort to this hack. 

#endif