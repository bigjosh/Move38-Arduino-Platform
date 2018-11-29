#include <avr/io.h>

#define F_CPU 8000000

#include <util/delay.h>

//#include "bitfun.h"

//#include "adc.h"

#include "Serial.h"

ServicePortSerial sp;


void setup() {
    
    adc_init();  
    
    adc_enable();  
    
    sp.begin();

}


// Returns the previous conversion result (call adc_startConversion() to start a conversion).
// Blocks if you call too soon and conversion not ready yet.

uint8_t adc_readLastVccX10(void) {              // Return Vcc x10
    
    while (TBI(ADCSRA,ADSC)) ;       // Wait for any pending conversion to complete

    uint8_t lastReading = (255*11 / ADCH);      // Remember the result from the last reading.
    
    sp.println(lastReading);    
    
    return( lastReading  );
    
}

#define DELAY_BLINK_MS 200      // Delay between flashes
#define DELAY_DIGIT_MS 750      // Delay between 10's and 1's digits
#define DELAY_READ_MS 1000    // Delay between consecutive readings



enum phase_t { READ , TENS, ONES  };
    
enum state_t { S_OFF , S_ON };

static phase_t phase=READ;
static state_t state=S_OFF;

static Timer next;

static uint8_t c=0;

static uint8_t lastReading=24;

void loop() {
    /*
    if (next.isExpired()) {
        
        if (state==S_ON) {
            
            setColor( OFF );
            
            state=S_OFF;
            
        } else {
            setColor( BLUE );
            
            state=S_ON;
            
        }                    
        
        next.set(500);
        
    }            
    
    return;
    */
    
    if (phase==READ) {
                                
        adc_startConversion();   
                         
        lastReading = adc_readLastVccX10();
                    
        c = lastReading/10;     // Tens digit
        
        if (c==0) c=10;
            
        phase = TENS;
            
        state = S_OFF;
        
        next.set(2000);        // Show immediately
            
    }            
    
    if (c==0) {         // Done with this digit
                        
        if (phase==TENS) {
                    
            c = lastReading%10;     // Ones digit
            
            if (c==0) c=10;            
                    
            phase = ONES;
                                        
            next.set( DELAY_DIGIT_MS ); 
            
        } else {
            
            phase = READ;
            
            next.set( DELAY_READ_MS );
        }                    
        
    } else {
        
        if (next.isExpired()) {
                       
            if (state==S_OFF) {
                
                setColor( RED );
                
                state = S_ON;
                
                next.set( DELAY_BLINK_MS ); 
                
            } else {        // state==ON
                
                setColor( OFF );
                
                state = S_OFF;
                
                c--;
                
                next.set( DELAY_BLINK_MS );
                
            }
            
        }                            
        
    }    
                             
}           // loop()


