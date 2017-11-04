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
        virtual int read(void);
        virtual size_t write(uint8_t);
        using Print::write; // pull in write(str) and write(buf, size) from Print
        operator bool() { return true; }

    };

#endif