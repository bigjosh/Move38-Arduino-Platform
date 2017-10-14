

#ifndef BLINKCORE_H_
#define BLINKCORE_H_

// This mess is to avoid "warning: "F_CPU" redefined" under Arduino IDE.
// If you know a better way, please tell me!

#ifdef F_CPU
#undef F_CPU
#endif

#define F_CPU 4000000				// Default fuses are DIV8 so we boot at 1Mhz,
// but then we change the prescaler to get to 4Mhz by the time user code runs

#define FACE_COUNT 6				// Total number of IRLEDs

// *** Ultils

// Bit manipulation macros
#define SBI(x,b) (x|= (1<<b))           // Set bit in IO reg
#define CBI(x,b) (x&=~(1<<b))           // Clear bit in IO reg
#define TBI(x,b) (x&(1<<b))             // Test bit in IO reg

#define FOREACH_FACE(x) for(int x = 0; x < FACE_COUNT ; ++ x)       // Pretend this is a real language with iterators

#define DO_ATOMICALLY ATOMIC_BLOCK(ATOMIC_FORCEON)                  // Non-HAL code always runs with interrupts on, so give users a way to do things atomically.

void inline INC_NO_OVERFLOW( uint8_t &x ) {
    if (!(++x)) x--;            // Prevent overflow by decrementing if we do overflow.
}


#endif