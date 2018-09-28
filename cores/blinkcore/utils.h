/*
 * utils.h
 *
 * Access to the unique serial number inside the ATMEGA chip.
 *
 * Created: 7/14/2017 1:50:04 PM
 *  Author: josh
 */


#ifndef UTILS_H_
#define UTILS_H_

#include "shared.h"

typedef struct {
    uint8_t bytes[SERIAL_NUMBER_LEN] ;
} utils_serialno_t;

// Returns the device's unique 9-byte serial number

utils_serialno_t const *utils_serialno(void);

/*
  Compiles to:

 280:	24 81       	ldd	r18, Z+4	; 0x04
 282:	2f 5f       	subi	r18, 0xFF	; 255
 284:	09 f0       	breq	.+2      	; 0x288 <__vector_4+0x50>
 286:	24 83       	std	Z+4, r18	; 0x04

*/

#endif /* UTILS_H_ */