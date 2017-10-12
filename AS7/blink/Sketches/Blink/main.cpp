#include "Arduino.h"
#include "hardware.h"

#include "blinklib.h"

#include "ir.h"

#include "utils.h"

#include "util/delay.h"

void setup() {

	// No setup needed for this simple example!

}


#define IDLE_TIME_US 1100

static uint8_t oddParityx(uint8_t p) {
    p = p ^ (p >> 4 | p << 4);
    p = p ^ (p >> 2);
    p = p ^ (p >> 1);
    return p & 1;
}

void txbit( bool bit ) {
    
	ir_tx_pulse( 0b00111111 );
	_delay_us(200);
	ir_tx_pulse( 0b00111111 );

	if (bit) {
    	_delay_us(200);
    	ir_tx_pulse( 0b00111111 );
	}
			
	_delay_us(IDLE_TIME_US);
    
}                

void loop() {
	
	
	for( int x=0; x<255; x++ ) {
	  
		cli();
		// Sync pulses....
		ir_tx_pulse( 0b00111111 );
		_delay_us(IDLE_TIME_US);
		ir_tx_pulse( 0b00111111 );
		_delay_us(IDLE_TIME_US);
		
		
		int bit=8;
	  
		while (bit--) {

                txbit( TBI( x , bit ) );
	  
		}
        
        txbit( oddParityx( x) );
        
		sei();
		
	  
		setColor( dim(GREEN,16) );
		delay(50);       // Show the color long enough to see
		setColor(OFF);
		delay(200);
	}
}
