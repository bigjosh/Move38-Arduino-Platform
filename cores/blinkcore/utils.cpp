/*
 * Time related functions
 *
 */


#include "hardware.h"

#include "utils.h"


// Returns the device's unique 8-byte serial number

utils_serialno_t const *utils_serialno(void) {

    // 0xF0 points to the 1st of 8 bytes of serial number data
    // As per "13.6.8.1. SNOBRx - Serial Number Byte 8 to 0"

    return ( utils_serialno_t *)   0xF0;
}    