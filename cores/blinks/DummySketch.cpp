#include "callbacks.h"
#include "pixel.h"

#include "hardware.h"

#include <util/delay.h>

// This dummy sketch exists solely to provide a run() so that we can compile without errors. 

// If you are allergic to the Arduino IDE, you can also develop a sketch here and then copy/paste it into the IDE when done. 

void run(void) {
    
    
    
    for(int i=0; i < 16; i++ ) {
        
        pixel_SetAllRGB( 0 , i << 4 , 0  );
        _delay_ms(100);
        
    }                
        
    for(int i=0; i < 16; i++ ) {
            
        pixel_SetAllRGB( 0 , (15-i) << 4 , 0  );
        _delay_ms(100);
            
    }

    
}