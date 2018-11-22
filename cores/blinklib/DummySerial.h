/*

  A very simplified serial port built to look like Arduino's Serial class.

  This serial port lives on the service port on new boards. It is really intended for debugging.

  The port always runs at a fixed 1M baud. Dev Candy Adapter boards and cables are available to
  connect to a USB port and then use the Arduino IDE's serial monitor to interact with your tile.

*/

#include <inttypes.h>

#include "ArduinoTypes.h"

#ifndef Serial_h

#define Serial_h

#include "Print.h"

class ServicePortSerial : public Print
{

    public:

    //inline ServicePortSerialSerial(void);
    void begin(void);
    void end();

    virtual int available(void);

    // Input buffer is only 1 byte so if an interrupts happens while data is streaming, you will loose any incoming data.
    // Best to limit yourself interactions with plenty of time (or an ACK) between each incoming byte.

    virtual int read(void);                 // Read a byte - returns byte read or -1 if no byte ready.
    virtual uint8_t readWait(void);            // Blocking read (faster)

    virtual size_t write(uint8_t);
    void flush(void);                       // Block until all pending transmits complete

    using Print::write;                     // pull in write(str) and write(buf, size) from Print


};

#endif



// Public Methods //////////////////////////////////////////////////////////////

void ServicePortSerial::begin(void)
{
}

void ServicePortSerial::end()
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

void ServicePortSerial::flush(void)
{
}