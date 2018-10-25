/*

    Include this instead of Serial.h to make the serial functions compile away, while still
    leaving the calls inside you program for documentation or future development. 
  
*/

#include <inttypes.h>

#include "ArduinoTypes.h"


// Public Methods //////////////////////////////////////////////////////////////

inline void ServicePortSerial::begin(void)
{
}

inline void ServicePortSerial::end()
{
}

// We only use the 1 byte hardware buffer

inline int ServicePortSerial::available(void) {
        return(0);
}

inline int ServicePortSerial::read(void)
{
    return -1;
}

inline byte ServicePortSerial::readWait(void)
{
    return 0;
}

// We don't implement flush because it would require adding a flag to remember if we ever sent.
// This is because the hardware only gives us a bit that tells us when a tx completes, not if
// no TX was ever started. Ardunio does this with the `_writen` flag.
// If you can convince me that we really need flush, LMK and it can be added.

inline size_t ServicePortSerial::write(uint8_t c)
{

  return(1);

}

// Block until all pending transmits complete

inline void ServicePortSerial::flush(void)
{
}