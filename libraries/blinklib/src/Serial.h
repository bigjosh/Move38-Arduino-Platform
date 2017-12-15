/*

  A very simplified serial port built to look like Arduino's Serial class. 
  
  This serial port lives on the service port on new boards. It is really intended for debugging. 
  
  The port always runs at a fixed 500K baud. Adapter cables and boards will be available to connect to a USB port
  and then use the Arduino IDE's serial monitor to interact with your tile. 
  
*/

//#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"

#ifndef Serial_h

    #define Serial_h

    #include <inttypes.h>

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
        virtual byte readWait(void);            // Blocking read (faster)
        
        virtual size_t write(uint8_t);
        void flush(void);                       // Block until all pending transmits complete
        
        using Print::write;                     // pull in write(str) and write(buf, size) from Print
        
                
    };

#endif