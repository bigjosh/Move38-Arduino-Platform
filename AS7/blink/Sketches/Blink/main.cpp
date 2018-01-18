/*
    
    Blink Pixel Balance Utility
    
    This utility allows you to find the best 32 brightness levels for the display pixels
    from the 255 possible drive pulse widths possible with the hardware. 
    
    To use it, you will need a blink with a service port installed and a Dev Candy board.
    
    Download this program into the blink and then connect to the service port  with serial monitor.
    
    Pixels P0, P2, and P4 display the current values for R,G, and B respectively.
    Pixels P1, P3, and P5 display the mix of the two adjacent colors. 
    
*/

#include <ctype.h>      // toupper()

#include "Serial.h"
#include "pixel.h"      // pixel_rawSetPixel()
#include "sp.h"         // sp_aux_analogRead()

struct color_t {
    
    char key;
    uint8_t pwmValue;
    
};  


// Keep an array of current active PWM values for each color
    
color_t colors[] = {
    { 'R' , 20 },
    { 'G' , 20 },
    { 'B' , 20 },    
};    // Init with some random but likely visible values

ServicePortSerial sp;
void setup() {
    
    sp.begin();
    sp.println("Blink Pixel Color Balance Utility");
}

uint8_t currentColor=2;     // Which color are we currently adjusting? Start with blue.
   
void loop() {
    
    sp.print( "Currently adjusting ");
    sp.print( colors[currentColor].key );
    sp.print( ":");
    sp.println( colors[currentColor].pwmValue );
    sp.println( "Switch to [R]ed, [G]reen, [B]lue - Turn dial to adjust");
            
    while (1) {
        
        // Display the current values
        
        pixel_bufferedSetPixelRaw( 0 , colors[0].pwmValue   , 255               , 255 );
        pixel_bufferedSetPixelRaw( 1 , colors[0].pwmValue   , colors[1].pwmValue, 255 );
        pixel_bufferedSetPixelRaw( 2 , 255                  , colors[1].pwmValue, 255 );
        pixel_bufferedSetPixelRaw( 3 , 255                  , colors[1].pwmValue, colors[2].pwmValue  );
        pixel_bufferedSetPixelRaw( 4 , 255                  , 255               , colors[2].pwmValue  );
        pixel_bufferedSetPixelRaw( 5 , 255 , 255               , 255 );
        //pixel_bufferedSetPixelRaw( 5 , colors[0].pwmValue   , 255               , colors[2].pwmValue  );
        pixel_displayBufferedPixels();

        uint8_t pwmValue = colors[currentColor].pwmValue;
        
        
        sp.flush();                             // Make sure we are done transmitting before reading analog pin
        uint8_t dial = sp_aux_analogRead();     // Get dial setting
        
        if (dial != pwmValue ) {
            
            colors[currentColor].pwmValue = dial;
            return;
            
        }            
        
        if (sp.available()) {
                        
            byte key =  sp.readWait() ;
                       
            switch ( key ) {
                
                case 'R':
                case 'G':
                case 'B':
                    for( uint8_t i=0; i<COUNT_OF(colors);i++) {
                        if (colors[i].key==key) {
                            currentColor=i;
                            return;
                        }
                    }                            
                    break;
                                            
                case 'M': 
                    if (pwmValue>10) {
                        pwmValue-=10;
                    }
                    break;                        
                    
                case '<':
                    if (pwmValue>0) {
                        pwmValue--;
                    }
                    break;

                case '>':
                    if (pwmValue<255) {
                        pwmValue++;
                    }
                    break;
                    
                case '/':
                    if (pwmValue<(255-10)) {
                        pwmValue+=10;
                    }
                    break;
                    
            }
            
            if (pwmValue != colors[currentColor].pwmValue ) {
                colors[currentColor].pwmValue = pwmValue;
                return;                      
            }                              
        }        
    }        
        
        
}


