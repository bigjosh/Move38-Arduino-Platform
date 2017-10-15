#include "Arduino.h"
#include "hardware.h"

#include "blinklib.h"

#include "ir.h"

#include "utils.h"

#include "util/delay.h"

void setup() {

  // No setup needed for this simple example!

  for( int b=0; b<31; b++ ) {
    setColor( dim( GREEN , b ) );
    delay(100);
  }

  setColor(OFF);
}

typedef enum { TX , RX, SYNC , ERROR } mode_t;

mode_t mode=TX;      // Are we transmitting or receiving?

uint8_t count=0;

void loop() {
    
    
    if( buttonSingleClicked() ) {

        // Click in TX goes to SYNC.
        // Click any other mode goes back to TX 
       
        if (mode==TX) {
            mode=SYNC;
        } else {
            mode=TX;
        }                        
               
        
    }        

    switch (mode) {

      case TX:
        
        setColor( RED );
        
        ir_sendData( 5 , count );
        
        count++;
        
        delay( 50);
        
        setColor( OFF );
        
        delay(100);

        break;

    case SYNC: 

        setColor( YELLOW );
        
        if ( 0 && !ir_isReady(0)) {          // Wait for 1st message to come in
            
          count = ir_getData(0); 

        } else {

          delay(20); 
          setColor(OFF);           

        }

        break;
            

    case RX:
        
                    
        if (!ir_isReady(0)) {          // Wait for 1st message to come in

          count++;        // next number expected (rolls on overflow)                
          
          if (count != ir_getData(0) ) {
                                   
            mode = ERROR;
            
          } else {
            
            setColor(GREEN);
            delay(50);
            setColor(OFF);            
          }   

        }
        
        break;

      case ERROR:        

        setColor(RED);


        // Do nothing in error mode - just show solid red
        break;

    }
    
}


