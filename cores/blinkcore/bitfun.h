

#ifndef BITFUN_H_
#define BITFUN_H_

// Bit manipulation macros
	#define SBI(x,b) (x|= (1<<b))           // Set bit in IO reg
	#define CBI(x,b) (x&=~(1<<b))           // Clear bit in IO reg
	#define TBI(x,b) (x&(1<<b))             // Test bit in IO reg


#endif
