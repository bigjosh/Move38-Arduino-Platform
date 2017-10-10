/*
 * utils.h
 *
 * Created: 7/14/2017 1:50:04 PM
 *  Author: josh
 */ 


#ifndef UTILS_H_
#define UTILS_H_

#include <util/atomic.h>

// Duplicated from Arduino.h

typedef bool boolean;
typedef uint8_t byte;

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

#define DO_ATOMICALLY ATOMIC_BLOCK(ATOMIC_FORCEON)                  // Non-HAL codes always runs with interrupts on, so give users a way to do things atomically.

void inline INC_NO_OVERFLOW( uint8_t &x ) {
    if (!(++x)) x--;            // Prevent overflow by decrementing if we do overflow. 
}    

/* 
  Compiles to:
  
 280:	24 81       	ldd	r18, Z+4	; 0x04
 282:	2f 5f       	subi	r18, 0xFF	; 255
 284:	09 f0       	breq	.+2      	; 0x288 <__vector_4+0x50>
 286:	24 83       	std	Z+4, r18	; 0x04  

*/

// TODO: Is there some binary way to do this withouth the branch?

// TODO: Where does this belong?
void run(void);

#endif /* UTILS_H_ */