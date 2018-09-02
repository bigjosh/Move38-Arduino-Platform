/*
 * Access the service port 
 *
 * The service port has a high speed (500Kbps) bi-directional serial connection plus an Aux pin that can be used
 * as either digitalIO or an analog in. 
 *
 * Mostly useful for debugging, but maybe other stuff to? :)
 * 
 */ 

#include "hardware.h"

#include "utils.h"

#include "sp.h"


// Read the analog voltage on service port pin A
// Returns 0-255 for voltage between 0 and Vcc
// Handy to connect a potentiometer here and use to tune params
// like rightness or speed

uint8_t sp_aux_analogRead(void) {
    
	ADMUX =
	_BV(REFS0)   |                  // Reference AVcc voltage
	_BV( ADLAR ) |                  // Left adjust result so only one 8 bit read of the high register needed
	_BV( MUX2 )  | _BV( MUX1 )      // Select ADC6
	;
	
	ADCSRA =
	_BV( ADEN )  |                  // Enable ADC
	_BV( ADSC )                     // Start a conversion
	;
	
	
	while (TBI(ADCSRA,ADSC)) ;       // Wait for conversion to complete
	
	return( ADCH );
	
}


// Initialize the serial on the service port. 
// Overrides digital mode for service port pins T and R respectively. 

void sp_serial_init(void) {
	sp_serial_init(500000);
}

void sp_serial_init(unsigned long _baudrate=500000) {
    
    //Initialize the AUX pin as digitalOut
    //SBI( SP_AUX_DDR , SP_AUX_PIN );
    
    // Initialize SP serial port for 500K baud, n-8-1
    // This feels like it belongs in hardware.c, maybe in an inline function?
        
    SBI( SP_SERIAL_CTRL_REG , U2X0);        // 2X speed
    
    SBI( UCSR0B , TXEN0 );                  // Enable transmitter (disables digital mode on T pin)
    
    SP_PIN_R_SET_1();                       // Enable pull-up on RX pin so we can use an open-collector to drive it 
    SBI( UCSR0B , RXEN0);                   // Enable receiver    (disables digital mode on R pin)
    
    #if F_CPU!=4000000  
        #error Serial port calculation in debug.cpp must be updated if not 4Mhz CPU clock.
    #endif
    
    /*
     * kenj:
     * UBRR calculation for async double speed is:
     * (from http://masteringarduino.blogspot.com/2013/11/USART.html)
     *    UBRR=(F_CPU/(8 * baudrate)-1)
     */
  switch(_baudrate) {
    case 250000:
		UBRR0=1;
		break;
    case 500000:
    default:
		UBRR0 = 0;                  // 500Kbd. This is as fast as we can go at 4Mhz, and happens to be 0% error and supported by the Arduino serial monitor. 
                                    // See datasheet table 25-7. 
		break;
    break;
  }
}   

// Free up service port pin R for digital IO again

void sp_serial_disable_rx(void) {
    CBI( UCSR0B , RXEN0 );                  // Enable transmitter (disables digital mode on T pin)    
}     

// Free up service port pin T for digital IO again

void sp_serial_disable_tx(void) {
    CBI( UCSR0B , TXEN0 );                  // Enable transmitter (disables digital mode on T pin)
}


// Send a byte out the serial port. DebugSerialInit() must be called first. Blocks unitl buffer free if TX already in progress.

void sp_serial_tx(uint8_t b) {    
        
    while (!TBI(SP_SERIAL_CTRL_REG,UDRE0));         // Wait for buffer to be clear so we don't overwrite in progress
    
    SP_SERIAL_DATA_REG=b;                           // Send new byte
    
}

// Wait for most recently transmitted byte to complete

void sp_serial_flush(void) {
    
    while (!TBI(SP_SERIAL_CTRL_REG,TXC0));         // Wait until the entire frame in the Transmit Shift Register has been shifted out and there are
                                                   // no new data currently present in the transmit buffer
        
}    


// Is there a char ready to read?

uint8_t sp_serial_rx_ready(void) {
    
    return TBI( SP_SERIAL_CTRL_REG , SP_SERIAL_READY_BIT );
    
}    

// Read byte from service port serial. Blocks if nothing received yet. 

uint8_t sp_serial_rx(void) {
    
    while ( !TBI( SP_SERIAL_CTRL_REG , SP_SERIAL_READY_BIT ) );
    
    return( SP_SERIAL_DATA_REG );
    
}

