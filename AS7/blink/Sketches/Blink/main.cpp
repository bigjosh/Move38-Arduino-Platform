#include <avr/io.h>

#define F_CPU 8000000

#include <util/delay.h>

#include "bitfun.h"

#include "adc.h"

void setup() {
    
    //adc_init();    

}


void showblink( uint8_t digit , Color c ) {
    
    if (digit==0) {
        digit=10;        
    }        
    
    while (digit--) {
        setColor( c );
        _delay_ms(100);
        setColor( OFF  );
        _delay_ms(100);
    }        
    
}    

void loop() {
    
    setColor( BLUE );
    _delay_ms(100);
    
    return;
    
   // adc_startConversion();
    
    //uint8_t r = adc_readLastResult();
    
    uint8_t r = 24;
    
    Color c;
    
    if (r > 24) {     // 2.4v min 
        c = GREEN;
    } else {
        c= RED;
    }                
        
    uint8_t whole = r / 10 ;
    
    uint8_t dec = r % 10;
    
    
    showblink( whole , c );
    
    _delay_ms( 250 );
    
    showblink( dec , c );
    
    _delay_ms( 500 );
    

}           // loop()


