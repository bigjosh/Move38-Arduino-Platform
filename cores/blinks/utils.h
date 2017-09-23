/*
 * utils.h
 *
 * Created: 7/14/2017 1:50:04 PM
 *  Author: josh
 */ 


#ifndef UTILS_H_
#define UTILS_H_

#include <util/atomic.h>

// Bit manipulation macros
#define SBI(x,b) (x|= (1<<b))           // Set bit in IO reg
#define CBI(x,b) (x&=~(1<<b))           // Clear bit in IO reg
#define TBI(x,b) (x&(1<<b))             // Test bit in IO reg

typedef struct {
    uint8_t bytes[9] ;
} utils_serialno_t;

// Returns the device's unique 8-byte serial number

utils_serialno_t const *utils_serialno(void);

// *** Ultils

#define FOREACH_FACE(x) for(int x = 0; x < FACE_COUNT ; ++ x)       // Pretend this is a real language with iterators

#define DO_ATOMICALLY ATOMIC_BLOCK(ATOMIC_FORCEON)                  // Non-HAL codes always runs with interrupts on, so giove users a way to do things atomically.

// TODO: Where does this belong?
void run(void);

#endif /* UTILS_H_ */